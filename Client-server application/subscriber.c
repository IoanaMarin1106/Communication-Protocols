#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "header.h"
#include <math.h>
#include "queue.h"
#include "list.h"

/*
	Functia converteste payload-ul primit in mesajul dat de catre server
	intr-un integer.
*/
void convert_to_int(struct to_client_udp_message message_recv) {
	int value = 0;
				
	value = value + ((uint8_t)message_recv.message.payload[1] << 24);
	value = value + ((uint8_t)message_recv.message.payload[2] << 16);
	value = value + ((uint8_t)message_recv.message.payload[3] << 8);
	value = value + (uint8_t)message_recv.message.payload[4];

	if (message_recv.message.payload[0] != 0) {
		value = value * (-1);
	}

	fprintf(stdout, "%s:%d - %s - INT - %d\n", message_recv.udp_ip,
		message_recv.port, message_recv.message.topic, value);
	return;
}


/*
	Functia converteste payload-ul primit in mesajul dat de catre server
	intr-o variabila de tip float.
*/
void convert_to_float(struct to_client_udp_message message_recv) {
	float value = 0;
	value = ((uint8_t)(message_recv.message.payload[0]) << 8) + 
			(uint8_t)(message_recv.message.payload[1]);

	float result = (float)value/100;

	fprintf(stdout, "%s:%d - %s - SHORT_REAL - %.3f\n", message_recv.udp_ip,
			message_recv.port, message_recv.message.topic, result);
	return;
}


/*
	Functia converteste payload-ul primit in mesajul dat de catre server
	intr-o variabila de tip double.
*/
void convert_to_double(struct to_client_udp_message message_recv) {
	int value = 0;

	value = value + ((uint8_t)message_recv.message.payload[1] << 24);
	value = value + ((uint8_t)message_recv.message.payload[2] << 16);
	value = value + ((uint8_t)message_recv.message.payload[3] << 8);
	value = value + (uint8_t)message_recv.message.payload[4];


	if (message_recv.message.payload[0] != 0) {
		value = value * (-1);
	}

	double powOfTen = pow(10, (uint8_t)message_recv.message.payload[5]);
	double result = (double)value/powOfTen;

	fprintf(stdout, "%s:%d - %s - FLOAT - %lf\n", message_recv.udp_ip,
			message_recv.port, message_recv.message.topic, result);
	return;
}

/*
	Functie ce tratateaza situatia de eroare primita in payload-ul mesajului.
*/
int error_handler(struct to_client_udp_message message_recv) {

	if (strncmp(message_recv.message.payload, 
				"This id_user is already in use. Cannot connect to the server.", 61) == 0) {
		fprintf(stderr, "This id_user is already in use. Cannot connect to the server.\n");
		return 1;
	}

	if (strncmp(message_recv.message.payload, 
				"User is already subscribing this channel.", 41) == 0) {
		fprintf(stderr, "User is already subscribing this channel.\n");
		return 0;
	}

	if (strncmp(message_recv.message.payload, 
				"Updating SF... SF updated for this topic.", 43) == 0) {
		fprintf(stderr, "Updating SF... SF updated for this topic.\n");
		return 0;
	}

	if(strncmp(message_recv.message.payload,  
				"User is not abonate anymore at this topic.", 45) == 0) {
		fprintf(stderr,  "User is not abonate anymore at this topic.\n");
		return 0;
	}

	if(strncmp(message_recv.message.payload, 
				"Cannot unsubscribe. User has not this topic.", 46) == 0) {
		fprintf(stderr, "Cannot unsubscribe. User has not this topic.\n");
		return 0;
	}

	return -1;
}

/*
	Functie ce aboneaza clientul curent la topicul dat ca parametru comenzii
	"subscribe" in caz ca nu apare o situatie de eroare.
*/
void subscribe_topic(char buffer[BUFLEN], int server_socket) {
	char command_type[10];
	char topic[TOPIC_LEN];
	unsigned char SF = 2;

	sscanf(buffer, "%s %s %hhu", command_type, topic, &SF);

	if (strcmp(topic, "") == 0) {
		fprintf(stderr, "%s\n", "Invalid subscribe operation. No topic.");
		return;
	}

	/* 
		Verificare caz de eroare pentru parametrul SF.
		Nu poate lua alte valori decat 0 sau 1.
	*/
	if (SF != 0 && SF != 1) {
		fprintf(stderr, "Invalid subscribe operation. SF incorrect.\n");
		return;
	}

	struct client_tcp_message message;
	memcpy(message.command_type, command_type, COMMAND_LEN);
	memcpy(message.topic, topic, TOPIC_LEN);
	message.SF = SF;

	int send_fd = send(server_socket, &message, sizeof(struct client_tcp_message), 0);
	DIE(send_fd < 0, "send tcp_message");

	fprintf(stdout, "subscribe %s\n", topic);
	return;
}

