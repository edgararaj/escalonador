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
#include "mysystem_pipe.h"
#include "orchestrator.h"
#include "status.h"
#include "time.h"

#define FCFS 0
#define SJF 1

char* get_client_callback_filepath_by_pid(int pid)
{
    // create path to output file consisting of <output_folder>/<file>
    // calculate length of <file>
    int len = snprintf(NULL, 0, "/tmp/escalonador_%d", pid) + 1;
    char* path = malloc(len);

    // A terminating null character is automatically appended after the content written.
    sprintf(path, "/tmp/escalonador_%d", pid);

    return path;
}

char* get_tmp_filepath(const char* output_folder, const char* file)
{
    // create path to output file consisting of <output_folder>/<file>
    // calculate length of <file>
    int len = snprintf(NULL, 0, "%s/%s", output_folder, file) + 1;
    char* path = malloc(len);

    // A terminating null character is automatically appended after the content written.
    sprintf(path, "%s/%s", output_folder, file);

    return path;
}

int handle_status(Msg t, int wfd, Status s)
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error forking process");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        int callback_fd;
        char* callback_fifo = get_client_callback_filepath_by_pid(t.pid);
        callback_fd = open(callback_fifo, O_WRONLY);
        if (callback_fd == -1) {
            perror("Error opening client callback FIFO");
            free(callback_fifo);
            return 0;
        }

        returnStatus(s, callback_fd);

        free(callback_fifo);
        close(callback_fd);

        Msg m;
        m.pid = getpid();
        m.type = KILL;
        ssize_t written_bytes = write(wfd, &m, sizeof(Msg));
        if (written_bytes == -1) {
            perror("Nao consegui escrever:");
        }
        _exit(0);
    }
    return 1;
}

int log_termination(Msg t, const char* completed_path)
{
    int completed_fd = open(completed_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (completed_fd == -1) {
        perror("Error opening completed file");
        return 0;
    }

    char buf[1024];
    int seconds = t.time / 1000;
    int milliseconds = t.time % 1000;
    printf("-- ID %d: Terminou após %d.%03d seg\n", t.id, seconds, milliseconds);
    sprintf(buf, "ID %d: Terminou após %d.%03d seg\n", t.id, seconds, milliseconds);
    write(completed_fd, buf, strlen(buf));
    close(completed_fd);

    return 1;
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
        return EXIT_SUCCESS;
    }

    int scheduling_policy = FCFS; // Default is FCFS
    if (argc > 3) {
        if (strcmp(argv[3], "SJF") == 0) {
            scheduling_policy = SJF;
        }
    }

    MinHeap minq;
    Queue q;

    if (scheduling_policy == SJF) {
        initMinHeap(&minq);
    } else {
        initQueue(&q);
    }

    Status s;
    initStatus(s);

    int N = atoi(argv[2]); // Number of parallel tasks
    if (N <= 0) {
        printf("Number of parallel tasks must be greater than 0.\n");
        freeMinHeap(&minq);
        freeStatus(s);
        return EXIT_FAILURE;
    }
    int tasks_running = 0; // Number of tasks running

    mkfifo(TASK_FIFO, 0644);

    int fd = open(TASK_FIFO, O_RDONLY);
    if (fd == -1) {
        perror("Error opening FIFO for reading");
        freeMinHeap(&minq);
        freeStatus(s);
        return EXIT_FAILURE;
    }

    int wfd = open(TASK_FIFO, O_WRONLY);
    if (wfd == -1) {
        perror("Error opening FIFO for writing");
        close(fd);
        freeMinHeap(&minq);
        freeStatus(s);
        return EXIT_FAILURE;
    }

    char* completed_path = get_tmp_filepath(argv[1], "completed.txt");

    int task_id = 0;

    int r = EXIT_SUCCESS;

    Msg t;
    ssize_t br;
    while ((br = read(fd, &t, sizeof(Msg))) > 0) {

        switch (t.type) {
        // Messages from client
        case STATUS:
            handle_status(t, wfd, s);
            break;
        case SINGLE:
        case PIPELINE: {
            if (t.type == PIPELINE) {
                printf("PIPELINE: %s\n", t.command);
            }
            Bin bin;
            bin.type = t.type;
            bin.time = t.time;
            bin.file = t.command;
            bin.id = task_id++;
            if (clock_gettime(CLOCK_MONOTONIC, &bin.ts_start) == -1) {
                perror("clock_gettime");
                exit(EXIT_FAILURE);
            }

            scheduling_policy == SJF ? insert(&minq, bin) : inQueue(&q, bin);
            schedTask(s, bin);

            char* callback_fifo = get_client_callback_filepath_by_pid(t.pid);
            int callback_fd = open(callback_fifo, O_WRONLY);
            if (callback_fd == -1) {
                perror("Error opening client callback FIFO");
                free(callback_fifo);
                break;
            }

            int task_id = bin.id;
            write(callback_fd, &task_id, sizeof(int));

            free(callback_fifo);
            close(callback_fd);
        } break;
        // Messages from server
        case KILL: {
            if (waitpid(t.pid, NULL, 0) == -1) {
                perror("Error waiting for child process");
                r = EXIT_FAILURE;
                break;
            }
        } break;
        case TERMINATED: {
            // Reinvidicar waiter fork
            int status;
            if (waitpid(t.pid, &status, 0) == -1) {
                perror("Error waiting for child process");
                r = EXIT_FAILURE;
                break;
            }
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                perror("Error executing task");
                r = EXIT_FAILURE;
                break;
            }

            log_termination(t, completed_path);

            // Retirar da lista de tarefas a correr
            Bin b;
            b.id = t.id;
            b.time = t.time;
            terminateTask(s, b);
            tasks_running--;
        } break;
        }

        while (tasks_running < N && (scheduling_policy == SJF ? minq.used : q.used)) {
            Bin a;
            int taken = (scheduling_policy == SJF ? removeMin(&minq, &a) : deQueue(&q, &a));
            if (taken) {
                execTask(s, a);
                pid_t cpid = fork();
                if (cpid == -1) {
                    perror("Error forking process");
                    free(a.file);
                    exit(EXIT_FAILURE);
                } else if (cpid == 0) {
                    printf("ID %d: %s\n", a.id, a.file);
                    switch (a.type) {
                    case SINGLE:
                        mysystem(a.file, argv[1]);
                        break;
                    case PIPELINE:
                        mysystem_pipe(a.file, argv[1]);
                        break;
                    default:
                        break;
                    }

                    struct timespec ts_end;
                    if (clock_gettime(CLOCK_MONOTONIC, &ts_end) == -1) {
                        perror("clock_gettime");
                        exit(EXIT_FAILURE);
                    }

                    Msg b = {};
                    b.time = get_delta_ms(a.ts_start, ts_end);
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
    freeMinHeap(&minq);
    freeQueue(&q);
    freeStatus(s);
    free(completed_path);

    (scheduling_policy == SJF ? freeMinHeap(&minq) : freeQueue(&q));

    return r;
}
