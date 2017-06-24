#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "err.h"

#define TRUE 1
#define FALSE 0

#define BUFFER_SIZE 2048
#define PORT 20160

#define MAX_MESSAGE_SIZE 1000
#define MAX_CLIENT 20

int finish = FALSE;
int socket_fd = 0;
int clients_count = 0;
int max_fd;
fd_set main_fd_set;

void connect_server(struct sockaddr_in *server, long port)
{

	int yes = 1;
		
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		syserr("socket");
	}
		
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		syserr("setsockopt");
	}

	server->sin_family = AF_INET;
	server->sin_port = htons(port);
	server->sin_addr.s_addr = htonl(INADDR_ANY);
	memset(server->sin_zero, '\0', sizeof server->sin_zero);

	if (bind(socket_fd, (struct sockaddr *)server, sizeof(struct sockaddr)) == -1) {
		syserr("bind");
	}

	if (listen(socket_fd, 10) == -1) {
		syserr("listen");
	}

	fflush(stdout);
}

void handle_new_client(struct sockaddr_in *client)
{

	socklen_t addr_len;
	int new_socket_fd;
	
	addr_len = sizeof(struct sockaddr_in);

	if ((new_socket_fd = accept(socket_fd, (struct sockaddr *)client, &addr_len)) == -1) {
		syserr("accept");
	} else {
		FD_SET(new_socket_fd, &main_fd_set);
		if (new_socket_fd > max_fd) {
			max_fd = new_socket_fd;
		}
		clients_count += 1;
		if (clients_count > MAX_CLIENT) {
			if (close(new_socket_fd) < 0) {
				syserr("close");
			}
			FD_CLR(new_socket_fd, &main_fd_set);
			clients_count -= 1;
		}
	}
}

void spread_msg(int j, int i, int nbytes_recvd, char *recv_buf)
{
	if (FD_ISSET(j, &main_fd_set)) {
		if (j != socket_fd && j != i) {
			if (send(j, recv_buf, nbytes_recvd, 0) == -1) {
				syserr("send");
			}
		}
	}
}
		
void handle_old_client(int i)
{

	int read_len, j;
	char recv_buf[BUFFER_SIZE];

	memset(&recv_buf[0], 0, sizeof(recv_buf));

	if ((read_len = recv(i, recv_buf, BUFFER_SIZE, 0)) <= 0) {
		if (read_len == 0) {
			if (close(i) < 0) {
				syserr("close");
			}
			FD_CLR(i, &main_fd_set);
			clients_count -= 1;
		} else {
			syserr("recv");
		}
	} else { 
		if (read_len > MAX_MESSAGE_SIZE) {
			if (close(i) < 0) {
				syserr("close");
			}
			FD_CLR(i, &main_fd_set);
			clients_count -= 1;
		} else {
			for (j = 0; j <= max_fd; j++) {
				spread_msg(j, i, read_len, recv_buf);
				fflush(stdout);
			}
		}
	}	
}

void shutdown_all_clients(int code)
{
	int j;

	for (j = 0; j <= max_fd; j++) {
		if (FD_ISSET(j, &main_fd_set)) {
			if (close(j) < 0) {
				syserr("close");
			}
			FD_CLR(j, &main_fd_set);
			clients_count -= 1;
		}
	}

	finish = TRUE;
	exit(code);
}

void handle_signal(int sig_number)
{
	if (sig_number == SIGINT) {
		shutdown_all_clients(EXIT_SUCCESS);
	} else if (sig_number == SIGPIPE) {
		shutdown_all_clients(EXIT_SUCCESS);
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

int main (int argc, char *argv[]) 
{	

	struct sockaddr_in server, client;
	fd_set read_fd_set;

	int i;
	char *rest;
	long port;

	if (setup_signal() != 0) {
		perror("setup_signal");
		exit(EXIT_FAILURE);
	}

	if (argc > 2) {
		fatal("Usage: %s [port]\n", argv[0]);
	}

	if (argc == 2) {
		port = strtol(argv[1], &rest, 10);
		if ((rest == argv[1]) || (*rest != '\0')) {
			fatal("'%s' is not valid port number\n", argv[1]);
		}
		if (port < 0) {
			fatal("'%s' is not valid port number\n", argv[1]);	
		}
	} else {
		port = PORT;
	}

	connect_server(&server, port);

	FD_ZERO(&main_fd_set);
	FD_ZERO(&read_fd_set);

	FD_SET(socket_fd, &main_fd_set);

	max_fd = socket_fd;
	while(!finish) {
		read_fd_set = main_fd_set;
		if (select(max_fd + 1, &read_fd_set, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}
		for (i = 0; i <= max_fd; i++) {
			if (FD_ISSET(i, &read_fd_set)) {
				if (i == socket_fd) {
					handle_new_client(&client);
				} else {
					handle_old_client(i);
				}
			}
		}
	}

	if (close(socket_fd)) {
		syserr("close");
	}

	return 0;
}

/*EOF*/
