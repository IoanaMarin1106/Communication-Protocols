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
#include <netinet/tcp.h>
#include "queue.h"
#include "list.h"

/* 
	Functie ce verifica daca serverul a primit comanda exit de la tasatura.
	In caz afirmativ returneaza 1, altfel 0.
*/
int server_closing() {
	char *command = malloc(sizeof(char) * 10);
	fscanf(stdin, "%s", command);

	if (!strcmp(command, "exit")) {
		fprintf(stdout,"Server shuts down correctly.\n");
		free(command);
		return 1;
	}
	free(command);
	return 0;
}

/* 
	Functia afiseaza un mesaj corespunzator pentru cazurile in care un client s-a conectat
	la server pentru prima data sau daca s-a reconectat.
*/
void connection_handler(
		int error, 
		int recconection,
		char id[ID_LEN], 
		struct sockaddr_in client_addr) {

	if (error == 0) {
		if (recconection == 0) {
			printf("New client %s connected from %s:%d.\n", id, 
					inet_ntoa(client_addr.sin_addr),
					ntohs(client_addr.sin_port));
		} else if (recconection == 1) {
			printf("Reconnect client %s connected from %s:%d.\n", id, 
					inet_ntoa(client_addr.sin_addr), 
					ntohs(client_addr.sin_port));
		}
	}
	return;
}

/* 
	Functia dezactiveaza algoritmul lui Neagle.
*/
void tcp_nodelay(int socket) {
	int yes = 1;
	int result = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof(int));
	DIE(result < 0, "setsockopt");
	return;
}

/*
	Functie ce gestioneaza erorile posibil aparute legate de client, trimitand
	un mesaj acestuia ce contine in payload eroarea aparuta.
*/
void error_handler(int socket, char error_message[1500]) {
	struct to_client_udp_message send_message;
	strcpy(send_message.message.payload, error_message);

	int send_fd = send(socket, &send_message, sizeof(struct to_client_udp_message), 0);
	DIE(send_fd < 0, "send message");
	return;
}

/* 
	Functie ce realoca spatiu pentru baza de date a clientilor conectati.
*/
void realloc_clients(struct tcp_client **clients, int *max_clients_number) {
	*max_clients_number *= 2;

	struct tcp_client *aux = (struct tcp_client*)realloc(*clients, 
							sizeof(struct tcp_client) * (*max_clients_number));
	if (aux == NULL) {
		fprintf(stderr, "Unable to reallocate memory for clients.\n");
		exit(1);
	}

	*clients = aux;
}

/*
	Functie ce pune in coada de mesaje a unui client, update-urile despre topicurile
	la care acesta este abonat, in timpul cat acesta este deconectat de la server.
	Se aplica numai pentru topicurile ce au SF setat cu 1.
*/
void put_in_queue(queue q, struct to_client_udp_message recv_msg) {

	struct to_client_udp_message *msg = (struct to_client_udp_message*)malloc(sizeof(struct to_client_udp_message));
	if (msg == NULL) {
		fprintf(stderr, "Unable to allocate memory for message.\n");
		exit(1);
	}

	strcpy(msg->udp_ip, recv_msg.udp_ip);
	msg->port = recv_msg.port;
	strcpy(msg->message.topic, recv_msg.message.topic);
	msg->message.data_type = recv_msg.message.data_type;

	if (recv_msg.message.data_type != 3) {
		memcpy(msg->message.payload, recv_msg.message.payload, PAYLOAD_LEN);
	} else {
		strcpy(msg->message.payload, recv_msg.message.payload);
	}

	queue_enq(q, msg);
	return;
}

