#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

typedef enum {
    // messages from the client
    SINGLE,
    PIPELINE,

    // messages from the server to itself
    TERMINATED
} Type;

#define TASK_COMMAND_SIZE 300
#define TASK_FIFO "/tmp/escalonador"

typedef struct {
    int pid;
    int time;
    char command[TASK_COMMAND_SIZE];
    Type type;
} Msg;

#endif