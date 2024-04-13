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
    int len = snprintf(NULL, 0, "/tmp/escalonador_%d", getpid());
    char* path = malloc(len);

    // A terminating null character is automatically appended after the content written.
    sprintf(path, "/tmp/escalonador_%d", getpid());

    return path;
}

int main(int argc, char* argv[])
{
    if (argc > 1 && strcmp(argv[1], "status") == 0) {
        mkfifo(TASK_FIFO, 0644);
        char* callback_fifo = get_callback_filepath();
        mkfifo(callback_fifo, 0644);

        int fd = open(TASK_FIFO, O_WRONLY);

        Msg t;
        t.pid = getpid();
        t.type = STATUS;

        write(fd, &t, sizeof(Msg));
        close(fd);

        int callback_fd = open(callback_fifo, O_RDONLY);
        struct s task;
        int prev_sts = STS_UNKNOWN;
        while (read(callback_fd, &task, sizeof(struct s)) > 0) {
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

        close(callback_fd);
        unlink(callback_fifo);
        free(callback_fifo);

    } else if (argc > 4 && strcmp(argv[1], "execute") == 0) {
        if (strcmp(argv[3], "-u") == 0) {
            printf("Single execution\n");

            int time = atoi(argv[2]);
            char* cmd = argv[4];

            printf("Time: %d, Command: %s\n", time, cmd);

            mkfifo(TASK_FIFO, 0644);
            char* callback_fifo = get_callback_filepath();
            mkfifo(callback_fifo, 0644);

            int fd = open(TASK_FIFO, O_WRONLY);
            Msg t;
            t.time = time;
            t.pid = getpid();
            strncpy(t.command, cmd, TASK_COMMAND_SIZE);
            t.type = SINGLE;

            write(fd, &t, sizeof(Msg));
            close(fd);

            int callback_fd = open(callback_fifo, O_RDONLY);
            int task_pid;
            read(callback_fd, &task_pid, sizeof(int));

            printf("Task PID: %d\n", task_pid);

            close(callback_fd);
            unlink(callback_fifo);
            free(callback_fifo);
        } else if (strcmp(argv[3], "-p") == 0) {
            printf("Pipe execution\n");
        } else {
            printf("Invalid option %s\n", argv[3]);
        }
    } else {
        printf("Uso:\n");
        printf("\t%s execute 100 -u 'prog-a arg-1 (...) arg-n'\n", argv[0]);
        printf("\t%s execute 3000 -p 'prog-a arg-1 (...) | prog-b arg-1 (...)'\n", argv[0]);
        printf("\t%s status\n", argv[0]);
    }
}