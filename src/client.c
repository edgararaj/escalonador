#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "orchestrator.h"

int main(int argc, char* argv[])
{
    if (argc > 1 && strcmp(argv[1], "status") == 0) {
        printf("Status\n");
    } else if (argc > 4 && strcmp(argv[1], "execute") == 0) {
        if (strcmp(argv[3], "-u") == 0) {
            printf("Single execution\n");

            int time = atoi(argv[2]);
            char* cmd = argv[4];

            printf("Time: %d, Command: %s\n", time, cmd);

            mkfifo(TASK_FIFO, 0644);

            int fd = open(TASK_FIFO, O_WRONLY);
            printf("fifo opened for writing\n");

            Task t;
            t.time = time;
            strncpy(t.command, cmd, TASK_COMMAND_SIZE);
            t.type = SINGLE;

            write(fd, &t, sizeof(Task));

            close(fd);
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