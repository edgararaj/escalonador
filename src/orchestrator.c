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

char* get_output_path(const char* output_folder)
{
    int pid = getpid();
    // create path to output file consisting of <output_folder>/<pid>.txt
    // calculate length of pid
    int pid_len = snprintf(NULL, 0, "%d", pid);
    char* path = malloc(strlen(output_folder) + pid_len + 5);

    // A terminating null character is automatically appended after the content written.
    sprintf(path, "%s/%d.txt", output_folder, pid);

    return path;
}

int mysystem(const char* command, const char* output_folder)
{
    int res = -1;

    // Estamos a assumir numero maximo de argumentos
    // isto teria de ser melhorado com realloc por exemplo
    char* exec_args[20];
    char *string, *cmd, *tofree;
    int i = 0;
    tofree = cmd = strdup(command);
    while ((string = strsep(&cmd, " ")) != NULL) {
        exec_args[i] = string;
        i++;
    }
    exec_args[i] = NULL;

    int mystdout;
    // criar ficheiro de output
    {
        char* path = get_output_path(output_folder);
        mystdout = open(path, O_CREAT | O_WRONLY, 0644);
        free(path);
    }

    close(1); /* Close original stdout */
    dup2(mystdout, 1); /* Move mystdout to FD 1 */
    close(2); /* Close original stderr */
    dup2(mystdout, 2); /* Move mystdout to FD 2 */

    execvp(exec_args[0], exec_args);

    _exit(-1);
}

void initQueue(Queue* q)
{
    q->ini = q->uti = 0;
    q->tam = 1;
    q->args = malloc(sizeof(Bin) * q->tam);
}

void freeQueue(Queue* q)
{
    for (int i = 0; i < q->uti; i++) {
        free(q->args[i].ficheiro);
    }
    free(q->args);
}

void reallocQueue(Queue* q)
{
    Bin* novo = malloc(sizeof(Bin) * q->tam * 2);
    int i, j;
    for (j = 0, i = q->ini; j < q->uti; j++, i++) {
        novo[j].tempo = q->args[i].tempo;
        novo[j].ficheiro = q->args[i].ficheiro;
        q->args[i].ficheiro = NULL;
    }
    free(q->args);
    q->args = novo;
    q->ini = 0;
    q->tam *= 2;
}

void inQueue(char* ficheiro, int tempo, Queue* q)
{
    if (q->tam == q->uti) {
        reallocQueue(q);
    }
    int pos = (q->ini + q->uti) % q->tam;

    q->args[pos].ficheiro = strdup(ficheiro);
    q->args[pos].tempo = tempo;

    q->uti++;
}

int deQueue(Queue* q, Bin* a)
{
    int i = 0;
    if (q->uti > 0) {
        int pos = q->ini;

        a->ficheiro = q->args[pos].ficheiro;
        q->args[pos].ficheiro = NULL;
        a->tempo = q->args[pos].tempo;

        i = 1;
        q->ini = (q->ini + 1) % q->tam;
        q->uti--;
    }
    return i;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Uso: %s <output_folder> <parallel-tasks> <sched-policy>\n", argv[0]);
        printf("Argumentos:\n");
        printf("\toutput-folder: pasta onde são guardados os ficheiros com o "
               "output de tarefas executadas.\n");
        printf("\tparallel-tasks: número de tarefas que podem ser executadas em "
               "paralelo.\n");
        printf("\tsched-policy: identificador da política de escalonamento, caso o "
               "servidor suporte várias políticas.\n");

        return 1;
    }

    Queue q;
    initQueue(&q);

    // Populate the queue with tasks
    inQueue("test/hello 4", 10, &q);
    inQueue("test/void 3", 10, &q);
    inQueue("test/hello 2", 10, &q);
    inQueue("test/void 1", 10, &q);

    // Parallel execution
    int N = atoi(argv[2]); // Number of parallel tasks
    pid_t pids[N]; // Array to store child process IDs
    char* cmds[N]; // Array to store commands
    int i = 0;

    while (q.uti) {
        Bin a;
        if (deQueue(&q, &a)) {
            pid_t cpid = fork();
            if (cpid == 0) {
                // Child process
                mysystem(a.ficheiro, argv[1]);
                _exit(0);
            } else if (cpid > 0) {
                // Parent process
                pids[i] = cpid; // Store child process ID
                cmds[i] = a.ficheiro; // Store command
                printf("PID %d: %s %d\n", cpid, a.ficheiro, i + 1);
            }
            i++;
        }
    }
    struct timespec ts_start, ts_end;

    if (clock_gettime(CLOCK_MONOTONIC, &ts_start) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    // Wait for all child processes to finish
    while (1) {
        int status;

        int rpid = waitpid(-1, &status, 0);

        if (rpid == -1) {
            break;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &ts_end) == -1) {
            perror("clock_gettime");
            exit(EXIT_FAILURE);
        }

        printf("PID %d: Terminou após %jd.%03ld seg\n", rpid, (intmax_t)(ts_end.tv_sec - ts_start.tv_sec), (ts_end.tv_nsec - ts_start.tv_nsec) / 1000000);
    }

    freeQueue(&q);
    return 0;
}