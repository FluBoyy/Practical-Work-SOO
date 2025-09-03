#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include "jogoUI.h"

void janelas(WINDOW *janela, int x)
{
    if (x == 1)
    {
        scrollok(janela, TRUE); // liga o scroll na "janela".
    }
    else{
        keypad(janela, TRUE); // para ligar as teclas de direção (aplicar à janela)
        wclear(janela);// limpa a janela
        int linhas, colunas;
        getmaxyx(janela, linhas, colunas);
        wborder(janela, '|', '|', '-', '-', '+', '+', '+', '+'); // Desenha uma borda. Nota importante: tudo o que escreverem, devem ter em conta a posição da borda
    }
    refresh(); // necessário para atualizar a janela
    wrefresh(janela); // necessário para atualizar a janela
}

void enviaNome(){
    int abrirFIFO_MOTOR;
    abrirFIFO_MOTOR = open(FIFO_MOTOR, O_WRONLY | O_NONBLOCK);
    if (abrirFIFO_MOTOR == -1){
        printf("[ERRO] Nao foi possivel abrir o FIFO_MOTOR!\n");
        unlink(FIFO_JOGO);
        exit(1);
    }
    write(abrirFIFO_MOTOR, &jogo, sizeof(JogoUI));
    close(abrirFIFO_MOTOR);
}

void atualizaCoordenadas(int x,int y,int abrirFIFO_JOGO,int abrirFIFO_MOTOR){
    jogo.x = x;
    jogo.y = y;
    int abrirFIFO_atualiza = open(FIFO_ATUALIZA, O_WRONLY );
    if (abrirFIFO_atualiza == -1){
        printf("[ERRO] Nao foi possivel [ABRIR]o FIFO_MOTOR ao atualizar as cordenadas!\n");
        unlink(FIFO_ATUALIZA);
        exit(1);
    }
    write(abrirFIFO_atualiza, &jogo, sizeof(JogoUI));
}

void lerResposta(int abrirFIFO_JOGO){

    // Lê o labirinto do pipe do motor
    if (abrirFIFO_JOGO == -1) {
        fprintf(stderr, "Erro a abrir o pipe do servidor!\n");
        exit(-1);
    }
    // Lê o labirinto do pipe
    read(abrirFIFO_JOGO, &jogo,  sizeof(JogoUI));
    if (jogo.resposta==0){
        unlink(FIFO_JOGO);
        perror("[ERRO] Já existe um Nome igual!");
        exit(2);
    }
    carregarLabirinto(jogo.labirinto);
}

void carregarLabirinto(char labirinto[16][40]) {
    // Cria uma sub-janela dentro da janela do labirinto
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 40; j++) {
            mvwaddch(janela.sub_janela, i, j, labirinto[i][j]); // Imprime o caractere na posição [i][j] do labirinto
        }
    }
    wrefresh(janela.sub_janela);  // Atualiza a sub-janela após imprimir o labirinto
}

void *thread_funcaoLabirinto(void * arg){
    if(access(FIFO_ATUALIZA2, F_OK)!=0){
        mkfifo(FIFO_ATUALIZA2,0600);
    }
    while(1) {

        int abrirFIFO_ATUALIZA2 = open(FIFO_ATUALIZA2, O_RDONLY );
        if(abrirFIFO_ATUALIZA2 == -1){
            printf("\n[ERRO] Ao abrir o FIFO ATUALIZA!\n");
            unlink(FIFO_ATUALIZA);
            exit(1);
        }
        ssize_t n = read(abrirFIFO_ATUALIZA2, &jogo, sizeof(JogoUI));  // Usar uma variável local para receber os dados do jogo
        if (n == -1) {
            perror("[ERRO] Ao ler do FIFO_ATUALIZA");
            unlink(FIFO_ATUALIZA);
            break;
        }
        close(abrirFIFO_ATUALIZA2);
        if (strcmp(jogo.mensagem,"Expulso")==0){
            if(jogo.pid==JogadorPid){
                wprintw(janela.janelaComandos,"\nFoste expulso do jogo!\n");
                wrefresh(janela.janelaComandos);
                strcpy(jogo.mensagem, "");
                endwin(); // Encerra o ncurses
                unlink(FIFO_JOGO);
                exit(0); // Sai do programa
            }else{
                wprintw(janela.janelaComandos,"\nO jogador %s foi expulso\n",jogo.nomeJogador);
            }
            wrefresh(janela.janelaComandos);
        }else if(strcmp(jogo.mensagem,"Saiu")==0){
            wprintw(janela.janelaComandos,"\nO jogador %s saiu do jogo\n",jogo.nomeJogador);
            wrefresh(janela.janelaComandos);
            strcpy(jogo.mensagem, "");
        }
        carregarLabirinto(jogo.labirinto);
    }
    unlink(FIFO_ATUALIZA2);
    return NULL;
}

