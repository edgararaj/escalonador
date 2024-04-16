#include "queues.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "orchestrator.h"
#include "status.h"

#define NANOSECONDS_IN_SECOND 1000000000L
#define NANOSECONDS_IN_MILLISECOND 1000000L

#define FCFS 0
#define SJF 1

char* get_client_callback_filepath_by_pid(int pid)
{
    // create path to output file consisting of <output_folder>/<file>
    // calculate length of <file>
    int len = snprintf(NULL, 0, "/tmp/escalonador_%d", pid);
    char* path = malloc(len);

    // A terminating null character is automatically appended after the content written.
    sprintf(path, "/tmp/escalonador_%d", pid);

    return path;
}

char* get_tmp_filepath(const char* output_folder, const char* file)
{
    // create path to output file consisting of <output_folder>/<file>
    // calculate length of <file>
    int len = snprintf(NULL, 0, "%s/%s", output_folder, file);
    char* path = malloc(len);

    // A terminating null character is automatically appended after the content written.
    sprintf(path, "%s/%s", output_folder, file);

    return path;
}

char* get_output_path(const char* output_folder)
{
    int pid = getpid();
    int pid_len = snprintf(NULL, 0, "%d", pid);
    char* path = malloc(strlen(output_folder) + pid_len + 5);
    sprintf(path, "%s/%d.txt", output_folder, pid);
    return path;
}

