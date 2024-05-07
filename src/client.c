#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "orchestrator.h"
#include "status.h"

char* get_callback_filepath()
{
    // create path to output file consisting of <output_folder>/<file>
    // calculate length of <file>
    int len = snprintf(NULL, 0, "/tmp/escalonador_%d", getpid()) + 1;
    char* path = malloc(len);

    if(path == NULL){
        printf("Debug: Error in malloc");
        return NULL;
    }

    // A terminating null character is automatically appended after the content written.
    sprintf(path, "/tmp/escalonador_%d", getpid());

    return path;
}

int main(int argc, char* argv[])
{
    if (argc > 1 && strcmp(argv[1], "status") == 0) {
        if (mkfifo(TASK_FIFO, 0644) == -1) {
            perror("Error making the FIFO:");
            exit(EXIT_FAILURE);
        }

        char* callback_fifo = get_callback_filepath();

        if (mkfifo(callback_fifo, 0644) == -1) {
            perror("Error making the FIFO:");
            exit(EXIT_FAILURE);
        }

        int fd = open(TASK_FIFO, O_WRONLY);
        if (fd == -1) {
            perror("Error opening FIFO for writing:");
            exit(EXIT_FAILURE);
        }

        Msg t;
        t.pid = getpid();
        t.type = STATUS;

        if (write(fd, &t, sizeof(Msg)) == -1) {
            perror("Error writing to FIFO:");
            close(fd);
            exit(EXIT_FAILURE);
        }

        if (close(fd) == -1){
            perror("Error closing FIFO:");
            exit(EXIT_FAILURE);

        }

        int callback_fd = open(callback_fifo, O_RDONLY);
        if (callback_fd == -1) {
            perror("Error opening callback FIFO for reading:");
            exit(EXIT_FAILURE);
        }

        struct s task;
        int prev_sts = STS_UNKNOWN;
        ssize_t rb;
        while ((rb = read(callback_fd, &task, sizeof(struct s)) > 0) > 0) {
            if (task.status != prev_sts) {
                switch (task.status) {
                case STS_SCHEDULED:
                    printf("Scheduled:\n");
                    break;
                case STS_EXECUTING:
                    printf("Executing:\n");
                    break;
                case STS_TERMINATED:
                    printf("Terminated:\n");
                    break;
                default:
                    break;
                }
                prev_sts = task.status;
            }
            if (task.status == STS_TERMINATED) {
                printf("%d: %s (%dms)\n", task.id, task.file, task.time);
            } else {
                printf("%d: %s\n", task.id, task.file);
            }
        }

        if(rb == -1){
            perror("Error reading from FIFO:");
            exit(EXIT_FAILURE);
        }

        if (prev_sts == STS_UNKNOWN) {
            printf("No tasks\n");
        }

        if (close(callback_fd) == -1) {
            perror("Error closing callback_fd:");
            exit(EXIT_FAILURE);
        }

        if (unlink(callback_fifo) == -1) {
            perror("Error in the unlink:");
            exit(EXIT_FAILURE);
        }
        free(callback_fifo);
    } else if (argc > 4 && strcmp(argv[1], "execute") == 0) {
        if (strcmp(argv[3], "-u") == 0 || strcmp(argv[3], "-p") == 0) {

            int time = atoi(argv[2]);
            char* cmd = argv[4];

            printf("Time: %d, Command: %s\n", time, cmd);

            if (mkfifo(TASK_FIFO, 0644) == -1) {
                perror("Error making the TASK_FIFO:");
                exit(EXIT_FAILURE);
            }

            char* callback_fifo = get_callback_filepath();

            if (mkfifo(callback_fifo, 0644) == -1) {
                perror("Error making the callback_fifo:");
                exit(EXIT_FAILURE);
            }

            int fd = open(TASK_FIFO, O_WRONLY);
            if (fd == -1) {
                perror("Error opening FIFO for writing");
                exit(EXIT_FAILURE);
            }

            Msg t;
            t.time = time;
            if (strcmp(argv[3], "-p") == 0) {
                printf("Pipe execution\n");
                t.type = PIPELINE;
            } else {
                printf("Single execution\n");
                t.type = SINGLE;
            }
            t.pid = getpid();
            strncpy(t.command, cmd, TASK_COMMAND_SIZE);
            t.command[TASK_COMMAND_SIZE - 1] = '\0';

            if (write(fd, &t, sizeof(Msg)) == -1) {
                perror("Error writing to FIFO:");
                close(fd);
                exit(EXIT_FAILURE);
            }
            if (close(fd) == -1) {
                perror("Error closing FIFO:");
                exit(EXIT_FAILURE);
            }

            int callback_fd = open(callback_fifo, O_RDONLY);
            if (callback_fd == -1) {
                perror("Error opening callback FIFO for reading");
                exit(EXIT_FAILURE);
            }

            int task_pid;
            if (read(callback_fd, &task_pid, sizeof(int)) == -1) {
                perror("Error reading from callback FIFO");
                close(callback_fd);
                exit(EXIT_FAILURE);
            }

            printf("Task ID: %d\n", task_pid);

            if (close(callback_fd) == -1) {
                perror("Error closing callback_fd:");
                exit(EXIT_FAILURE);
            }
            if (unlink(callback_fifo) == -1) {
                perror("Error in the unlink:");
                exit(EXIT_FAILURE);
            }
            free(callback_fifo);
        } else {
            printf("Invalid option %s\n", argv[3]);
        }
    } else {
        printf("Uso:\n");
        printf("\t%s execute 100 -u 'prog-a arg-1 (...) arg-n'\n", argv[0]);
        printf("\t%s execute 3000 -p 'prog-a arg-1 (...) | prog-b arg-1 (...)'\n", argv[0]);
        printf("\t%s status\n", argv[0]);
    }
    return 0;
}