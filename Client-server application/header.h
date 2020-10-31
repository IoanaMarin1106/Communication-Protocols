#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include "queue.h"
#include "queue.c"
#include "list.h"
#include "list.c"


#define TOPIC_LEN 50
#define PAYLOAD_LEN 1500
#define MAX_TOPICS 99999999
#define ID_LEN 10
#define IP_LEN 16
#define COMMAND_LEN 11


/* Mesaj de tip UDP pe ruta client_udp -> server */
struct udp_message {
	char topic[TOPIC_LEN];			/* titlul topicului */
	unsigned char data_type;		/* tipul de date */
	char payload[PAYLOAD_LEN];		/* continutul mesajului */
};

/* Mesaj de tip UDP pe ruta server->client tcp */
struct to_client_udp_message {
	char udp_ip[IP_LEN];			/* Ip-ul clientului UDP */
	int port;						/* Port-ul cientului UDP */
	struct udp_message message;		/* Mesaj de tip UDP */
};

/* Mesaj de tip tcp pe ruta client->server */
struct client_tcp_message {
	char command_type[COMMAND_LEN];		/* Tipul comenzii */
	char topic[TOPIC_LEN];				/* Titlul topicului */
	char id_client[ID_LEN];				/* Id-ul clientului */
	unsigned int SF;					/* parametrul store & forward: 0/1 */
};

/* Structura pentru un topic */
struct topic {
	unsigned char SF;			/* parametrul store & forward: 0 sau 1*/
	char title[TOPIC_LEN];		/* titlul topicului */
	int isSubscribed; 			/* variabila setata cu 1 daca clientul
									este abonat la topic, 0 altfel */
};

/* Structura pentru un client de tip TCP */
struct tcp_client {
	int socket;					/* socketul clientului */
	char id_client[ID_LEN];		/* id-ul clientului */
	struct topic *topics;		/* lista de topicuri la care este abonat clientul*/
	int active_topics_number;	/* numarul de topicuri la care clientul este 
									abonat in momentul prezent */
	int topics_number;			/* numarul total de topicuri */
	int max_topics;				/* numarul maxim de topicuri la care 
								   se poate abona un client*/
	int isConnected; 			/*variabila setata cu 1 daca clientul este conectat,
						          0 daca clientul nu este conectat.*/
	queue message_queue;		/* coada de mesaje pastrate in cazul algoritmului SF.*/
};

