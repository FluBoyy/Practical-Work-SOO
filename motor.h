#ifndef motor_h
#define motor_h
#include "utils.h"
typedef struct motor{
    char nomeJogador[50];
    int pid;
    int x,y;
    char comando[TAMANHO_STRING];
    int resposta;
    char labirinto[16][40];
    char mensagem[TAMANHO_STRING];
} Motor;
Motor m;

typedef struct infoJogadores{
    char nomeJogador[TAMANHO_STRING];
    int pid;
}InfoJogadores;
InfoJogadores arrayJogadores[MAX_UTILIZADORES];

void *thread_enviaCordeadas();
bool valida(WINDOW *janelaComandos, char Buffer[200], int PID[2]);
void janelas(WINDOW *janela, int x);
void carregarLabirinto(WINDOW *janela, const char *nomeArquivo,char labirinto[16][40]);
void *thread_funcao(void *arg);
void recebe(int abrirFIFO_JOGO);
void mandaLab(int abrirFIFO_JOGO);
void atualizaMapa();
char  intervalo[5]="3", duracao[5]="5";
union sigval val;


#endif // MOTOR_H
