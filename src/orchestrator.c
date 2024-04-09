#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    char* ficheiro;
    int tempo;
} Bin;

typedef struct {
    int ini, tam, uti;
    Bin* args;
} Queue;

char* get_output_path(const char* output_folder) {
    int pid = getpid();
    int pid_len = snprintf(NULL, 0, "%d", pid);
    char* path = malloc(strlen(output_folder) + pid_len + 5);
    sprintf(path, "%s/%d.txt", output_folder, pid);
    return path;
}

int mysystem(const char* command, const char* output_folder) {
    char* exec_args[20];
    char *string, *cmd, *tofree;
    int i = 0;
    tofree = cmd = strdup(command);
    while ((string = strsep(&cmd, " ")) != NULL) {
        exec_args[i++] = string;
    }
    exec_args[i] = NULL;

    int mystdout;
    char* path = get_output_path(output_folder);
    mystdout = open(path, O_CREAT | O_WRONLY, 0644);
    free(path); // Free allocated path

    close(1); // Close original stdout
    dup2(mystdout, 1); // Duplicate mystdout to stdout
    close(2); // Close original stderr
    dup2(mystdout, 2); // Duplicate mystdout to stderr
    close(mystdout); // Close the original file descriptor

    execvp(exec_args[0], exec_args);

    // Free allocated command and exec_args before exiting
    free(tofree);
    _exit(-1);
}


void initQueue(Queue* q) {
    q->ini = q->uti = 0;
    q->tam = 1;
    q->args = malloc(sizeof(Bin) * q->tam);
}

void freeQueue(Queue* q) {
    for (int i = 0; i < q->uti; i++) {
        free(q->args[i].ficheiro);
    }
    free(q->args);
}

void reallocQueue(Queue* q) {
    Bin* novo = malloc(sizeof(Bin) * q->tam * 2);
    for (int j = 0, i = q->ini; j < q->uti; j++, i++) {
        novo[j].tempo = q->args[i].tempo;
        novo[j].ficheiro = q->args[i].ficheiro;
        q->args[i].ficheiro = NULL;
    }
    free(q->args);
    q->args = novo;
    q->ini = 0;
    q->tam *= 2;
}

void inQueue(char* ficheiro, int tempo, Queue* q) {
    if (q->tam == q->uti) {
        reallocQueue(q);
    }
    int pos = (q->ini + q->uti) % q->tam;
    q->args[pos].ficheiro = strdup(ficheiro);
    q->args[pos].tempo = tempo;
    q->uti++;
}

int deQueue(Queue* q, Bin* a) {
    if (q->uti > 0) {
        int pos = q->ini;
        *a = q->args[pos]; // Copy the entire struct
        q->args[pos].ficheiro = NULL; // Prevent double free
        q->ini = (q->ini + 1) % q->tam;
        q->uti--;
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) { // Ensure at least 3 arguments are provided
        printf("Uso: %s <output_folder> <parallel-tasks> <sched-policy>\n", argv[0]);
        return 1;
    }

    Queue q;
    initQueue(&q);
    // Populate the queue with tasks
    inQueue("test/hello 4", 10, &q);
    inQueue("test/void 3", 10, &q);
    inQueue("test/hello 2", 10, &q);
    inQueue("test/void 1", 10, &q);

    int N = atoi(argv[2]); // Number of parallel tasks
    if (N <= 0) {
        printf("Number of parallel tasks must be greater than 0.\n");
        freeQueue(&q);
        return 1;
    }

    pid_t pids[N]; // Array to store child process IDs
    int i = 0;

    while (q.uti) {
        Bin a;
        if (deQueue(&q, &a)) {
            pid_t cpid = fork();
            if (cpid == 0) {
                mysystem(a.ficheiro, argv[1]);
                free(a.ficheiro); // Free the command string in child process
                _exit(0);
            } else if (cpid > 0) {
                pids[i] = cpid; // Store child process ID
                printf("PID %d: %s %d\n", cpid, a.ficheiro, i + 1);
                free(a.ficheiro); // Free the command string in parent process
            }
            i++;
        }
    }

    // Wait for all child processes to finish
    for (int j = 0; j < i; j++) {
        waitpid(pids[j], NULL, 0);
    }

    freeQueue(&q);
    return 0;
}
