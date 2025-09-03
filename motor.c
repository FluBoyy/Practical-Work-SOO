#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <signal.h>
#include "motor.h"
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>


bool valida(WINDOW *janelaComandos, char Buffer[200], int PID[2]){
    char comando[50];
    echo(); // para conseguirmos ver o que escrevemos
    wprintw(janelaComandos, "\n Comando : ");
    wgetstr(janelaComandos, comando);

    char* argumentosComando[10];
    int nArgumentos = 0;        // numero de argumentos do comando ex: /sell bola 130 ; bola(posição 1) , 130(posição 2)

    argumentosComando[0] = strtok(comando, " ");
    while(argumentosComando[nArgumentos] != NULL){
        argumentosComando[++nArgumentos] = strtok(NULL, " ");
    }
    // strtok divide a string em palavras mas a última será sempre null.Temos de "eliminar" o ultimo "valor" , decrementando a variável nArgumentos
    nArgumentos -= 1;

    if(strcmp(comando,"users")==0){
        for (int i = 0; i < MAX_UTILIZADORES; i++) {
            if (strcmp(arrayJogadores[i].nomeJogador, "") != 0) {
                wprintw(janelaComandos,"%s\t", arrayJogadores[i].nomeJogador);
            }
        }
        return 1;
    }
    else if(strcmp(comando, "kick") == 0 && nArgumentos == 1){
        wprintw(janelaComandos,"Comando valido. Banir jogador \n");
        bool expulso = false;
        int i;
        for (i = 0; i < MAX_UTILIZADORES; ++i) {
            if (strcmp(arrayJogadores[i].nomeJogador, argumentosComando[1]) == 0) {
                wprintw(janelaComandos, "Jogador %s expulso com sucesso!\n", argumentosComando[1]);
                int fd = open(FIFO_ATUALIZA2, O_WRONLY);
                if (fd == -1) {
                    perror("\nErro ao abrir o fifo para atualizar o mapa!\n");
                    unlink(FIFO_ATUALIZA2);
                    exit(1);
                }
                strcpy(newUtilizador.nomeJogador, argumentosComando[1]);
                newUtilizador.pid = arrayJogadores[i].pid;
                strcpy(newUtilizador.mensagem,"Expulso");
                write(fd, &newUtilizador, sizeof(Utilizador));
                arrayJogadores[i].pid = 0;
                strcpy(arrayJogadores[i].nomeJogador, "");
                expulso = true;
                break;
            }
        }
        if(!expulso)
            wprintw(janelaComandos, "Jogador [%s] não encontrado!\n", argumentosComando[1]);
        return 1;
    } 
    else if(strcmp(comando, "test bots") == 0){
        wprintw(janelaComandos,"Comando valido. Listar bots:");

        pipe(PID);

        int res = fork();
        if (res < 0) {
            perror("Erro ao criar processo filho");
        } else if (res == 0) { // Processo filho
            close(PID[0]);
            close(1);
            dup(PID[1]);
            close(PID[1]);
            execl("bot", "bot", intervalo, duracao ,NULL);
            perror("\n ##### || ERRO || #####\n");
            sleep(2);
            exit(1);   
        } else { // Processo pai
            int n=0;
            while(n<3){
            read(PID[0],Buffer, 199);
            wprintw(janelaComandos,"\nBot lançado com sucesso.\n");
            wprintw(janelaComandos,"Recebido = %s\n",Buffer);
            n++;
            wrefresh(janelaComandos);
            }


            sigqueue(res, SIGINT, val); // Envia um sinal SIGINT para o processo com o ID res
            
            int status;     
            waitpid(res, &status, 0); // Espera o processo com o ID res terminar e armazena seu status de saída em status
            return 1;
        }
    } else if(strcmp(comando, "begin") == 0){
        wprintw(janelaComandos,"Comando valido. Iniciar jogo\n");
        return 1;
    } 
    else if(strcmp(comando, "bmov") == 0){
        wprintw(janelaComandos,"Comando valido. Inserir bloqueio\n");
        return 1;
    } 
    else if(strcmp(comando, "rbm") == 0){
        wprintw(janelaComandos,"Comando valido. Remover bloqueio\n");
        return 1;
    } 
    else if(strcmp(comando, "end") == 0){
        wprintw(janelaComandos, "Comando valido. Terminar Jogo\n");
        wrefresh(janelaComandos);
        wgetch(janelaComandos);
        unlink(FIFO_MOTOR);
        endwin();
        return 0;
    } 
    else {
        wprintw(janelaComandos, "  ## Comando inválido! ## \n");
        return 1;
    }
    noecho(); // Desabilita a exibição dos caracteres
    wrefresh(janelaComandos);
}

