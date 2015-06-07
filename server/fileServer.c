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

//Socket constants
#define PORT 7070
#define BACKLOG 5

//File transfer constants
#define MAXDATASIZE 100				//Buffer size in bytes for file transfer
#define COMMAND_SIZE 256			//Buffer size in bytes for client commands
#define CHUNK_SIZE 128				//Number of blocks read from files to send or receive

void sigchld_handler(int s)
{
	while(wait(NULL) > 0);
}

int main(){
	int sockfd, new_fd;
	int buf[MAXDATASIZE];
	char cmd_buf[COMMAND_SIZE];
	struct sockaddr_in server_address;
	struct sockaddr_in remote_address;
	int sin_size;
	int yes = 1;
	int numbytes;

	char *cmd;
	char *filename;

	struct sigaction sa;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	//Server socket configuration
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;
	memset(&(server_address.sin_zero), '\0', 8);

	if (bind(sockfd, (struct sockaddr *)&server_address, sizeof(struct sockaddr)) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}else{
		printf("Server listening on address: %s:%d.\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));
	}

	//Sigaction clears dead processes
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while(1){
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&remote_address, &sin_size)) == -1) {
			perror("accept");
			continue;
		}
		printf("server: got connection from %s\n", inet_ntoa(remote_address.sin_addr));
		printf("Socket open on port %d.\n", ntohs(remote_address.sin_port));
		//Starts a new process to handle connection
		if (!fork()) {
			//Closes server listener socket
			close(sockfd);

			//Child process stays listening for commands
			bzero(cmd_buf, sizeof(cmd_buf));
			if ((numbytes = recv(new_fd, cmd_buf, COMMAND_SIZE, 0)) == -1) {
				perror("recv");
				exit(1);
			}

			//Command and filename are separated in two char *
			cmd = strtok(cmd_buf, " ");
			filename = strtok(NULL, "\n");

			if(strcmp("traer",cmd) == 0){
				printf("Client requested update of file %s.\n", filename);

				FILE *fs = fopen(filename, "r");
				if(fs == NULL)
				{
				    printf("ERROR: File %s not found.\n", filename);
				    exit(1);
				}

				char file_buf[CHUNK_SIZE];
				int block_size = 0;
				while((block_size = fread(file_buf, sizeof(char), CHUNK_SIZE, fs)) > 0){
					if(send(new_fd, file_buf, block_size, 0) < 0){
						perror("send");
						exit(1);
					}
					bzero(file_buf, CHUNK_SIZE);
				}
			}else if(strcmp("subir",cmd) == 0){
				printf("Client wants to upload file %s.\n", filename);
			}else {
				printf("Client sent an invalid command.\n");
			}
			/*
			if (send(new_fd, "YA COMPARE\n", 14, 0) == -1)
				perror("send");*/
			close(new_fd);
			exit(0);
		}
		close(new_fd);
	}
	return 0;
}
