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

    char* exec_args[TASK_COMMAND_SIZE];
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
