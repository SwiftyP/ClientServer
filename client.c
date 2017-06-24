#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>

#include "err.h"

#define TRUE 1
#define FALSE 0

#define MAX_MESSAGE_SIZE 1000

#define BUFFER_SIZE 2048
#define PORT 20160

int socket_fd;

void shutdown_client(int code)
{
	if (close(socket_fd) < 0) {
		syserr("close");
	}
	exit(code);
}

void handle_signal(int sig_number)
{
	if (sig_number == SIGINT) {
		shutdown_client(EXIT_SUCCESS);
	} else if (sig_number == SIGPIPE) {
		shutdown_client(EXIT_SUCCESS);
	}
}

int setup_signal()
{
	struct sigaction sa;
	sa.sa_handler = handle_signal;
	
	if (sigaction(SIGINT, &sa, 0) != 0) {
		perror("sigaction()");
		return -1;
	}
	if (sigaction(SIGPIPE, &sa, 0) != 0) {
		perror("sigaction()");
		return -1;
	}
	return 0;
}

void connect_with_server(int *socket_fd, struct sockaddr_in *server, char* host, long port) 
{

	struct hostent *h;

	if ((*socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		syserr("socket");
	}

	if ((h = gethostbyname(host)) == NULL) {
		syserr("gethostbyname");
	}

	memcpy(&server->sin_addr, h->h_addr_list[0], h->h_length);
	server->sin_family = AF_INET;
	server->sin_port = htons(port);
	
	memset(server->sin_zero, '\0', sizeof server->sin_zero);
	
	if (connect(*socket_fd, (struct sockaddr *)server, sizeof(struct sockaddr)) == -1) {
		syserr("connect");
	}
}

void send_msg(int socket_fd)
{
	char read_buf[BUFFER_SIZE];
	char clear_buf[BUFFER_SIZE];
	size_t read_len;
	uint16_t l;
	
	memset(&read_buf[0], 0, sizeof(read_buf));
	memset(&clear_buf[0], 0, sizeof(clear_buf));

	fgets(read_buf, BUFFER_SIZE, stdin);
	fflush(stdin);
	read_len = strlen(read_buf);

	if (read_len > MAX_MESSAGE_SIZE) {
		shutdown_client(EXIT_SUCCESS);
	}

	if (read_buf[read_len - 1] == '\n' || read_buf[read_len - 1] == '\0') {
		strncpy(clear_buf, read_buf, read_len - 1);
		l = htons(strlen(clear_buf));
		send(socket_fd, &l, sizeof(uint16_t), 0);
		send(socket_fd, clear_buf, strlen(clear_buf), 0);
	}
}

void recive_msg(int socket_fd)
{

	char recv_buf[BUFFER_SIZE];
	uint16_t l;
	int read_len;
	memset(&recv_buf[0], 0, sizeof(recv_buf));

	if (recv(socket_fd, &l, sizeof(uint16_t), 0) == 0) {
		shutdown_client(100);
	} else {
		read_len = ntohs(l);
		recv(socket_fd, recv_buf, read_len, 0);
		printf("%s\n" , recv_buf);
		fflush(stdout);
	}
}

int main(int argc, char *argv[]) 
{	

	struct sockaddr_in server;
	fd_set main_fd_set;
	fd_set read_fd_set;

	int max_fd, i;
	long port;
	char* rest;

	if (setup_signal() != 0) {
		perror("setup_signal");
		exit(EXIT_FAILURE);
	}

	if (argc > 3 || argc < 2) {
		fatal("Usage: %s host [port]\n", argv[0]);
	}

	if (argc == 3) {
		port = strtol(argv[2], &rest, 10);
		if ((rest == argv[2]) || (*rest != '\0')) {
			fatal("'%s' is not valid port number\n", argv[2]);
		}
		if (port < 0) {
			fatal("'%s' is not valid port number\n", argv[2]);	
		}
	} else {
		port = PORT;
	}

	connect_with_server(&socket_fd, &server, argv[1], port);

	FD_ZERO(&main_fd_set);
	FD_ZERO(&read_fd_set);
    
	FD_SET(0, &main_fd_set);
	FD_SET(socket_fd, &main_fd_set);
	
	max_fd = socket_fd;
	
	while (TRUE) {
		read_fd_set = main_fd_set;
		if (select(max_fd + 1, &read_fd_set, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}
		for (i = 0; i <= max_fd; i++) {
			if (FD_ISSET(i, &read_fd_set)) {
				if (i == 0) {
					send_msg(socket_fd);
				} else {
					recive_msg(socket_fd);
				}
			}
		}
	}

	if (close(socket_fd) < 0) {
		syserr("close");
	}

	return 0;
}

/*EOF*/