void janelas(WINDOW *janela, int x) {
    if (x != 1) {
        keypad(janela, TRUE);
        wclear(janela);
        int linhas, colunas;
        getmaxyx(janela, linhas, colunas);
        // Desenha uma borda
        wborder(janela, '|', '|', '-', '-', '+', '+', '+', '+');
    } else
    {
        scrollok(janela, TRUE);
    }
    
    refresh();
    wrefresh(janela);
}

void carregarLabirinto(WINDOW *janela, const char *nomeArquivo,char labirinto[16][40]) {
    FILE *arquivo = fopen(nomeArquivo, "r");
    if (arquivo == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(1);
    }

    // Cria uma sub-janela dentro da janela do labirinto
    WINDOW *sub_janela = derwin(janela, 16, 40, 2, 3);

    int i = 0;
    while (fgets(labirinto[i], 42, arquivo) != NULL) {
    i++;
    }

    for (i = 0; i < 16; i++) {
        for (int j = 0; j < 40; j++) {
            wprintw(sub_janela, "%c", labirinto[i][j]); // Imprime o caractere na posição [i][j] do labirinto
        }
    }
    wrefresh(sub_janela);  // Atualiza a sub-janela após imprimir o labirinto
    fclose(arquivo);
}

void atualizaMapa(){
    int fd = open(FIFO_ATUALIZA2, O_WRONLY );
    if(fd == -1){
        perror("\nErro ao abrir o fifo para atualizar o mapa!\n");
        unlink(FIFO_ATUALIZA2);
        exit(1);
    }
    write(fd, &newUtilizador, sizeof(Utilizador));
    close(fd);
}

void *thread_enviaCordeadas(){
    if(access(FIFO_ATUALIZA,F_OK)!=0){
        mkfifo(FIFO_ATUALIZA,0600);
    }
    int abrirFIFO_atualiza = open(FIFO_ATUALIZA, O_RDONLY );
    if(abrirFIFO_atualiza == -1){
        perror("\nErro ao abrir o fifo para atualizar o mapa!\n");
        unlink(FIFO_ATUALIZA);
        unlink(FIFO_MOTOR);
        exit(0);
    }
    do {
        int n =read(abrirFIFO_atualiza, &newUtilizador, sizeof(Utilizador));
        if(n>0){
            for (int i = 0; i < 16; ++i) {
                for (int j = 0; j < 40; ++j) {
                    if (m.labirinto[i-4][j-7] == newUtilizador.nomeJogador[0]) {
                        m.labirinto[i-4][j-7] = ' ';
                    }
                    newUtilizador.labirinto[i][j]=m.labirinto[i][j];
                }
            }
            m.labirinto[newUtilizador.y - 4][newUtilizador.x - 7] = newUtilizador.nomeJogador[0];
            newUtilizador.labirinto[newUtilizador.y - 4][newUtilizador.x - 7] = newUtilizador.nomeJogador[0];
            atualizaMapa();
        }else{
            perror("\nErro ao ler o fifo para atualizar o mapa!\n");
            unlink(FIFO_ATUALIZA);
            unlink(FIFO_MOTOR);
            exit(0);
        }
    } while(1);
}

