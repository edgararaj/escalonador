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
#include "time.h"

char* get_output_path(const char* output_folder, int pid)
{
    int pid_len = snprintf(NULL, 0, "%d", pid) + 1;
    char* path = malloc(strlen(output_folder) + pid_len + 5);
    sprintf(path, "%s/%d.txt", output_folder, pid);
    return path;
}

int mysystem(const char* command, int mystdout)
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
    if (cpid == -1) {
        perror("Error forking process");
        exit(EXIT_FAILURE);
    } else if (cpid == 0) {
        dup2(mystdout, 1); /* Move mystdout to FD 1 */
        dup2(mystdout, 2); /* Move mystdout to FD 2 */

        execvp(exec_args[0], exec_args);

        _exit(-1);
    } else if (cpid > 0) {
        int status;
        if (wait(&status) > 0) {
            if (WIFEXITED(status) && WEXITSTATUS(status) != 255) {
                res = WEXITSTATUS(status);
            }
        }
    }

    free(tofree);

    return res;
}