int main(int argc, char const *argv[])
{
	/* Verificam numarul parametrilor din linia de comanda. */
	if (argc != 4) {
		fprintf(stderr, "Error. Number of arguments in command line.\n");
		exit(1);
	}

	int server_socket, ret;
	struct sockaddr_in server_addr;
	int fd_max;

	fd_set read_fds, 	/* Multimea de citire folosita in select().*/
			tmp_fds;	/* Multimea temporara. */

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	/* Socket-ul serverului. */
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	DIE(server_socket < 0, "socket");

	FD_SET(server_socket, &read_fds);
	FD_SET(0, &read_fds);

	fd_max = server_socket;

	/* Setam IP-ul si PORT-ul serverului. */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &server_addr.sin_addr);
	DIE(ret < 0, "inet_aton");

	ret = connect(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr));
	DIE(ret < 0, "connect_client");

	/* Trimitem ID-ul noului client creat catre server. */
	struct client_tcp_message message;
	memcpy(message.id_client, argv[1], ID_LEN);
	ret = send(server_socket, &message, sizeof(struct client_tcp_message), 0);
	DIE(ret < 0, "send_ip_abon");

	char buffer[BUFLEN];
	int send_fd;

	while (1) {
		tmp_fds = read_fds;
		ret = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select_client");

		/* User-ul a dat o comanda de la tastatura */
		if (FD_ISSET(0, &tmp_fds) != 0) {

			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			char command[COMMAND_LEN];
			sscanf(buffer, "%s ", command);

			/* In cazul in care am primit comanda "exit" */
			if (strncmp(buffer, "exit", 4) == 0) {
				break;

			} else if (strcmp(command, "subscribe") == 0) {
				subscribe_topic(buffer, server_socket);
			} else if (strcmp(command, "unsubscribe") == 0) {
				char command_type[12];
				char topic[TOPIC_LEN];

				memset(topic, 0, TOPIC_LEN);

				sscanf(buffer, "%s %s", command_type, topic);

				if (strcmp(topic, "") == 0) {
					fprintf(stderr, "%s\n", "Invalid unsubscribe operation. No topic.");
					continue;
				}

				struct client_tcp_message message;
				memcpy(message.command_type, command_type, COMMAND_LEN);
				memcpy(message.topic, topic, TOPIC_LEN);
				message.SF = -1;

				send_fd = send(server_socket, &message, sizeof(struct client_tcp_message), 0);
				DIE(send_fd < 0, "send tcp_message");

				fprintf(stdout, "unsubscribe %s\n", topic);
			} else {
				fprintf(stderr, "Invalid operation. Type unknown.\n");
				continue;
			}

		} else if (FD_ISSET(server_socket, &tmp_fds) != 0) {
			/* Cazul in care clientul primeste un mesaj de la server.*/

			struct to_client_udp_message message_recv;

			int recv_fd = recv(server_socket, &message_recv, sizeof(struct to_client_udp_message), 0);
			DIE(recv_fd < 0, "recv client udp");

			/* 	
				Cazul in care serverul a inchis conexiunea, asa ca a trimis mesaj
				catre toti clientii pentru a-si inchide conexiunea.
			*/
			if (recv_fd == 0) {
				break;
			}

			/* In cazul in care primim un mesaj de eroare de la server.*/
			if (error_handler(message_recv) == 0) {
				continue;
			} else if (error_handler(message_recv) == 1) {
				/* 	
					In cazul in care user-ul curent a incercat sa se logheze cu un
					user_id deja existent. Se inchide conexiunea user-ului ce a incercat
					o logare invalida.
				*/
				break;
			}

			/* 	
				Afisarea si convertirea datelor primite de la server prin intermediul
				clientilor UDP, in functie de fiecare tip de date.
			*/
			if (message_recv.message.data_type == 0) {
				convert_to_int(message_recv);
			} else if (message_recv.message.data_type == 1) {
				convert_to_float(message_recv);
			} else if (message_recv.message.data_type == 2) {
				convert_to_double(message_recv);
			} else if (message_recv.message.data_type == 3) {
				fprintf(stdout, "%s:%d - %s - STRING - %s\n", message_recv.udp_ip, 
						message_recv.port, message_recv.message.topic, message_recv.message.payload);
			}
		}
	}

	/* Inchidem socket-ul clientului.*/
	close(server_socket);
	return 0;
}