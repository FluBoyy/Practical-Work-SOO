#ifndef JOGOUI_H
#define JOGOUI_H

#include "utils.h"
typedef struct jogoUI{
    char nomeJogador[50];
    int pid;
    int x, y;
    char comando[TAMANHO_STRING];
    int resposta;
    char labirinto[16][40];
    char mensagem[TAMANHO_STRING];
}JogoUI;
JogoUI jogo;

typedef struct janelas{
    WINDOW *janelaJogo;
    WINDOW *janelaComandos;
    WINDOW *sub_janela;
}Janelas;
Janelas janela;

typedef struct recebeLista{
    char nomeJogador[TAMANHO_STRING];
    int pid;
}RecebeLista;
RecebeLista lista[MAX_UTILIZADORES];

char JogadorNome[TAMANHO_STRING];
int JogadorPid;
void recebeMensagem(int abrirFIFO_JOGO);
void *thread_funcaoLabirinto(void * arg);
void janelas(WINDOW *janela, int x);
void atualizaCoordenadas(int x,int y,int abrirFIFO_JOGO,int abrirFIFO_MOTOR);
void carregarLabirinto(char labirinto[16][40]);
void lerResposta(int abrirFIFO_JOGO);
void enviaNome();
void executaComando(char *comando,int abrirFIFO_MOTOR,int abrirFIFO_JOGO);
void fazerPedido(char *comando,int abrirFIFO_MOTOR);
void recebePedido(int abrirFIFO_JOGO);
#endif
