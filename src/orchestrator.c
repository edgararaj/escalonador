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

#include "mysystem.h"
#include "orchestrator.h"
#include "status.h"

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

    MinHeap q;
    initMinHeap(&q);

    Status s;
    initStatus(s);

    int N = atoi(argv[2]); // Number of parallel tasks
    if (N <= 0) {
        printf("Number of parallel tasks must be greater than 0.\n");
        freeMinHeap(&q);
        freeStatus(s);
        return 1;
    }
    int tasks_running = 0; // Number of tasks running

    mkfifo(TASK_FIFO, 0644);

    int fd = open(TASK_FIFO, O_RDONLY);
    if (fd == -1) {
        perror("Error opening FIFO for reading");
        freeMinHeap(&q);
        freeStatus(s);
        return 1;
    }

    int wfd = open(TASK_FIFO, O_WRONLY);
    if (wfd == -1) {
        perror("Error opening FIFO for writing");
        close(fd);
        freeMinHeap(&q);
        freeStatus(s);
        return 1;
    }

    char* completed_path = get_tmp_filepath(argv[1], "completed.txt");

    int task_id = 0;

    Msg t;
    ssize_t br;
    while ((br = read(fd, &t, sizeof(Msg))) > 0) {

        switch (t.type) {
        // Messages from client
        case STATUS: {
            int callback_fd;
            char* callback_fifo = get_client_callback_filepath_by_pid(t.pid);
            callback_fd = open(callback_fifo, O_WRONLY);
            if (callback_fd == -1) {
                perror("Error opening client callback FIFO");
                free(callback_fifo);
                break; // Exit switch on error
            }

            returnStatus(s, callback_fd);

            free(callback_fifo);
            close(callback_fd);
        } break;
        case SINGLE:
        case PIPELINE: {
            Bin bin;
            bin.time = t.time;
            bin.file = t.command;
            bin.id = task_id++;
            insert(&q, bin);
            schedTask(s, bin);

            int callback_fd;
            char* callback_fifo = get_client_callback_filepath_by_pid(t.pid);
            callback_fd = open(callback_fifo, O_WRONLY);
            if (callback_fd == -1) {
                perror("Error opening client callback FIFO");
                free(callback_fifo);
                break; // Exit switch on error
            }

            int task_id = bin.id;
            write(callback_fd, &task_id, sizeof(int));

            free(callback_fifo);
            close(callback_fd);
        } break;
        // Messages from server
        case TERMINATED: {
            int status;
            waitpid(t.pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                perror("Error executing task");
                break; // Exit switch on error
            }
            int completed_fd = open(completed_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
            if (completed_fd == -1) {
                perror("Error opening completed file");
                break; // Exit switch on error
            }
            char buf[1024];
            int seconds = t.time / 1000;
            int milliseconds = t.time % 1000;
            printf("PID %d: Terminou após %d.%03d seg\n", t.id, seconds, milliseconds);
            sprintf(buf, "PID %d: Terminou após %d.%03d seg\n", t.id, seconds, milliseconds);
            write(completed_fd, buf, strlen(buf));
            close(completed_fd);
            Bin b;
            b.id = t.id;
            b.time = t.time;

            terminateTask(s, b);
            tasks_running--;
        } break;
        }

        while (tasks_running < N && q.used) {
            Bin a;
            if (removeMin(&q, &a)) {
                execTask(s, a);
                pid_t cpid = fork();
                if (cpid == -1) {
                    perror("Error forking process");
                    free(a.file);
                    exit(EXIT_FAILURE);
                } else if (cpid == 0) {
                    Msg b;
                    printf("PID %d: %s\n", a.id, a.file);
                    mysystem(a.file, argv[1], &b.time);
                    b.type = TERMINATED;
                    b.pid = getpid();
                    b.id = a.id;
                    write(wfd, &b, sizeof(Msg));

                    free(a.file);
                    _exit(0);
                } else {
                    free(a.file);
                    tasks_running++;
                }
            }
        }
    }

    close(fd);
    close(wfd);
    freeMinHeap(&q);
    freeStatus(s);
    free(completed_path);

    // Handle read errors
    if (br == -1) {
        perror("Error reading from FIFO");
        return 1;
    }

    return 0;
}
