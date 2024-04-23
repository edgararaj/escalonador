#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include <sys/types.h>

#define TASK_COMMAND_SIZE 301
#define TASK_FIFO "/tmp/escalonador"

// Message structure
// SINGLE and PIPELINE messages are sent by the client
// pid (pid of client) | time (expected execution time) | command

// STATUS messages are sent by the client
// ---

// TERMINATED messages are sent by the server
// pid (pid of waiter fork) | id (id of the task) | time (termination time) | command

// KILL messages are sent by the server
// pid (pid of status fork)

typedef enum {
    // messages from the client
    SINGLE,
    PIPELINE,
    STATUS,

    // messages from the server to itself
    KILL,
    TERMINATED
} Type;

typedef struct {
    pid_t pid;
    int id;
    int time;
    char command[TASK_COMMAND_SIZE];
    Type type;
} Msg;

#endif