void *thread_funcaoTeclado(void *arg){
    int abrirFIFO_JOGO = *((int*)arg);
    // Converter o ponteiro void de volta para um ponteiro int e desreferenciá-lo para obter o valor.
    char comando[50];
    int x=22,y=8;

    // ENVIA INFO
    enviaNome();
    // RECEBE INFO
    lerResposta(abrirFIFO_JOGO);
    do{
        strcpy(jogo.nomeJogador,JogadorNome);
        jogo.pid=JogadorPid;
        strcpy(jogo.comando,"");
        strcpy(jogo.mensagem,"");
        int abrirFIFO_MOTOR;
        abrirFIFO_MOTOR = open(FIFO_MOTOR, O_WRONLY | O_NONBLOCK);
        mvaddch(y, x, jogo.nomeJogador[0]); // mete a 1 letra do nome
        refresh();
        int entrada = getch(); // Captura a entrada do utilizador
        if (entrada == ' ') {
            echo(); // para conseguirmos ver o que escrevemos
            wprintw(janela.janelaComandos, "\n Comando > ");
            wgetstr(janela.janelaComandos, comando);
            executaComando(comando,abrirFIFO_MOTOR,abrirFIFO_JOGO);

            noecho(); // Desabilita a exibição dos caracteres
            wrefresh(janela.janelaComandos);
        } else {
            if (entrada == KEY_RIGHT) {
                // Verifica se a próxima posição é uma parede
                if (jogo.labirinto[y-4][x - 6] !='x') {
                    // Se não for uma parede, atualiza as coordenadas do jogo
                    ++x;
                    atualizaCoordenadas(x,y,abrirFIFO_JOGO,abrirFIFO_MOTOR);
                }
            } else if (entrada == KEY_LEFT) {
                // Verifica se a próxima posição é uma parede
                if (jogo.labirinto[y-4][x - 8] != 'x') {
                    // Se não for uma parede, atualiza as coordenadas do jogo
                    --x;
                    atualizaCoordenadas(x,y,abrirFIFO_JOGO,abrirFIFO_MOTOR);
                }
            } else if (entrada == KEY_DOWN) {
                // Verifica se a próxima posição é uma parede
                if (jogo.labirinto[y -3][x - 7] != 'x') {
                    // Se não for uma parede, atualiza as coordenadas do jogo
                    ++y;
                    atualizaCoordenadas(x,y,abrirFIFO_JOGO,abrirFIFO_MOTOR);
                }
            } else if (entrada == KEY_UP) {
                // Verifica se a próxima posição é uma parede
                if (jogo.labirinto[y - 5][x - 7] != 'x') {
                    // Se não for uma parede, atualiza as coordenadas do jogo
                    --y;
                    atualizaCoordenadas(x,y,abrirFIFO_JOGO,abrirFIFO_MOTOR);
                }
            }
        }
    }while (strcmp(comando,"exit")!=0);
    unlink(FIFO_JOGO);
    return NULL;
}