void *thread_funcao(void *arg){
    int abrirFIFO_MOTOR = *((int*)arg); // Converter o ponteiro void de volta para um ponteiro int e desreferenciá-lo para obter o valor.
    int abrirFIFO_JOGO;
    do {
        strcpy(newUtilizador.comando,"");
        strcpy(newUtilizador.mensagem,"");
        strcpy(newUtilizador.nomeJogador,"");
        newUtilizador.pid=0;
        abrirFIFO_MOTOR = open(FIFO_MOTOR, O_RDONLY );
        read(abrirFIFO_MOTOR,&newUtilizador,sizeof(Utilizador));
        if(strcmp(newUtilizador.comando,"")==0){
            if(newUtilizador.x != 0 && newUtilizador.y != 0){
                pthread_mutex_lock(&lock);

                pthread_mutex_unlock(&lock);
            }else{
                recebe(abrirFIFO_JOGO);
            }
        }else {
            char *argumentosComando[10];
            int nArgumentos = 0;

            argumentosComando[0] = strtok(newUtilizador.comando, " ");
            while (argumentosComando[nArgumentos] != NULL) {
                argumentosComando[++nArgumentos] = strtok(NULL, " ");
            }
            nArgumentos -= 1;
            if (strcmp(argumentosComando[0], "players") == 0 && nArgumentos == 0) {
                char str_fifoJogo[TAMANHO_STRING];
                // CRIAR UM FIFO PARA CADA JOGO (ALTERAR O DIRETÓRIO DE CADA UM | EX: FIFO_JOGO (pid))
                sprintf(str_fifoJogo, FIFO_JOGO, newUtilizador.pid);    // str_fifojogo = FIFO_JOGO + newUtilizador.pid
                abrirFIFO_JOGO = open(str_fifoJogo, O_WRONLY | O_NONBLOCK);
                if(access(str_fifoJogo, F_OK) != -1) {                   // Verifica se o FIFO ainda existe
                    if(abrirFIFO_JOGO == -1){
                        perror("\nErro ao mandar os players!\n");
                        unlink(FIFO_MOTOR);
                        exit(1);
                    }
                    write(abrirFIFO_JOGO, &arrayJogadores, sizeof(arrayJogadores));
                    close(abrirFIFO_JOGO);
                }
            } else if (strcmp(argumentosComando[0], "msg") == 0 && nArgumentos == 2) {
                strcpy(newUtilizador.mensagem,argumentosComando[2]);
                for (int i = 0; i < MAX_UTILIZADORES; ++i) {
                    if(strcmp(arrayJogadores[i].nomeJogador, argumentosComando[1]) == 0){
                        char str_fifoJogo[TAMANHO_STRING];
                        sprintf(str_fifoJogo, FIFO_JOGO, arrayJogadores[i].pid); // Cria o nome do FIFO para o jogador
                        abrirFIFO_JOGO = open(str_fifoJogo, O_WRONLY | O_NONBLOCK); // Abre o FIFO do jogador
                        if(abrirFIFO_JOGO == -1){
                            perror("\nErro ao enviar a mensagem!\n");
                            unlink(FIFO_MOTOR);
                            exit(1);
                        }
                        write(abrirFIFO_JOGO, &newUtilizador, sizeof(Utilizador)); // Envia as coordenadas atualizadas para o jogador
                        close(abrirFIFO_JOGO); // Fecha o FIFO do jogador
                    }
                }
            } else if (strcmp(argumentosComando[0], "exit") == 0 && nArgumentos == 0) {
                for (int i = 0; i < MAX_UTILIZADORES; ++i) {
                    if (strcmp(arrayJogadores[i].nomeJogador, newUtilizador.nomeJogador) == 0) {
                        newUtilizador.pid = arrayJogadores[i].pid;
                        strcpy(newUtilizador.mensagem,"Saiu");
                        int fd = open(FIFO_ATUALIZA2, O_WRONLY);
                        write(fd, &newUtilizador, sizeof(Utilizador));
                        arrayJogadores[i].pid = 0;
                        strcpy(arrayJogadores[i].nomeJogador, "");
                        break;
                    }
                }
            }
        }
    } while(1);
    close(abrirFIFO_MOTOR);
}

void recebe(int abrirFIFO_JOGO){

    int nomeExiste = 0;
    for (int i = 0; i < MAX_UTILIZADORES; i++){
        if(strcmp(newUtilizador.nomeJogador, arrayJogadores[i].nomeJogador) == 0){
            nomeExiste = 1;
            break;
        }
    }

    if(nomeExiste==0) {
        // O nome não existe no array, então podemos armazená-lo
        for (int i = 0; i < MAX_UTILIZADORES; i++) {
            if (arrayJogadores[i].nomeJogador[0] == '\0') {
                strcpy(arrayJogadores[i].nomeJogador, newUtilizador.nomeJogador);
                arrayJogadores[i].pid = newUtilizador.pid;
                strcpy(m.nomeJogador, newUtilizador.nomeJogador);
                m.pid=newUtilizador.pid;
                m.resposta = 1;
                mandaLab(abrirFIFO_JOGO);
                break;
            }
        }
    }else {
        // O nome já existe no array, então não podemos armazená-lo
        newUtilizador.resposta = 0;
        char str_fifoJogo[TAMANHO_STRING];
        // CRIAR UM FIFO PARA CADA JOGO (ALTERAR O DIRETÓRIO DE CADA UM | EX: FIFO_JOGO (pid))
        sprintf(str_fifoJogo, FIFO_JOGO, newUtilizador.pid);    // str_fifojogo = FIFO_JOGO + newUtilizador.pid
        abrirFIFO_JOGO = open(str_fifoJogo, O_WRONLY);
        // Verifica se o FIFO ainda existe
         if(abrirFIFO_JOGO == -1) {
             perror("\nErro ao abrir o fifo para mandar ao jogo (nome ja usado)!\n");
             unlink(FIFO_MOTOR);
             exit(1);
         }
        write(abrirFIFO_JOGO, &newUtilizador, sizeof(Utilizador));
        close(abrirFIFO_JOGO);
    }
}

