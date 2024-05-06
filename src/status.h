#ifndef STATUS_H
#define STATUS_H

#include "queues.h"

typedef enum {
    STS_UNKNOWN = -2,
    STS_SCHEDULED = -1,
    STS_EXECUTING = 0,
    STS_TERMINATED = 1
} Sts;

struct s {
    int id, time;
    Sts status;
    char file[TASK_COMMAND_SIZE];
};
typedef struct st {
    struct s data;
    struct st* next;
}* St;
typedef struct aux {
    St s, e;
} Aux;
typedef Aux* Status[3];
// mudar de int para o tipo do pid
void returnStatus(Status a, int fd, const char* completed_bin_path);
void initStatus(Status a);
void freeStatus(Status a);
void schedTask(Status a, Bin b);
void execTask(Status a, Bin b);
void terminateTask(Status a, Bin b, const char* completed_bin_path);

#endif