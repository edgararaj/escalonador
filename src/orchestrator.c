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

#define NANOSECONDS_IN_SECOND 1000000000L
#define NANOSECONDS_IN_MILLISECOND 1000000L

typedef struct {
    char* ficheiro;
    int tempo;
} Bin;

typedef struct {
    int ini, tam, uti;
    Bin* args;
} Queue;

typedef struct {
    int pid;
    long seconds;
    long milliseconds;
} TerminationLog;

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
    // create path to output file consisting of <output_folder>/<pid>.txt
    // calculate length of <pid>
    int len = snprintf(NULL, 0, "%s/%d.txt", output_folder, pid);
    char* path = malloc(len);

    // A terminating null character is automatically appended after the content written.
    sprintf(path, "%s/%d.txt", output_folder, pid);

    return path;
}

int mysystem(const char* command, const char* output_folder, TerminationLog* b)
{
    int res = -1;

    // Estamos a assumir numero maximo de argumentos
    // isto teria de ser melhorado com realloc por exemplo
    char* exec_args[20];
    char *string, *cmd, *tofree;
    int i = 0;
    tofree = cmd = strdup(command);
    while ((string = strsep(&cmd, " ")) != NULL) {
        exec_args[i] = string;
        i++;
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

        printf("PID %d: %s\n", cpid, command);
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

        b->pid = cpid;
        b->seconds = seconds;
        b->milliseconds = milliseconds;
    }

    free(tofree);

    return res;
}

void initQueue(Queue* q)
{
    q->ini = q->uti = 0;
    q->tam = 1;
    q->args = malloc(sizeof(Bin) * q->tam);
}

void freeQueue(Queue* q)
{
    for (int i = 0; i < q->uti; i++) {
        free(q->args[i].ficheiro);
    }
    free(q->args);
}

void reallocQueue(Queue* q)
{
    Bin* novo = malloc(sizeof(Bin) * q->tam * 2);
    int i, j;
    for (j = 0, i = q->ini; j < q->uti; j++, i++) {
        novo[j].tempo = q->args[i].tempo;
        novo[j].ficheiro = q->args[i].ficheiro;
        q->args[i].ficheiro = NULL;
    }
    free(q->args);
    q->args = novo;
    q->ini = 0;
    q->tam *= 2;
}

void inQueue(char* ficheiro, int tempo, Queue* q)
{
    if (q->tam == q->uti) {
        reallocQueue(q);
    }
    int pos = (q->ini + q->uti) % q->tam;

    q->args[pos].ficheiro = strdup(ficheiro);
    q->args[pos].tempo = tempo;

    q->uti++;
}

int deQueue(Queue* q, Bin* a)
{
    int i = 0;
    if (q->uti > 0) {
        int pos = q->ini;

        a->ficheiro = q->args[pos].ficheiro;
        q->args[pos].ficheiro = NULL;
        a->tempo = q->args[pos].tempo;

        i = 1;
        q->ini = (q->ini + 1) % q->tam;
        q->uti--;
    }
    return i;
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

    Queue q;
    initQueue(&q);

    int N = atoi(argv[2]); // Number of parallel tasks
    int tasks_running = 0; // Number of tasks running

    mkfifo(TASK_FIFO, 0644);

    int fd = open(TASK_FIFO, O_RDONLY);
    printf("fifo opened for reading\n");

    int pipefd[2];
    pipe(pipefd);

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

    char* completed_path = get_tmp_filepath("tmp", "completed.txt");

    while (1) {
        Task t;
        if (read(fd, &t, sizeof(Task)) > 0) {
            inQueue(t.command, t.time, &q);
        }

        TerminationLog b;
        if (read(pipefd[0], &b, sizeof(TerminationLog)) > 0) {
            {
                int completed_fd = open(completed_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
                char buf[1024];
                printf("PID %d: Terminou após %ld.%03ld seg\n", b.pid, b.seconds, b.milliseconds);
                sprintf(buf, "PID %d: Terminou após %ld.%03ld seg\n", b.pid, b.seconds, b.milliseconds);
                write(completed_fd, buf, strlen(buf));
                close(completed_fd);
            }
            tasks_running--;
        }

        while (tasks_running < N && q.uti) {
            Bin a;
            if (deQueue(&q, &a)) {
                pid_t cpid = fork();
                if (cpid == 0) {
                    // close read end of pipe
                    close(pipefd[0]);

                    TerminationLog b;
                    // write to pipe
                    mysystem(a.ficheiro, argv[1], &b);

                    write(pipefd[1], &b, sizeof(TerminationLog));

                    _exit(0);
                } else if (cpid > 0) {
                    free(a.ficheiro);
                    tasks_running++;
                }
            }
        }
    }

    free(completed_path);
    freeQueue(&q);
    return 0;
}