int main(int argc, char *argv[]) {
    initscr();  // Inicializar ncurses
    cbreak();   // deixa a entrada de caracteres sem a necessidade de pressionar Enter
    keypad(stdscr, TRUE); //teclas especiais como "espaço"
    jogo.x=0;
    jogo.y=0;
    pthread_t Teclado,atualizaCord;
    pthread_mutex_init(&lock, NULL);
    //############### CRIAR JANELAS ###########################
    janela.janelaJogo = newwin(20, 46,    2, 4);
    janela.janelaComandos = newwin(23, 70, 22, 1);
    janelas(janela.janelaJogo,2);
    janelas(janela.janelaComandos,1);
    janela.sub_janela = derwin(janela.janelaJogo, 16, 40, 2, 3);
    // Verifica se o processo motor está em execução
    if(access(FIFO_MOTOR, F_OK)!=0){
        printf("[ERRO] O FIFO do motor nao existe!\n");
        exit(1);
    }
    jogo.pid=getpid();
    //######## CRIAR O FIFO JOGO PARA CADA JOGADOR  ###############
    char str_fifoJogo[TAMANHO_STRING];
    // CRIAR UM FIFO PARA CADA JOGOUI (ALTERAR O DIRETÓRIO DE CADA UM | EX: FIFO_JOGO_2256 (pid))
    sprintf(str_fifoJogo, FIFO_JOGO, jogo.pid);    // str_FIFO_JOGO = FIFO_JOGO + newUtilizador.pid

    //criar o fifo [0 -> sucesso]
    if(mkfifo(str_fifoJogo,0600) != 0){
        printf("\n[ERRO] Ao criar o FIFO JOGO!\n");
    }
    int abrirFIFO_JOGO = open(str_fifoJogo, O_RDWR);
    if(abrirFIFO_JOGO == -1){
        printf("\n[ERRO] Ao abrir o FIFO JOGO!\n");
        unlink(FIFO_JOGO);
    }

//    //######## CRIAR O FIFO ATUALIZA PARA CADA JOGADOR  ###############
//    char str_fifoAtualiza[TAMANHO_STRING];
//    // CRIAR UM FIFO PARA CADA JOGOUI (ALTERAR O DIRETÓRIO DE CADA UM | EX: FIFO_JOGO_2256 (pid))
//    sprintf(str_fifoAtualiza, FIFO_ATUALIZA, jogo.pid);    // str_FIFO_JOGO = FIFO_JOGO + newUtilizador.pid


//##########################################################
    if (argc != 2) {
        printw("Erro de argumentos ex : ""./jogoUI Andre""\n");
        refresh();
        unlink(str_fifoJogo);
        getch();
        endwin(); // Encerrar a ncurses
        return -1;
    }
    else{
        if (strlen(argv[1]) > 50) {
            printw("Error: nomeJogador is too long.\n");
            refresh();
            unlink(str_fifoJogo);
            getch();
            endwin(); // Encerrar a ncurses
            return 1;
        }
        strncpy(jogo.nomeJogador, argv[1], sizeof(jogo.nomeJogador) - 1);
        jogo.nomeJogador[sizeof(jogo.nomeJogador) - 1] = '\0'; // para acabar com \0
        strcpy(JogadorNome,jogo.nomeJogador);
        JogadorPid=jogo.pid;
    }
    //################## THREAD ###############################
    pthread_create(&Teclado, NULL, thread_funcaoTeclado, (void*)&abrirFIFO_JOGO);
    pthread_create(&atualizaCord,NULL,thread_funcaoLabirinto,NULL);
    pthread_join(Teclado, NULL);
    pthread_join(atualizaCord,NULL);
    endwin(); // Encerra o ncurses
    unlink(FIFO_ATUALIZA);
    unlink(str_fifoJogo);
    return 0;
}

