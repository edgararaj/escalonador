#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "queues.h"

#define NANOSECONDS_IN_SECOND 1000000000L
#define NANOSECONDS_IN_MILLISECOND 1000000L

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

int mysystem(const char* command, const char* output_folder)
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

    // Populate the queue with tasks
    insert("test/hello 4", 4, &q);
    insert("test/void 3", 3, &q);
    insert("test/hello 2", 2, &q);
    insert("test/void 1", 1, &q);

    // Parallel execution
    int N = atoi(argv[2]); // Number of parallel tasks
    int i = 0;

    char* completed_path = get_tmp_filepath(argv[1], "completed.txt");

    struct timespec ts_start, ts_end;

    if (clock_gettime(CLOCK_MONOTONIC, &ts_start) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    while (1) {
        while (q.used && i < N) {
            Bin a;
            if (removeMin(&q, &a)) {
                pid_t cpid = fork();
                if (cpid == 0) {
                    mysystem(a.file, argv[1]);
                    _exit(0);
                } else if (cpid > 0) {
                    printf("PID %d: %s (%d)\n", cpid, a.file, i + 1);
                    i++;
                }
            }
        }

        int status;
        int rpid = waitpid(-1, &status, 0);
        if (rpid == -1) {
            break;
        }

        i--;

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

        {
            int completed_fd = open(completed_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
            char buf[1024];
            sprintf(buf, "PID %d: Terminou após %ld.%03ld seg\n", rpid, seconds, milliseconds);
            write(completed_fd, buf, strlen(buf));
            close(completed_fd);
        }
    }

    free(completed_path);
    freeMinHeap(&q);
    return 0;
}