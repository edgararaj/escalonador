#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

typedef enum {
    SINGLE,
    PIPELINE
} Type;

#define TASK_COMMAND_SIZE 300
#define TASK_FIFO "/tmp/escalonador"

typedef struct {
    int client_pid;
    int time;
    char command[TASK_COMMAND_SIZE];
    Type type;
} Task;

#endif