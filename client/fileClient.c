#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "../lib/visuals.h"
#include "../lib/helpers.h"

#define PORT 7070
#define MAXDATASIZE 100
#define CHUNK_SIZE 256

int main(int argc, char *argv[])
{	
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	char get_file_buf[CHUNK_SIZE];
	struct hostent *he;
	struct sockaddr_in their_addr; // información de la dirección de destino
	if (argc != 3) {
		fprintf(stderr,"usage: client hostname port\n");
		exit(1);
	}
	char * address = argv[1];
	int port = atoi(argv[2]);
	
	if ((he = gethostbyname(argv[1])) == NULL) {
		perror("gethostbyname");
		exit(1);
	}
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero), '\0', 8);
	if (connect(sockfd, (struct sockaddr *)&their_addr,sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}
	
	printf("Conexión exitosa al servidor %s:%d.\n", address, port);
	char command[256];
	char *cmd;
	char *filename;
	while(1){
		printf("Ingrese comandos: ");
		fgets(command, sizeof(command), stdin);
		cmd = strtok(command, " ");
		filename = strtok(NULL, " ");
		if(strcmp("traer",cmd) == 0){
			printf("Comando: Traer\n");
			char * request = concat(cmd, " ");
			request = concat(request, filename);
			if (send(sockfd, request, strlen(request), 0) == -1)
				perror("send");
			free(request);
			FILE *fr = fopen(filename, "a");
			bzero(get_file_buf, CHUNK_SIZE);
			int block_size = 0;
			while((block_size = recv(sockfd, get_file_buf, CHUNK_SIZE, 0)) > 0){
				int write_sz = fwrite(get_file_buf, sizeof(char), block_size, fr);
				if(write_sz < block_size){
					error("File write failed on server.\n");
				}
                bzero(get_file_buf, CHUNK_SIZE);
                if (block_size == 0 || block_size != CHUNK_SIZE) 
                {
                    break;
                }

			}
			/*
			if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
				perror("recv");
				exit(1);
			}
			buf[numbytes] = '\0';
			printf("El servisaurio dijo: %s",buf);*/
		}else if(strcmp("subir",cmd) == 0){
			printf("Comando: Subir\n");
		}else if(strcmp("salir\n",cmd) == 0){
			printf("Cerrando el programa...\n");
			return 0;
		}else{
			printf("Comando inválido.\n");
		}
		printf("%s", filename);
	}	
	
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		perror("recv");
		exit(1);
	}
	
	buf[numbytes] = '\0';
	printf("Received: %s",buf);
	close(sockfd);
	return 0;
}
