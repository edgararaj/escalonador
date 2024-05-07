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
            if(pipe(p[i]) == -1) {
                perror("Error creating pipe:");
                _exit(EXIT_FAILURE);
            }

            pid = fork();

            if (pid == -1) {
                perror("Error in the Pipe Line:");
                _exit(EXIT_FAILURE);
            } else if (pid == 0) {

                if (dup2(mystdout, 2) == -1) {
                    perror("Error in dup2 to stderr:");
                    _exit(EXIT_FAILURE);
                }/* Move mystdout to FD 2 and verify error */

                if (close(p[i][0]) == -1) {
                    perror("Error closing pipe:");
                    _exit(EXIT_FAILURE);
                }

                if (dup2(p[i][1], 1) == -1) {
                    perror("Error in dup2 to stdout:");
                    _exit(EXIT_FAILURE);
                }

                if (close(p[i][1]) == -1) {
                    perror("Error closing pipe:");
                    _exit(EXIT_FAILURE);
                }

                exec_command(progs[i]);
                _exit(EXIT_SUCCESS);

            } else {
                if (close(p[i][1]) == -1) {
                    perror("Error closing pipe:");
                    _exit(EXIT_FAILURE);
                }
            }

        } else if (i == n - 1) {

            pid = fork();

            if (pid == -1) {
                perror("Error in the Pipe Line:");
                _exit(255);
            } else if (pid == 0) {

                if (dup2(p[i - 1][0], 0) == -1) {
                    perror("Error in dup2 to stdin:");
                    _exit(EXIT_FAILURE);
                }

                if (close(p[i - 1][0]) == -1) {
                    perror("Error closing pipe:");
                    _exit(EXIT_FAILURE);
                }

                if (dup2(mystdout, 1) == -1) {  
                    perror("Error in dup2 to stdout:");
                    _exit(EXIT_FAILURE);
                } /* Move mystdout to FD 1 and verify error */

                if (dup2(mystdout, 2) == -1) {
                    perror("Error in dup2 to stderr:");
                    _exit(EXIT_FAILURE);
                } /* Move mystdout to FD 2 and verify error */

                exec_command(progs[i]);
                _exit(EXIT_SUCCESS);
            } else {
                if (close(p[i - 1][0]) == -1) {
                    perror("Error closing pipe:");
                    _exit(EXIT_FAILURE);
                }
            }
        } else {

            if (pipe(p[i]) == -1) {
                perror("Error creating pipe:");
                _exit(EXIT_FAILURE);
            } // create the pipe and verify error

            pid = fork();
            if (pid == -1) {
                perror("Error in the Pipe Line:");
                _exit(EXIT_FAILURE);
            } else if (pid == 0) {
                if (dup2(mystdout, 2) == -1) {
                    perror("Error in dup2 to stderr:");
                    _exit(EXIT_FAILURE);
                } /* Move mystdout to FD 2 and verify errpr*/

                if (dup2(p[i - 1][0], 0) == -1) {
                    perror("Error in dup2 to stdin:");
                    _exit(EXIT_FAILURE);
                }

                if (close(p[i - 1][0]) == -1) {
                    perror("Error closing pipe:");
                    _exit(EXIT_FAILURE);
                }

                if (close(p[i][0]) == -1) {
                    perror("Error closing pipe:");
                    _exit(EXIT_FAILURE);
                }

                if (dup2(p[i][1], 1) == -1) {
                    perror("Error in dup2 to stdout:");
                    _exit(EXIT_FAILURE);
                }

                if (close(p[i][1]) == -1) {
                    perror("Error closing pipe:");
                    _exit(EXIT_FAILURE);
                }

                exec_command(progs[i]);
                _exit(EXIT_SUCCESS);
            } else {

                if (close(p[i - 1][0]) == -1) {
                    perror("Error closing pipe:");
                    _exit(EXIT_FAILURE);
                }
                if (close(p[i][1]) == -1) {
                    perror("Error closing pipe:");
                    _exit(EXIT_FAILURE);
                }
            }
        }
    }
    int status;
    for (i = 0; i < n; i++) {
        wait(&status);

        if (WIFEXITED(status) && WEXITSTATUS(status)) {
            perror("Error in the status collection:");
            _exit(EXIT_FAILURE);
        }
    }
}

void mysystem_pipe(char* args, int mystdout)
{
    char* progs[TASK_COMMAND_SIZE];
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
