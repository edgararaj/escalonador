#include "status.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void initStatus(Status a)
{
    for (int i = 0; i < 3; i++) {
        a[i] = malloc(sizeof(Aux)); // Allocate memory for each Aux struct
        if (a[i] == NULL) {
            // Handle allocation failure if needed
            exit(EXIT_FAILURE);
        }
        a[i]->s = NULL;
        a[i]->e = NULL;
    }
}

void freeStatus(Status a)
{
    int i;
    St x, tmp;
    for (i = 0; i < 3; i++) {
        x = a[i]->s;
        while (x) {
            tmp = x;
            x = x->next;
            free(tmp);
        }
        free(a[i]);
    }
}

void schedTask(Status a, Bin b)
{
    St x = malloc(sizeof(struct st));
    x->next = NULL;
    strcpy(x->data.file, b.file);
    x->data.status = -1;
    x->data.time = b.time;
    x->data.id = b.id;
    if (!a[0]->s) {
        a[0]->s = a[0]->e = x;
    } else {
        a[0]->e->next = x;
        a[0]->e = x;
    }
}

void execTask(Status a, Bin b)
{
    St x;
    if (a[0]->e == a[0]->s) {
        x = a[0]->e;
        a[0]->e = a[0]->s = NULL;

    } else if (a[0]->s->data.id == b.id) {
        x = a[0]->s;
        a[0]->s = a[0]->s->next;
    } else if (a[0]->e->data.id == b.id) {
        x = a[0]->e;
        St y;
        for (y = a[0]->s; y->next != x; y = y->next)
            ;
        a[0]->e = y;
    } else {
        St prev;
        for (x = a[0]->s; x && x->data.id != b.id; x = x->next) {
            prev = x;
        }
        if (x == NULL) {
            perror("x == NULL");
        }
        prev->next = x->next;
    }
    x->next = NULL;
    x->data.status = 0;
    if (!a[1]->s) {
        a[1]->s = a[1]->e = x;
    } else {
        a[1]->e->next = x;
        a[1]->e = x;
    }
}

void terminateTask(Status a, Bin b)
{
    St x;
    if (a[1]->e == a[1]->s) {
        x = a[1]->e;
        a[1]->e = a[1]->s = NULL;

    } else if (a[1]->s->data.id == b.id) {
        x = a[1]->s;
        a[1]->s = a[1]->s->next;
    } else if (a[1]->e->data.id == b.id) {
        x = a[1]->e;
        St y;
        for (y = a[1]->s; y->next != x; y = y->next)
            ;
        a[1]->e = y;
    } else {
        St prev;
        for (x = a[1]->s; x->data.id != b.id; x = x->next) {
            prev = x;
        }
        prev->next = x->next;
    }
    x->next = NULL;
    x->data.status = 1;
    x->data.time = b.time;
    if (!a[2]->s) {
        a[2]->s = a[2]->e = x;
    } else {
        a[2]->e->next = x;
        a[2]->e = x;
    }
}

void returnStatus(Status a, int fd)
{
    // TODO: Fazer fork, por causa do enunciado!!!
    St x;
    int i;
    for (i = 0; i < 3; i++) {
        for (x = a[i]->s; x; x = x->next) {
            write(fd, &(x->data), sizeof(struct s));
        }
    }
}