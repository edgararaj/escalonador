typedef struct {
    char* file;
    int time;
} Bin;

// Queue
typedef struct {
    int ini, size, used;
    Bin* args;
} Queue;

void initQueue(Queue* q);
void inQueue(char* ficheiro, int tempo, Queue* q);
void reallocQueue(Queue* q);
int deQueue(Queue* q, Bin* a);
void freeQueue(Queue* q);

// Min-Heap

typedef struct {
    int size, used;
    Bin* args;
} MinHeap;

void initMinHeap(MinHeap* m);
void insert(char* file, int time, MinHeap* m);
int removeMin(MinHeap* m, Bin* a);
void bubbleUp(MinHeap* m, int i);
void bubbleDown(MinHeap* m, int i);
void freeMinHeap(MinHeap* m);