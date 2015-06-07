/*
 * File server using stream sockets.
 * Authors:
 * - Bruno Faúndez <brunofaundezv@gmail.com>
 * - Claudio Parra
 * - Maximiliano Tapia
 *
 * This program receives connections on port 7070 and receives or sends files.
 * fileServer.c is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 * tum_ardrone is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with fileServer.c. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "../lib/helpers.h"

//Server socket constants
#define PORT 7070
#define BACKLOG 5

//File transfer constants
#define COMMAND_SIZE 256			//Buffer size in bytes for client commands
#define CHUNK_SIZE 16					//Number of bytes read from files to send or receive

void sigchld_handler(int s)
{
	while(wait(NULL) > 0);
}

int main(){
	int server_sockfd, client_sockfd;			//Socket variables for server and client
	char cmd_buf[COMMAND_SIZE];						//Buffer to store requests from client
	int numbytes;													//Bytes received in client command
	struct sockaddr_in server_address;		//Server address
	struct sockaddr_in client_address;		//Client address
	int sin_size;													//Size of client sockets

	//Client commands are separated in two strings: A command and a filename
	char *cmd;
	char *filename;

	//Server socket is initialized as SOCK_STREAM to transfer data based on a connection
	if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	//Setsockopt configures socket to make it reusable after it is closed
	int yes = 1;
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	//Server socket configuration
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;
	memset(&(server_address.sin_zero), '\0', 8);

	if (bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(struct sockaddr)) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(server_sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}else{
		printf("Server listening on address: %s:%d.\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));
	}

	//Sigaction clears dead processes
	struct sigaction sa;
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	//Server starts checking if there is any connection to accept.
	//If so, it initializes client_sockfd
	while(1){
		sin_size = sizeof(struct sockaddr_in);
		if ((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &sin_size)) == -1) {
			perror("accept");
			continue;
		}
		printf("Server: connection received from %s\n", inet_ntoa(client_address.sin_addr));
		printf("Socket open on port %d.\n", ntohs(client_address.sin_port));

		//Starts a new process to handle connection
		int pid = fork();
		if (!pid) {
			//Child processes close server listener socket
			close(server_sockfd);
			while(1){
				//Child process stays listening for commands from client
				bzero(cmd_buf, sizeof(cmd_buf));
				if ((numbytes = recv(client_sockfd, cmd_buf, COMMAND_SIZE, 0)) == -1) {
					perror("recv");
					exit(1);
				}

				//Command and filename are separated in two (char *)
				cmd = strtok(cmd_buf, " ");
				filename = strtok(NULL, "\n");

				//If checks the command sent by the client
				if(strcmp("traer",cmd) == 0){
					//Sends file to client
					printf("Client requested download of file %s.\n", filename);

					FILE *fs = fopen(filename, "r");
					if(fs == NULL)
					{
					    printf("ERROR: File %s not found.\n", filename);
					    exit(1);
					}

					//Initializes a buffer to store pieces of the file to be sent
					char file_buffer[CHUNK_SIZE];
					//Reads from file into buffer, sends buffer to client, clears buffer,
					//until there is no more to be read from file
					int bytes_read = 0;
					while((bytes_read = fread(file_buffer, sizeof(char), CHUNK_SIZE, fs)) > 0){
						if(send(client_sockfd, file_buffer, bytes_read, 0) < 0){
							perror("send");
							exit(1);
						}
						bzero(file_buffer, CHUNK_SIZE);
					}
					fclose(fs);
				}else if(strcmp("subir",cmd) == 0){
					//Receives file from client
					printf("Client wants to upload file %s.\n", filename);
					FILE *fw = fopen(filename, "w");

					//Initializes a buffer to store pieces of the file to be received
					char file_buffer[CHUNK_SIZE];
					bzero(file_buffer, CHUNK_SIZE);

					//Reads from socket, writes bytes to file in server side, clears buffer,
					//until there is no more to be read from client
					long int total_bytes_read = 0;
					int bytes_read = 0;
					while((bytes_read = recv(client_sockfd, file_buffer, CHUNK_SIZE, 0)) > 0){
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
				}else {
					printf("Client sent an invalid command.\n");
				}
			}
			close(client_sockfd);
			exit(0);
		}
		close(client_sockfd);
	}
	return 0;
}