/*
	Functie ce trimite toate mesajele din coada unui client de mesaje, in
	momentul in care acesta se reconecteaza la server.
	La iesirea din functie coada ramane goala.
*/
void send_updates(queue q, int client_socket) {

	while (!queue_empty(q)) {
		struct to_client_udp_message *msg = queue_deq(q);

		struct to_client_udp_message send_message;

		strcpy(send_message.udp_ip, msg->udp_ip);
		send_message.port = msg->port;
		strcpy(send_message.message.topic, msg->message.topic);
		send_message.message.data_type = msg->message.data_type;

		if (msg->message.data_type != 3) {
			memcpy(send_message.message.payload, msg->message.payload, PAYLOAD_LEN);
		} else {
			strcpy(send_message.message.payload, msg->message.payload);
		}

		int send_fd = send(client_socket, msg, sizeof(struct to_client_udp_message), 0);
		DIE(send_fd < 0, "send_fd");

		free(msg);
	}
	return;
}

int main(int argc, char const *argv[])
{
	/* Verificam numarul parametrilor din linia de comanda. */
	if (argc != 2) {
		fprintf(stderr, "Error. Number of arguments in command line.\n");
		exit(1);
	}

	fd_set read_fds, 	/* Multimea de citire folosita in select() */
			tmp_fds; 	/* Multime folosita temporar. */

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	int udp_socket;	/* Socket-ul pentru clientii UDP.*/
	int ret_udp;

	/* Creerea socket-ului UDP. */
	udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

	int tcp_socket, 	/* Socket-ul pasiv TCP al serverului.*/
		portno;
	int ret;
	struct sockaddr_in server_addr;

	/* Creearea socket-ului TCP pasiv.*/
	tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_socket < 0, "tcp_socket");

	portno = atoi(argv[1]);
	DIE(portno < 0, "atoi");

	/* Setarea IP-ului si PORT-ului server-ului.*/
	memset((char*) &server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portno);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(tcp_socket, (struct sockaddr*) &server_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret_udp = bind(udp_socket, (struct sockaddr*) &server_addr, sizeof(struct sockaddr));
	DIE(ret_udp < 0, "bind");

	ret = listen(tcp_socket, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	int fdmax;

	FD_SET(tcp_socket, &read_fds);
	FD_SET(udp_socket, &read_fds);
	FD_SET(0, &read_fds);

	if (tcp_socket <= udp_socket) {
		fdmax = udp_socket;
	} else {
		fdmax = tcp_socket;
	}

	int clients_number = 0;
	int max_clients_number = 2;
	int active_clients = 0;

	/* Baza de date ce va tine evidenta clientilor conectati si care se vor reconecta.*/
	struct tcp_client *clients = (struct tcp_client*)malloc(sizeof(struct tcp_client) * 
															max_clients_number);
	if (clients == NULL) {
		fprintf(stderr, "Unable to alloc memory for clients.");
		exit(1);
	}

	struct sockaddr_in client_addr;
	int new_sockfd;		/* Socket-ul active pentru fiecare dintre clienti.*/

	while(1) {
		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		/* Se primesc mesaje de la stdin. */
		if (FD_ISSET(0, &tmp_fds)) {
			if (server_closing() == 1) {
				break;
			}
			continue;

		} else if (FD_ISSET(tcp_socket, &tmp_fds)) {
			/* Se primesc mesaje pe socket-ul pasiv de TCP */

			socklen_t clilen = sizeof(client_addr);
			new_sockfd = accept(tcp_socket, (struct sockaddr*) &client_addr, &clilen);
			DIE(new_sockfd < 0, "accept");

			struct client_tcp_message message_recv;
			ret = recv(new_sockfd, &message_recv, sizeof(message_recv), 0);
			DIE(ret < 0, "recv_client_connect");

			tcp_nodelay(new_sockfd);
			FD_SET(new_sockfd, &read_fds);

			if (new_sockfd > fdmax) {
				fdmax = new_sockfd;
			}

			int isReconnected = 0, /* Variabila ce va semnifica 
									reconectarea unui client.*/
				error = 0;		/* In caz de eroare, valoarea "error"
								va fi setate pe 1.*/

			/* 
				In cazul in care numarul clientilor activi la momentul curent
				este egal cu numarul maxim de clienti activi posibili,
				atunci se redimensioneaza baza de date.
			*/
			if(active_clients == max_clients_number) {
				realloc_clients(&clients, &max_clients_number);
			}

			/* Verificam daca un client vrea sa se reconecteze. */
			for (int i = 0; i < clients_number; i++) {

				if (clients[i].isConnected == 0 && 
					strcmp(clients[i].id_client, message_recv.id_client) == 0) {
			
					isReconnected = 1;
					
					clients[i].isConnected = 1;
					clients[i].socket = new_sockfd;
					active_clients += 1;

					/* 
						In cazul in care s-a reconectat si are mesaje ce trebuiesc
						trimise, serverul face acest lucru si goleste coada de 
						mesaje a user-ului.
					*/
					send_updates(clients[i].message_queue, clients[i].socket);

				} else if (clients[i].isConnected == 1 &&
							strcmp(clients[i].id_client, message_recv.id_client) == 0) {
					error = 1;
					error_handler(
						new_sockfd, 
						"This id_user is already in use. Cannot connect to the server.");
					FD_CLR(new_sockfd, &read_fds);
					continue;
				}
			}

			/* Cazul in care este un client nou. */
			if (isReconnected == 0 && error == 0) {
				/* 
					Setam campurile specifice clientului nou si alocam spatiu pentru
					vectorul de topicuri.
				*/
				memcpy(clients[clients_number].id_client, message_recv.id_client, 
											strlen(message_recv.id_client));
				clients[clients_number].socket = new_sockfd;
				clients[clients_number].isConnected = 1;
				clients[clients_number].max_topics = 2;
				clients[clients_number].topics = (struct topic*)malloc(sizeof(struct topic) * 
													clients[clients_number].max_topics);

				if (clients[clients_number].topics == NULL) {
					fprintf(stderr, "Unable to alloc memory for topics");
					exit(1);
				}

				clients[clients_number].topics_number = 0;
				clients[clients_number].active_topics_number = 0;
				clients[clients_number].message_queue = queue_create();

				active_clients += 1;
				clients_number += 1;
			}

			connection_handler(error, isReconnected, 
							message_recv.id_client, client_addr);

		} else if (FD_ISSET(udp_socket, &tmp_fds)) {
			/* Se primesc mesaje de la clientii de UDP. */

			/* 
				Structura in care se vor primi IP-ul si PORT-ul
				clientului UDP.
			*/
			struct sockaddr_in from_udp_client;

			/* Structura in care se va primi mesajul de la clientul UDP.*/
			struct udp_message message_recv;

			/* Structura in care se va trimite mesajul catre clientii TCP.*/
			struct to_client_udp_message server_message;

			socklen_t length = sizeof(from_udp_client);

			memset(message_recv.topic, 0, TOPIC_LEN);
			memset(message_recv.payload, 0, PAYLOAD_LEN);

			/* 	
				Se primeste IP-ul si PORT-ul clientului UDP si 
				mesajul pe care acesta il trimite catre server.
			*/
			ret = recvfrom(udp_socket, &message_recv, sizeof(struct udp_message), 0,
							(struct sockaddr*) &from_udp_client, &length);	
			DIE(ret < 0, "recvfrom");


			memcpy(server_message.message.topic, message_recv.topic, TOPIC_LEN);
			server_message.message.data_type = message_recv.data_type;

			/* Pentru a adauga in fiecare caz terminatorul de sir. */
			if (message_recv.data_type == 3) {
				char aux_payload[PAYLOAD_LEN + 1];
				strcpy(aux_payload, message_recv.payload);
				strcat(aux_payload, "\0");
				strcpy(server_message.message.payload, aux_payload);
			} else {
				memcpy(server_message.message.payload, message_recv.payload, 
						PAYLOAD_LEN);
			}

			server_message.port = htons(from_udp_client.sin_port);
			memcpy(server_message.udp_ip, inet_ntoa(from_udp_client.sin_addr), IP_LEN);

			for (int i = 0; i < clients_number; i++) {
				struct tcp_client *client = &(clients[i]);

				for (int j = 0; j < client->topics_number; j++) {
					if (strcmp(client->topics[j].title, message_recv.topic) == 0 &&
						client->topics[j].isSubscribed == 1) {

						if (client->isConnected == 1) {
							int send_fd = send(client->socket, &server_message, 
											sizeof(struct to_client_udp_message), 0);
							DIE(send_fd < 0, "send_udp_message");
						} 

						/* 	
							In cazul in care e deconectat dar are SF setat pe 1, 
							retinem in coada update-urile primite de la clientii UDP.
						*/
						if (client->isConnected == 0 && client->topics[j].SF == 1) {
							put_in_queue(client->message_queue, server_message);
						}
					}
				}
			}

		} else {
			/* 
				Se primesc mesaje de la clientii de TCP, pe un socket pasiv,
				specific pentru fiecare client.
			*/
			for (int i = 0; i < clients_number; i++) {
				struct tcp_client *client = &(clients[i]);

				/* 	
					Verificam pe care socket activ am primit mesaj,
					adica de la ce client am primit vreun mesaj.
				*/
				if (FD_ISSET(client->socket, &tmp_fds)) {
					struct client_tcp_message message_recv;
					ret = recv(client->socket, &message_recv, sizeof(struct client_tcp_message), 0);
					DIE(ret < 0, "recv message");

					/* In cazul in care un client s-a deconectat.*/
					if (ret == 0) { 
						printf("Client %s disconnected.\n", client->id_client);
						client->isConnected = 0;
						active_clients -= 1;

						FD_CLR(client->socket, &read_fds);

					} else if (strncmp(message_recv.command_type, "subscribe", 9) == 0) {
						/* Un client a trimis o comanda de abonare la un topic.*/

						int error = 0;
						int current_topic = client->topics_number;

						/* 
							User-ul este deja abonat la un topic, 
							nu se mai doreste o abonare in plus. 
						*/
						for (int i = 0 ; i < current_topic; i++) {
							if (client->topics[i].isSubscribed == 1 &&
								strcmp(client->topics[i].title, message_recv.topic) == 0) {

								if (message_recv.SF == client->topics[i].SF) {
									error_handler(
										client->socket, 
										"User is already subscribing this channel.");
									error = 1;
								} else {
									error_handler(
										client->socket, 
										"Updating SF... SF updated for this topic.");
									client->topics[i].SF = message_recv.SF;
									error = 1;
								}
							} 
						} 

						if (error == 0) {
							/* 
								Se redimensioneaza vectorul de topicuri in caz ca
								s-a atins numarul maxim de topicuri permise.
							*/ 
							if (client->active_topics_number >= client->max_topics) {
								client->max_topics *= 2;

								struct topic **topics = &(client->topics);
								struct topic *aux = (struct topic*)realloc(*topics, 
														sizeof(struct topic) * client->max_topics);
								*topics = aux;
							}
							strcpy(client->topics[current_topic].title, message_recv.topic);
							client->topics[current_topic].SF = message_recv.SF;
							client->topics[current_topic].isSubscribed = 1;
							client->topics_number += 1;
							client->active_topics_number += 1;
						} else {
							continue;
						}

					} else if (strncmp(message_recv.command_type, "unsubscribe", 11) == 0) {
						int topics_size = client->topics_number;
						int ok = 0; /* Variabila setata pe 0 daca nu am gasit topic-ul 
										de la care se doreste dezabonarea.*/

						for (int i = 0; i < topics_size; i++) {
							if (strcmp(client->topics[i].title, message_recv.topic) == 0) {
								if (client->topics[i].isSubscribed == 1) {
									client->topics[i].isSubscribed = 0;
									client->active_topics_number -= 1;
									ok = 1;
								} else {
									ok = 1;
									error_handler(
										client->socket,  
										"User is not subscribing anymore this topic.");
									continue;
								}
							}
						}

						if (ok == 0) {
							error_handler(
								client->socket, 
								"Cannot unsubscribe. User has never subscribed this topic.");
						}
					}
				}
			}
		}
	}

	for(int i = 0; i < clients_number; i++) {
		free(clients[i].topics);
		while(!queue_empty(clients[i].message_queue)) {
			free(queue_deq(clients[i].message_queue));
		}
		free(clients[i].message_queue);
	}

	free(clients);

	close(udp_socket);
	close(tcp_socket);
	return 0;
}