 
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
# include <pthread.h>
# include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

#define MAX_FLOW_SIZE 1000  //Tamanho maximo do buffer de caracteres
#define true 1

pthread_t id,id2;
int flag = 0;  //mudar para bol, flag que libera e trava threads, consequentemente os botoes
int ready; //mudar para bol, sem uso por enquanto
int fd1;
int fd2;
char value1;
char value2;
char msg1[6];
char msg2[6];
struct pollfd poll_gpio1;
struct pollfd poll_gpio2;
char buf[MAX_FLOW_SIZE];

void * minha_thread (void *apelido) { //thread responsavel pelo primeiro botao
	while (1) {
        poll(&poll_gpio1, 1, -1);
        if((poll_gpio1.revents & POLLPRI) == POLLPRI){
            lseek(fd1, 0, SEEK_SET);
            read(fd1, &value1, 1);
          	strcpy(buf, "Gato1");
			printf("Interrupt GPIO val: %s\n", buf); 
            flag = 1;
            sleep(3); //sem isso o server quebra, pois envia dois msgs muito rapido
            pthread_exit(NULL);
        }
        else if(flag == 1){
        	pthread_exit(NULL);
        }
	}
}

void * minha_thread_2(void *apelido) { //thread responsavel pelo segundo botao
	while (1) {
        poll(&poll_gpio2, 1, -1);
        if((poll_gpio2.revents & POLLPRI) == POLLPRI){
            lseek(fd2, 0, SEEK_SET);
            read(fd2, &value1, 1);
            strcpy(buf, "Dogo2");
            printf("Interrupt GPIO val: %s\n", buf); 
            flag = 1;
            sleep(3); //sem isso o server quebra, pois envia dois msgs muito rapido
            pthread_exit(NULL);
        }
        else if(flag == 1){
        	pthread_exit(NULL);
        }
	}
}


int main(int argc, char *argv[]){
	int sockId, serverPort, sentBytes, recvBytes, connId;
	struct sockaddr_in client;
	struct hostent *hp;

 
    poll_gpio1.events = POLLPRI;
    poll_gpio2.events = POLLPRI;
 
    // export GPIO
    fd1 = open("/sys/class/gpio/export", O_WRONLY);
    write(fd1, "46", 2);
    close(fd1);
    
    fd2 = open("/sys/class/gpio/export", O_WRONLY);
    write(fd2, "88", 2);
    close(fd2);
 
    // configure as input
    fd1 = open("/sys/class/gpio/gpio46/direction", O_WRONLY);
    write(fd1, "in", 2);
    close(fd1);
    
    fd2 = open("/sys/class/gpio/gpio88/direction", O_WRONLY);
    write(fd2, "in", 2);
    close(fd2);
 
    // configure interrupt
    fd1 = open("/sys/class/gpio/gpio46/edge", O_WRONLY);
    write(fd1, "rising", 6); // configure as rising edge
    close(fd1);
    
    fd2 = open("/sys/class/gpio/gpio88/edge", O_WRONLY);
    write(fd2, "rising", 6); // configure as rising edge
    close(fd2);
    
    
    
/*
 *******************************************************************************
               Testando os argumentos passados na chamada
 *******************************************************************************
 */
	if (argc != 3) {
		printf("Incorreto... Usar: %s IP_ou_nome_do_servidor porta_do_servidor\n", argv[0]);
		return(1);
	}
/*----------------------------------------------------------------------------*/

/*
 *******************************************************************************
               Setando e testando o endereco informado (primeiro argumento)
 *******************************************************************************
 */
	hp = gethostbyname(argv[1]);
	if (((char *) hp) == NULL) {
		printf("Host Invalido: %s\n", argv[1]);
		return(1);
	}
/*----------------------------------------------------------------------------*/

/*
 *******************************************************************************
               Setando os atributos da estrutura do socket
 *******************************************************************************
 */
	memcpy((char*)&client.sin_addr, (char*)hp->h_addr, hp->h_length);
	client.sin_family = AF_INET;
	serverPort = atoi(argv[2]);
	if (serverPort <= 0) {
		printf("Porta Invalida: %s\n", argv[2]);
		return(1);
	}
	client.sin_port = htons(serverPort);
/*----------------------------------------------------------------------------*/

/*
 *******************************************************************************
               Abrindo um socket do tipo stream (TCP)
 *******************************************************************************
 */
	sockId = socket(AF_INET, SOCK_STREAM, 0);
	if (sockId < 0) {
		printf("Stream socket nao pode ser aberto\n");
		return(1);
	}
/*----------------------------------------------------------------------------*/

/*
 *******************************************************************************
               Conectando o socket ao servidor
 *******************************************************************************
 */
	connId = connect(sockId, (struct sockaddr *)&client, sizeof(client));
	if (connId == -1) {
		printf("A conexao com o servidor %s na porta %s falhou\n", argv[1], argv[2]);
		close(sockId);
		return(1);
	}
/*----------------------------------------------------------------------------*/
    // wait for interrupt
    
 	while(1){
    // open value file
    fd1 = open("/sys/class/gpio/gpio46/value", O_RDONLY);
    poll_gpio1.fd = fd1;
 
    poll(&poll_gpio1, 1, -1); // discard first IRQ
    read(fd1, &value1, 1);
    
    fd2 = open("/sys/class/gpio/gpio88/value", O_RDONLY);
    poll_gpio2.fd = fd2;
 	
    poll(&poll_gpio2, 1, -1); // discard first IRQ
    read(fd2, &value2, 1);
    
   	pthread_create (&id, NULL , (void *) minha_thread , NULL);
	pthread_create (&id2,NULL , (void *) minha_thread_2,NULL);
	
	while(flag == 0){}
	sleep(5); //sem isso o server quebra, pois envia dois msgs muito rapido
	flag = 0; //controle da threads, possivel agora rodas novamente

 
 
    close(fd1); //close value1 file
    close(fd2); //close value1 file

		printf("Mensagem digitada: [%s]\n",buf);
		printf("Tamanho do buffer: %zu\n",strlen(buf));

/*
 *******************************************************************************
               Condicao de parada (termino normal)
 *******************************************************************************
 */
		if (!strcmp(buf,"END")) {
			close(sockId);
			printf("Encerrado\n");
			return(0);
		}
/*----------------------------------------------------------------------------*/

/*
 *******************************************************************************
               Enviando as mensagens para o servidor
 *******************************************************************************
 */
		sentBytes = send(sockId, buf, strlen(buf), 0);
		if (sentBytes < 0) {
			close(sockId);
			printf("A conexao foi perdida\n");
			return(1);
		}
		memset(buf,0,sizeof(buf));
/*----------------------------------------------------------------------------*/
/*
 *******************************************************************************
               Recebendo as mensagens do servidor
 *******************************************************************************
 */
		recvBytes = recv(sockId, buf, MAX_FLOW_SIZE, 0);
		printf("Passei por aqui\n"); //teste para ver onde o que o codigo esta rodando
		if (recvBytes < 0) {
			close(sockId);
			printf("Ocorreu um erro na aplicacao\n");
			return(1);
		}
		if (recvBytes == 0) {
			close(sockId);
			printf("A conexao foi encerrada pelo servidor\n");
			return(1);
		}
		printf("O servidor retornou %zu caracteres: [%s]\n", strlen(buf), buf);
		memset(buf,0,sizeof(buf));
/*----------------------------------------------------------------------------*/
		
	}
}


 
