#ifndef QUEUES_H
#define QUEUES_H

// precisa do id
typedef struct {
    char* file;
    int time, id;
} Bin;

// Queue
typedef struct {
    int ini, size, used;
    Bin* args;
} Queue;

void initQueue(Queue* q);
void inQueue(Queue* q, Bin a);
void reallocQueue(Queue* q);
int deQueue(Queue* q, Bin* a);
void freeQueue(Queue* q);

// Min-Heap

typedef struct {
    int size, used;
    Bin* args;
} MinHeap;

void initMinHeap(MinHeap* m);
void insert(MinHeap* m, Bin a);
int removeMin(MinHeap* m, Bin* a);
void bubbleUp(MinHeap* m, int i);
void bubbleDown(MinHeap* m, int i);
void freeMinHeap(MinHeap* m);

#endif