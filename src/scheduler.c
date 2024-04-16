#include "queues.h"
#include <stdlib.h>
#include <string.h>

void initQueue(Queue* q)
{
    q->ini = q->used = 0;
    q->size = 8;
    q->args = malloc(sizeof(Bin) * q->size);
}

void freeQueue(Queue* q)
{
    for (int i = 0; i < q->used; i++) {
        free(q->args[i].file);
    }
    free(q->args);
}
void reallocQueue(Queue* q)
{
    Bin* new = malloc(sizeof(Bin) * q->size * 2);
    int i, j;
    for (j = 0, i = q->ini; j < q->used; j++) {
        new[j].file = q->args[i].file;
        new[j].id = q->args[i].id;
        new[j].time = q->args[i].time;

        q->args[i].file = NULL;
        i = (i + 1) % q->used;
    }
    free(q->args);
    q->args = new;
    q->ini = 0;
    q->size *= 2;
}

void inQueue(Queue* q, Bin a)
{
    if (q->size == q->used) {
        reallocQueue(q);
    }
    int pos = (q->ini + q->used) % q->size;

    q->args[pos].file = strdup(a.file);
    q->args[pos].id = a.id;
    q->args[pos].time = a.time;

    q->used++;
}

int deQueue(Queue* q, Bin* a)
{
    int i = 0;
    if (q->used > 0) {
        int pos = q->ini;

        a->file = q->args[pos].file;
        a->id = q->args[pos].id;
        a->time = q->args[pos].id;
        q->args[pos].file = NULL;

        i = 1;
        q->ini = (q->ini + 1) % q->size;
        q->used--;
    }
    return i;
}

void initMinHeap(MinHeap* m)
{
    m->size = 8;
    m->used = 0;
    m->args = malloc(sizeof(Bin) * m->size);
}

void insert(MinHeap* m, Bin a)
{
    if (m->size == m->used) {
        m->size *= 2;
        // falta verificar o realloc...
        m->args = realloc(m->args, sizeof(Bin) * m->size);
    }

    m->args[m->used].time = a.time;
    m->args[m->used].file = strdup(a.file);
    m->args[m->used].id = a.id;

    bubbleUp(m, m->used);
    m->used++;
}

void bubbleUp(MinHeap* m, int i)
{
    int x = (i - 1) / 2, y;
    char* tmp;
    while (x >= 0 && m->args[i].time < m->args[x].time) {
        tmp = m->args[i].file;
        m->args[i].file = m->args[x].file;
        m->args[x].file = tmp;
        y = m->args[i].time;
        m->args[i].time = m->args[x].time;
        m->args[x].time = y;
        y = m->args[i].id;
        m->args[i].id = m->args[x].id;
        m->args[x].id = y;

        i = x;
        x = (i - 1) / 2;
    }
}

int removeMin(MinHeap* m, Bin* a)
{
    int i = 0;
    if (m->used) {
        a->file = m->args[0].file;
        a->time = m->args[0].time;
        a->id = m->args[0].id;
        m->used--;
        m->args[0].file = m->args[m->used].file;
        m->args[0].time = m->args[m->used].time;
        m->args[0].id = m->args[m->used].id;
        m->args[m->used].file = NULL;
        
        bubbleDown(m, 0);
        i = 1;
    }

    return i;
}
void bubbleDown(MinHeap* m, int i)
{
    int left = 2 * i + 1, flag = 1, min;
    while (flag && left < m->used) {
        min = (m->args[i].time < m->args[left].time) ? i : left;
        if (left + 1 < m->used) {
            min = (m->args[min].time < m->args[left + 1].time) ? min : left + 1;
        }
        if (min == i) {
            flag = 0;
        } else {
            i = min;
            left = 2 * i + 1;

            int y;
            char* tmp;

            tmp = m->args[i].file;
            m->args[i].file = m->args[min].file;
            m->args[min].file = tmp;
            y = m->args[i].time;
            m->args[i].time = m->args[min].time;
            m->args[min].time = y;
            y = m->args[i].id;
            m->args[i].id = m->args[min].id;
            m->args[min].id = y;
        }
    }
}

void freeMinHeap(MinHeap* m)
{
    int i;
    for (i = 0; i < m->used; i++) {
        free(m->args[i].file);
    }
    free(m->args);
}