void mandaLab(int abrirFIFO_JOGO){
    pthread_mutex_lock(&lock);
    char str_fifoJogo[TAMANHO_STRING];
    sprintf(str_fifoJogo, FIFO_JOGO, m.pid);

    if(access(str_fifoJogo, F_OK) != 0) {
        if(mkfifo(str_fifoJogo, 0600) != 0) {
            perror("\nErro ao criar o FIFO JOGO!\n");
            exit(1);
        }
    }

    abrirFIFO_JOGO = open(str_fifoJogo, O_WRONLY);
    if(abrirFIFO_JOGO == -1) {
        perror("\nErro ao abrir o fifo para mandar o labirinto!\n");
        unlink(FIFO_MOTOR);
        exit(1);
    }
    write(abrirFIFO_JOGO, &m, sizeof(Motor));
    close(abrirFIFO_JOGO);
    pthread_mutex_unlock(&lock);
}

int main() {
    int n;
    const char *nomeArquivo = "nivel1.txt";
    initscr();  // Inicializar ncurses
    cbreak();
    keypad(stdscr, TRUE);
    WINDOW *janelaLabirinto = newwin(20, 46, 2, 4);
    WINDOW *janelaComandos = newwin(23, 70, 22, 1);
    janelas(janelaLabirinto, 2);
    janelas(janelaComandos, 1);
    carregarLabirinto(janelaLabirinto, nomeArquivo,m.labirinto);

//    if(getenv("NPLAYERS")==NULL){
//        printf("\n[ERRO] NPLAYERS nao definido!\n");
//        exit(-1);
//    }
//    if(getenv("DURACAO")==NULL){
//        printf("\n[ERRO] DURACAO nao definido!\n");
//        exit(-1);
//    }
//    if(getenv("INSCRICAO")==NULL){
//        printf("\n[ERRO] INSCRICAO nao definido!\n");
//        exit(-1);
//    }
//    if (getenv("DECREMENTO") != NULL) {
//        printf("\n[ERRO] INCRICAO nao definido!\n");
//        exit(-1);
//    }
    // Criar fifo motor
    if(access(FIFO_MOTOR, F_OK)==0){
        perror("\nJa existe um motor ativo!\n");
        exit(-1);
    }
    //criar o fifo [0 -> sucesso]
    if(mkfifo(FIFO_MOTOR,0600) != 0){
        // ERRO AO CRIAR FIFO BACKEND
        perror("\nErro ao criar o fifo motor!\n");
        exit(0);
    }
    int abrirFIFO_MOTOR = open(FIFO_MOTOR, O_RDWR);
    if(abrirFIFO_MOTOR == -1){
        unlink(FIFO_MOTOR);
        perror("\nErro ao abrir o fifo para resposta!\n");
        exit(-1);
    }

    //-----------------------------
    pthread_t thread, thread2;
    pthread_create(&thread, NULL, thread_funcao, (void*)&abrirFIFO_MOTOR);
    pthread_create(&thread2, NULL, thread_enviaCordeadas, NULL);
    //----------------------------------------

    char Buffer[200];
    int PID[2];
    do {
        n=valida(janelaComandos, Buffer, PID);
    } while (n==1);

    endwin();  // Encerra o ncurses
    unlink(FIFO_MOTOR);

    pthread_mutex_destroy(&lock);
    pthread_join(thread, NULL);
    pthread_join(thread2, NULL);

    return 0;
}