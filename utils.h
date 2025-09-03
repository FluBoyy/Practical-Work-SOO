#define TAMANHO_STRING 100
#define MAX_UTILIZADORES 5
#define FIFO_MOTOR "PIPE_MOTOR"
#define FIFO_JOGO "PIPE_JOGO_%d"
#define FIFO_ATUALIZA "PIPE_ATUALIZAA"
#define FIFO_ATUALIZA2 "PIPE_ATUALIZA2"
#include <pthread.h>
#include <semaphore.h>

pthread_mutex_t lock;

typedef struct utilizador{
    char nomeJogador[50];
    int pid;
    int x,y;
    char comando[TAMANHO_STRING];
    int resposta;
    char labirinto[16][40];
    char mensagem[TAMANHO_STRING];
}Utilizador;

Utilizador newUtilizador;

int numUtilizadores = 0;