int mysystem(const char* command, const char* output_folder, int* time)
{
    int res = -1;

    char* exec_args[20];
    char *string, *cmd, *tofree;
    int i = 0;
    tofree = cmd = strdup(command);
    while ((string = strsep(&cmd, " ")) != NULL) {
        exec_args[i++] = string;
    }
    exec_args[i] = NULL;

    pid_t cpid = fork();
    if (cpid == 0) {
        int mystdout;
        // criar ficheiro de output
        {
            char* path = get_output_path(output_folder);
            mystdout = open(path, O_CREAT | O_WRONLY, 0644);
            free(path);
        }

        close(1); /* Close original stdout */
        dup2(mystdout, 1); /* Move mystdout to FD 1 */
        close(2); /* Close original stderr */
        dup2(mystdout, 2); /* Move mystdout to FD 2 */

        execvp(exec_args[0], exec_args);

        _exit(-1);
    } else if (cpid > 0) {
        struct timespec ts_start, ts_end;
        int status;

        if (clock_gettime(CLOCK_MONOTONIC, &ts_start) == -1) {
            perror("clock_gettime");
            exit(EXIT_FAILURE);
        }

        if (wait(&status) > 0) {
            if (WIFEXITED(status) && WEXITSTATUS(status) != 255) {
                res = WEXITSTATUS(status);
            }
        }

        if (clock_gettime(CLOCK_MONOTONIC, &ts_end) == -1) {
            perror("clock_gettime");
            exit(EXIT_FAILURE);
        }
        // Calculate time difference
        long seconds = ts_end.tv_sec - ts_start.tv_sec;
        long nanoseconds = ts_end.tv_nsec - ts_start.tv_nsec;
        if (nanoseconds < 0) {
            seconds--;
            nanoseconds += NANOSECONDS_IN_SECOND;
        }

        // Convert nanoseconds to milliseconds
        long milliseconds = nanoseconds / NANOSECONDS_IN_MILLISECOND;

        *time = seconds * 1000 + milliseconds;
    }

    free(tofree);

    return res;
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("Uso: %s <output_folder> <parallel-tasks> <sched-policy>\n", argv[0]);
        printf("Argumentos:\n");
        printf("\toutput-folder: pasta onde são guardados os ficheiros com o "
               "output de tarefas executadas.\n");
        printf("\tparallel-tasks: número de tarefas que podem ser executadas em "
               "paralelo.\n");
        printf("\tsched-policy: identificador da política de escalonamento, caso o "
               "servidor suporte várias políticas.\n");
        return 1;
    }

    int scheduling_policy = FCFS;  // Default is FCFS
    if (argc > 3) {
        if (strcmp(argv[3], "SJF") == 0) {
            scheduling_policy = SJF;
        }
    }

    MinHeap q;
    Queue queue;
    if (scheduling_policy == SJF) {
        initMinHeap(&q);
    } else {
        initQueue(&queue);
    }

    Status s;
    initStatus(s);

    int N = atoi(argv[2]); // Number of parallel tasks
    if (N <= 0) {
        printf("Number of parallel tasks must be greater than 0.\n");
        freeMinHeap(&q);
        return 1;
    }
    int tasks_running = 0; // Number of tasks running

    mkfifo(TASK_FIFO, 0644);

    int fd = open(TASK_FIFO, O_RDONLY);
    int wfd = open(TASK_FIFO, O_WRONLY);

    char* completed_path = get_tmp_filepath(argv[1], "completed.txt");

    int task_id = 0;

    Msg t;
    while (read(fd, &t, sizeof(Msg)) > 0) {
        // printf("Received msg, type: %d, pid: %d, time: %d, command: %s\n", t.type, t.pid, t.time, t.command);
        switch (t.type) {
        // Messages from client
        case STATUS: {
            {
                pid_t pid = fork();
                if (pid == 0){
                    int callback_fd;
                    char* callback_fifo = get_client_callback_filepath_by_pid(t.pid);
                    callback_fd = open(callback_fifo, O_WRONLY);
                    
                    returnStatus(s, callback_fd);


                    free(callback_fifo);
                    close(callback_fd);

                    Msg m;
                    m.pid = getpid();
                    m.type = KILL;
                    ssize_t written_bytes = write(wfd,&m,sizeof(Msg));
                    if(written_bytes == -1){
                        perror("Nao consegui escrever:");
                    }
                    _exit(0); 
                }
            }
        } break;
        case SINGLE:
        case PIPELINE: {
            Bin bin;
            bin.time = t.time;
            bin.file = t.command;
            bin.id = task_id++;
            insert(&q, bin);
            schedTask(s, bin);

            {
                int callback_fd;
                char* callback_fifo = get_client_callback_filepath_by_pid(t.pid);
                callback_fd = open(callback_fifo, O_WRONLY);

                int task_id = bin.id;
                write(callback_fd, &task_id, sizeof(int));

                free(callback_fifo);
                close(callback_fd);
            }
        } break;
        // Messages from server
        case KILL:{
            waitpid(t.pid, NULL, 0);
        }break;

        case TERMINATED: {
            waitpid(t.pid, NULL, 0);
            {
                int completed_fd = open(completed_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
                char buf[1024];
                int seconds = t.time / 1000;
                int milliseconds = t.time % 1000;
                printf("PID %d: Terminou após %d.%03d seg\n", t.id, seconds, milliseconds);
                sprintf(buf, "PID %d: Terminou após %d.%03d seg\n", t.id, seconds, milliseconds);
                write(completed_fd, buf, strlen(buf));
                close(completed_fd);
            }
            Bin b;
            b.id = t.id;
            b.time = t.time;

            terminateTask(s, b);
            tasks_running--;
        } break;
        }

        while (tasks_running < N && (scheduling_policy == SJF ? q.used : queue.used)) {
            Bin a;
            if (removeMin(&q, &a)) {
                execTask(s, a);
                pid_t cpid = fork();
                if (cpid == 0) {

                    Msg b;
                    printf("PID %d: %s\n", a.id, a.file);
                    mysystem(a.file, argv[1], &b.time);
                    b.type = TERMINATED;
                    b.pid = getpid();
                    b.id = a.id;
                    write(wfd, &b, sizeof(Msg));
                    // printf("Sent msg, type: %d, pid: %d, time: %d\n", b.type, b.pid, b.time);

                    free(a.file);
                    _exit(0);
                } else if (cpid > 0) {
                    free(a.file);
                    tasks_running++;
                }
            }
        }
    }

    free(completed_path);
    freeStatus(s);
    if(scheduling_policy == SJF){
        freeMinHeap(&q);
    } else {
        freeQueue(&queue);
    }

    return 0;
}
