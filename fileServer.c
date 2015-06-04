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
#include "visuals.h"

#define PORT 7070
#define BACKLOG 5
#define MAXDATASIZE 100

void sigchld_handler(int s)
{
	while(wait(NULL) > 0);
}

int main(){
	int sockfd, new_fd;
	int buf[MAXDATASIZE];
	struct sockaddr_in server_address;
	struct sockaddr_in remote_address;
	int sin_size;
	int yes = 1;
	int numbytes;
	
	struct sigaction sa;
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	
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
		consoleColor(GREEN);
		printf("Server listening on address: %s:%d%s.\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port), RST);
	}
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
		consoleColor(GREEN);
		printf("server: got connection from %s\n%s", inet_ntoa(remote_address.sin_addr), RST);
		printf("Socket opened on port %d.\n", ntohs(remote_address.sin_port));
		if (!fork()) { // Este es el proceso hijo
			close(sockfd); // El hijo no necesita este descriptor
			if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
				perror("recv");
				exit(1);
			}
			buf[numbytes] = '\0';
			printf("Received: %s",buf);
			if (send(new_fd, "YA COMPARE\n", 14, 0) == -1)
				perror("send");
			close(new_fd);
			exit(0);
		}
		close(new_fd);
	}
	return 0;
}
