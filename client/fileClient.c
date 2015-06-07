#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "../lib/helpers.h"

//Socket constants
#define PORT 7070

//File transfer constants
#define CHUNK_SIZE 128
#define COMMAND_SIZE 256

int main(int argc, char *argv[])
{
	int sockfd, numbytes;

	struct hostent *he;
	struct sockaddr_in server_address;

	if (argc != 3) {
		fprintf(stderr,"Uso: cliente host_address port\n");
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

	//Client socket configuration
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(server_address.sin_zero), '\0', 8);
	if (connect(sockfd, (struct sockaddr *)&server_address,sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}

	printf("Conexión exitosa al servidor %s:%d.\n", address, port);
	char command[COMMAND_SIZE];
	char *cmd;
	char *filename;
	while(1){
		printf("Ingrese comandos: ");
		memset(command, 0, sizeof(command));
		fgets(command, sizeof(command), stdin);
		cmd = strtok(command, " ");
		filename = strtok(NULL, "\n");

		if(strcmp("traer", cmd) == 0){
			printf("Comando: Traer\n");
			char * request = concat(cmd, " ");
			request = concat(request, filename);

			if (send(sockfd, request, strlen(request), 0) == -1)
				perror("send");
			free(request);

			FILE *fw = fopen(filename, "w");

			char file_buffer[CHUNK_SIZE];
			bzero(file_buffer, CHUNK_SIZE);

			long int total_bytes_read = 0;
			int bytes_read = 0;
			while((bytes_read = recv(sockfd, file_buffer, CHUNK_SIZE, 0)) > 0){
				int bytes_written = fwrite(file_buffer, sizeof(char), bytes_read, fw);
				if(bytes_written < bytes_read){
					error("File download failed.\n");
				}
        bzero(file_buffer, CHUNK_SIZE);
				total_bytes_read += bytes_read;
				printSize(total_bytes_read);
        if (bytes_read == 0 || bytes_read != CHUNK_SIZE){
            break;
        }
 			}
			fclose(fw);
		}else if(strcmp("subir", cmd) == 0){
			printf("Comando: Subir\n");
			FILE *fs = fopen(filename, "r");
			if(fs == NULL)
			{
					printf("ERROR: File %s not found.\n", filename);
					exit(1);
			}
			char file_buffer[CHUNK_SIZE];
			int block_size = 0;
			while((block_size = fread(file_buffer, sizeof(char), CHUNK_SIZE, fs)) > 0){
				if(send(sockfd, file_buffer, block_size, 0) < 0){
					perror("send");
					exit(1);
				}
				bzero(file_buffer, CHUNK_SIZE);
			}
			fclose(fs);
		}else if(strcmp("salir\n", cmd) == 0){
			printf("Cerrando el programa...\n");
			return 0;
		}else{
			printf("Comando inválido.\n");
		}
	}
	close(sockfd);
	return 0;
}
