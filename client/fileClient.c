/*
 * File client using stream sockets.
 * Authors:
 * - Bruno Faúndez <brunofaundezv@gmail.com>
 * - Claudio Parra
 * - Maximiliano Tapia
 *
 * This program connects to a server on port 7070 and uploads or downloads files.
 *
 * Usage:
 * ./fileClient hostname port
 *
 * fileServer.c is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 * fileClient.c is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with fileClient.c. If not, see <http://www.gnu.org/licenses/>.
 */

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

//File transfer constants
#define CHUNK_SIZE 16							//Number of bytes read from files to send or receive
#define COMMAND_SIZE 256					//Buffer size in bytes for client commands

int main(int argc, char *argv[])
{
	int sockfd;														//Socket file descriptor

	struct hostent *he;										//Host address
	struct sockaddr_in server_address;		//Server address

	if (argc != 3) {
		fprintf(stderr,"Uso: cliente host_address port\n");
		exit(1);
	}

	//The program receives an address for the server and a port to connect to
	char * address = argv[1];
	int port = atoi(argv[2]);

	//The host name is queried based on the name given in the command line
	if ((he = gethostbyname(argv[1])) == NULL) {
		perror("gethostbyname");
		exit(1);
	}

	//Client-Server socket is initialized as SOCK_STREAM to transfer data based on a connection
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

	//Conection was successful
	printf("Conexión exitosa al servidor %s:%d.\n", address, port);
	char command[COMMAND_SIZE];
	char *cmd;
	char *filename;
	while(1){
		//Console stays waiting for commands
		printf("Ingrese comandos: ");
		memset(command, 0, sizeof(command));
		fgets(command, sizeof(command), stdin);
		//Command is separated in command and filename
		//The command is then validated
		cmd = strtok(command, " ");
		filename = strtok(NULL, "\n");

		if(strcmp("traer", cmd) == 0){
			//Downloads file from server
			printf("Comando: Traer\n");
			char * request = concat(cmd, " ");
			request = concat(request, filename);

			//Sends a download request so the server starts sending the file
			if (send(sockfd, request, strlen(request), 0) == -1)
				perror("send");
			free(request);

			FILE *fw = fopen(filename, "w");

			//Initializes a buffer to store pieces of the file to be received
			char file_buffer[CHUNK_SIZE];
			bzero(file_buffer, CHUNK_SIZE);

			//Reads from socket, writes bytes to file in client side, clears buffer,
			//until there is no more to be read from server
			long int total_bytes_read = 0;
			int bytes_read = 0;
			while((bytes_read = recv(sockfd, file_buffer, CHUNK_SIZE, 0)) > 0){
				int bytes_written = fwrite(file_buffer, sizeof(char), bytes_read, fw);
				if(bytes_written < bytes_read){
					error("File download failed.\n");
				}
        bzero(file_buffer, CHUNK_SIZE);

				//Counts received bytes and displays them to see transmission progress
				total_bytes_read += bytes_read;
				printSize(total_bytes_read);
        if (bytes_read == 0 || bytes_read != CHUNK_SIZE){
						printf("\nTransfer complete.\n");
            break;
        }
 			}
			fclose(fw);
		}else if(strcmp("subir", cmd) == 0){
			//Uploads file to server
			printf("Comando: Subir\n");
			char * request = concat(cmd, " ");
			request = concat(request, filename);

			//Sends an upload request to the server so it stays waiting to receive the file
			if (send(sockfd, request, strlen(request), 0) == -1)
				perror("send");
			free(request);

			FILE *fs = fopen(filename, "r");
			if(fs == NULL)
			{
					printf("ERROR: File %s not found.\n", filename);
					exit(1);
			}

			//Initializes a buffer to store pieces of the file to be uploaded
			char file_buffer[CHUNK_SIZE];
			//Reads from file into buffer, sends buffer to client, clears buffer,
			//until there is no more to be read from file
			int bytes_read = 0;
			while((bytes_read = fread(file_buffer, sizeof(char), CHUNK_SIZE, fs)) > 0){
				if(send(sockfd, file_buffer, bytes_read, 0) < 0){
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
