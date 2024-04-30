#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mysystem_pipe.h"
#include "orchestrator.h"

#define READ 0
#define WRITE 1

#define MAX_COMMANDS 30

int exec_command(char* arg)
{

    char* exec_args[MAX_COMMANDS];

    char* string;
    int exec_ret = 0;
    int i = 0;

    char* command = strdup(arg);

    string = strtok(command, " ");

    while (string != NULL) {
        exec_args[i] = string;
        string = strtok(NULL, " ");
        i++;
    }
    exec_args[i] = NULL;

    exec_ret = execvp(exec_args[0], exec_args);

    return exec_ret;
}

void pipeLine(char* progs[], int n, int mystdout)
{

    int p[n - 1][2];
    int i;
    pid_t pid;

    for (i = 0; i < n; i++) {
        if (i == 0) {
            pipe(p[i]);
            pid = fork();
            if (pid == -1) {
                perror("Error in the Pipe Line:");
                _exit(255);
            } else if (pid == 0) {

                
                dup2(mystdout, 2); /* Move mystdout to FD 2 */
                close(p[i][0]);

                dup2(p[i][1], 1);
                close(p[i][1]);

                exec_command(progs[i]);
                _exit(0);
            } else {
                close(p[i][1]);
            }

        } else if (i == n - 1) {
            pid = fork();
            if (pid == -1) {
                perror("Error in the Pipe Line:");
                _exit(255);
            } else if (pid == 0) {

                dup2(p[i - 1][0], 0);
                close(p[i - 1][0]);

               
                dup2(mystdout, 1); /* Move mystdout to FD 1 */
                dup2(mystdout, 2); /* Move mystdout to FD 2 */

                exec_command(progs[i]);
                _exit(0);
            } else {
                close(p[i - 1][0]);
            }
        } else {

            pipe(p[i]); // create the pipe

            pid = fork();
            if (pid == -1) {
                perror("Error in the Pipe Line:");
                _exit(255);
            } else if (pid == 0) {
                dup2(mystdout, 2); /* Move mystdout to FD 2 */

                dup2(p[i - 1][0], 0);
                close(p[i - 1][0]);

                close(p[i][0]);
                dup2(p[i][1], 1);
                close(p[i][1]);

                exec_command(progs[i]);
                _exit(0);
            } else {
                close(p[i - 1][0]);
                close(p[i][1]);
            }
        }
    }
    int status;
    for (i = 0; i < n; i++) {
        wait(&status);

        if (WIFEXITED(status) && WEXITSTATUS(status)) {
            perror("Error in the status collection:");
            _exit(255);
        }
    }
}


void mysystem_pipe(char* args, const char* output_folder)
{
    int mystdout;
    // criar ficheiro de output
    {
        char* path = get_output_path(output_folder);
        mystdout = open(path, O_CREAT | O_WRONLY, 0644);
        free(path);
    }

    char* progs[301];
    int i = 0;
    char* string;
    char* command = strdup(args);
    string = strtok(command, "|");

    while (string != NULL) {
        progs[i] = string;
        string = strtok(NULL, "|");
        i++;
    }
    progs[i] = NULL;
    pipeLine(progs, i, mystdout);
}