void executaComando(char *comando,int abrirFIFO_MOTOR,int abrirFIFO_JOGO){
    char comandoRecebido[50];
    strcpy(comandoRecebido,comando);
    char* argumentosComando[10];
    int nArgumentos = 0;        // numero de argumentos do comando ex: /sell bola 130 ; bola(posição 1) , 130(posição 2)

    argumentosComando[0] = strtok(comando, " ");
    while(argumentosComando[nArgumentos] != NULL){
        argumentosComando[++nArgumentos] = strtok(NULL, " ");
    }
    // strtok divide a string em palavras mas a última será sempre null.Temos de "eliminar" o ultimo "valor" , decrementando a variável nArgumentos
    nArgumentos -= 1;

    // COMANDOS
    if(strcmp(argumentosComando[0], "players") == 0 && nArgumentos == 0){
        wprintw(janela.janelaComandos,"\nComando executado com sucesso! \n\n");
        fazerPedido(comandoRecebido,abrirFIFO_MOTOR);
        recebePedido(abrirFIFO_JOGO);
    }
    else if(strcmp(argumentosComando[0], "msg") == 0 && nArgumentos == 2){
        wprintw(janela.janelaComandos,"\nMensagem enviada!\n\n");
        fazerPedido(comandoRecebido,abrirFIFO_MOTOR);
        recebeMensagem(abrirFIFO_JOGO);
    }
    else if(strcmp(argumentosComando[0], "exit") == 0 && nArgumentos == 0){
        fazerPedido(comandoRecebido,abrirFIFO_MOTOR);
        wprintw(janela.janelaComandos,"\nA sair...\n");
        wrefresh(janela.janelaComandos);
        endwin(); // Encerra o ncurses
        unlink(FIFO_JOGO);
        exit(0); // Sai do programa
    }else{
        wprintw(janela.janelaComandos,"\nComando não encontrado ou executado sem sucesso!\n\n");
    }
}

void recebeMensagem(int abrirFIFO_JOGO){
    int n;
    if (abrirFIFO_JOGO == -1){
        printf("[ERRO] Nao foi possivel ler o FIFO JOGO no recebe Pedido!\n");
        exit(1);
    }

    n = read(abrirFIFO_JOGO, &jogo, sizeof(JogoUI));

    if(n != sizeof(JogoUI)){
        printf("[ERRO] A receber mensagem FIFO [PEDIDO]!\n");
        exit(2);
    }
    wprintw(janela.janelaComandos,"\nMensagem recebida: %s\n",jogo.mensagem);
    wrefresh(janela.janelaComandos);
    strcpy(jogo.mensagem, "");
}

void fazerPedido(char *comando,int abrirFIFO_MOTOR){
    int n;
    strcpy(jogo.comando, comando);
    if (abrirFIFO_MOTOR == -1){
        printf("[ERRO] Nao foi possivel abrir o FIFO MOTOR no fazer pedido!\n");
        exit(1);
    }

    n = write(abrirFIFO_MOTOR, &jogo, sizeof(JogoUI));

    if(n != sizeof(JogoUI)){
        printf("[ERRO] A enviar mensagem FIFO!\n");
        exit(2);
    }

    close(abrirFIFO_MOTOR);
}


void recebePedido(int abrirFIFO_JOGO){
    int n;
    if (abrirFIFO_JOGO == -1){
        printf("[ERRO] Nao foi possivel ler o FIFO JOGO no recebe Pedido!\n");
        exit(1);
    }

    n = read(abrirFIFO_JOGO, &lista, sizeof(lista));

    if(n != sizeof(lista)){
        printf("[ERRO] A receber mensagem FIFO [PEDIDO]!\n");
        exit(2);
    }
    //wprintw(janela.janelaComandos,"\n Lista de jogadores: \nNome:%s",lista[0].nomeJogador);
    for (int i = 0; i < MAX_UTILIZADORES; i++) {
        if (strcmp(lista[i].nomeJogador, "") != 0) {
            wprintw(janela.janelaComandos, "%s\t", lista[i].nomeJogador);
            wrefresh(janela.janelaComandos);
        }
    }
}