#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

/* Functie ce inregistreaza un client.*/
void register_client(
	int sockfd,
	char username[50], 
	char password[50], 
	char **body_data,
	int body_data_fields_count,
	char *host,
	char *url,
	char *content_type) {

	/* Se alcatuieste payload-ul mesajului.*/
	memcpy(body_data[0], "username", 8);
	memcpy(body_data[1], username, 50);
	memcpy(body_data[2], "password", 8);
	memcpy(body_data[3], password, 50);

	char *message;
	char *response;

	/* Se compune mesajul in format JSON ce va fi trimis catre server.*/
	message = compute_post_request(host, url, content_type, body_data,
									body_data_fields_count, NULL, 0, 
									NULL, 0);
	send_to_server(sockfd, message);
	response = receive_from_server(sockfd);

	/* Se extrage statusul din raspunsul serverului.*/
	char status[3];
	strncpy(status, response + 9, 3);

	/* Se verifica statusul raspunsului.*/
	if (strcmp(status, "201") == 0) {
		fprintf(stdout, "The registration was successful.\n");
	} else {
		char *payload;
		payload = strstr(response, "{");
		fprintf(stdout, "%s\n", payload);
	}

	return;
}

/* Functie ce realizeaza autentificarea unui client.*/
void login_client(
	int sockfd,
	char username[50], 
	char password[50], 
	char **body_data,
	int body_data_fields_count,
	char *host,
	char *url,
	char *content_type,
	char cookie_res[400],
	int *is_logged_user) {

	/* Se alcatuieste payload-ul mesajului.*/
	memcpy(body_data[0], "username", 8);
	memcpy(body_data[1], username, 50);
	memcpy(body_data[2], "password", 8);
	memcpy(body_data[3], password, 50);

	char *message;
	char *response;

	/* Se compune mesajul in format JSON ce va fi trimis catre server.*/
	message = compute_post_request(host, url, content_type, body_data,
									body_data_fields_count, NULL, 0,
									NULL, 0);
	send_to_server(sockfd, message);
	response = receive_from_server(sockfd);

	/* Se extrage statusul din raspunsul serverului.*/
	char status[3];
	strncpy(status, response + 9, 3);

	/* Se verifica statusul raspunsului.*/
	if (strcmp(status, "200") == 0) {
		fprintf(stdout, "OK. Login successful.\n");
		char *cookie = strstr(response, "connect");
		cookie = strtok(cookie, ";");
		memcpy(cookie_res, cookie, 400);
		*is_logged_user = 1;
	} else {
		char *payload;
		payload = strstr(response, "{");
		fprintf(stdout, "%s\n", payload);
		*is_logged_user = 0;
	}

	return;
}

/* Functie ce cere acces in biblioteca.*/
void enter_library(
	int sockfd,
	char **cookies,
	int cookies_count,
	char *host,
	char *url,
	char token[500]) {

	char *message;
	char *response;

	/* Se compune mesajul in format JSON ce va fi trimis catre server.*/
	message = compute_get_request(host, url, "", cookies, cookies_count, NULL, 0);
	send_to_server(sockfd, message);
	response = receive_from_server(sockfd);

	/* Se extrage statusul din raspunsul serverului.*/
	char status[3] = "";
	strncpy(status, response + 9, 3);
	
	/* Se verifica statusul raspunsului.*/
	if (strcmp(status, "200") == 0) {
		fprintf(stdout, "OK. You enter library.\n");

		char *string = (strstr(response, "{")) + 10;
		string = strtok(string, "\"");
		memcpy(token, string, 500);
	} else if (strcmp(status, "401") == 0) {
		fprintf(stdout, "You are not logged in!\n");
	}else {
		char *payload;
		payload = strstr(response, "{");
		fprintf(stdout, "%s\n", payload);
	}

	return;
}

/* Functie ce va afisa informatii despre cartile de pe server.*/
void get_books(
	int sockfd,
	char *host,
	char *url,
	char **tokens,
	int tokens_count) {

	char *message;
	char *response;

	/* Se compune mesajul in format JSON ce va fi trimis catre server.*/
	message = compute_get_request(host, url, "", NULL, 0, 
								tokens, tokens_count);
	send_to_server(sockfd, message);
	response = receive_from_server(sockfd);

	/* Se extrage statusul din raspunsul serverului.*/
	char status[3] = "";
	strncpy(status, response + 9, 3);

	/* Se verifica statusul raspunsului.*/
	if (strcmp(status, "200") == 0) {
		char *payload;
		payload = strstr(response, "{");

		if (payload == NULL) {
			fprintf(stdout, "You don't have any books.\n");
			return;
		}

		fprintf(stdout, "OK. These are your books.\n");
		fprintf(stdout, "%s\n", payload);
	} else if (strcmp(status, "500") == 0) {
		fprintf(stdout, "Error when decoding token!\n");
	} else {
		char *payload;
		payload = strstr(response, "{");
		fprintf(stdout, "%s\n", payload);
	}

	return;
}

/* 
	Functie ce va afisa informatii despre cartea al carei id este citit
	de la tastatura.
*/
void get_book(
	int sockfd,
	char *host,
	char *url,
	char **tokens,
	int tokens_count) {

	char *message;
	char *response;

	/* Se compune mesajul in format JSON ce va fi trimis catre server.*/
	message = compute_get_request(host, url, "", NULL, 0, 
								tokens, tokens_count);
	send_to_server(sockfd, message);
	response = receive_from_server(sockfd);

	/* Se extrage statusul din raspunsul serverului.*/
	char status[3] = "";
	strncpy(status, response + 9, 3);

	/* Se verifica statusul raspunsului.*/
	if (strcmp(status, "200") == 0) {
		char *payload;
		payload = strstr(response, "{");
		fprintf(stdout, "%s\n", payload);
	} else if (strcmp(status, "500") == 0) {
		fprintf(stdout, "Error when decoding token!\n");
	} else {
		char *payload;
		payload = strstr(response, "{");
		fprintf(stdout, "%s\n", payload);
	}
	return;
}

/* Functie ce adauga o carte pe server.*/
void add_book(
	int sockfd,
	char title[100],
	char author[100],
	char genre[100],
	char publisher[100],
	char page_count[10],
	char **body_data,
	int body_data_fields_count,
	char *host,
	char *url,
	char *content_type,
	char **tokens,
	int tokens_count) {

	/* Se alcatuieste payload-ul mesajului.*/
	strcpy(body_data[0], "title");
	strcpy(body_data[1], title);
	strcpy(body_data[2], "author");
	strcpy(body_data[3], author);
	strcpy(body_data[4], "genre");
	strcpy(body_data[5], genre);
	strcpy(body_data[6], "page_count");
	strcpy(body_data[7], page_count);
	strcpy(body_data[8], "publisher");
	strcpy(body_data[9], publisher);

	char *message;
	char *response;

	/* Se compune mesajul in format JSON ce va fi trimis catre server.*/
	message = compute_post_request(host, url, content_type, body_data,
									body_data_fields_count, NULL, 0, tokens,
									tokens_count);
	send_to_server(sockfd, message);
	response = receive_from_server(sockfd);

	/* Se extrage statusul din raspunsul serverului.*/
	char status[3];
	strncpy(status, response + 9, 3);

	/* Se verifica statusul raspunsului.*/
	if (strcmp(status, "200") == 0) {
		fprintf(stdout, "OK. Your book is added successful.\n");
	}  else if (strcmp(status, "500") == 0) {
		fprintf(stdout, "Error when decoding token!\n");
	} else {
		char *payload;
		payload = strstr(response, "{");
		fprintf(stdout, "%s\n", payload);
	}

	return;
}

/* Functie ce sterge o carte de pe server.*/
void delete_book(
	int sockfd,
	char *host, 
	char *url, 
	char **tokens,
	int tokens_count) {

	char *message;
	char *response;

	/* Se compune mesajul in format JSON ce va fi trimis catre server.*/
	message = compute_delete_request(host, url, "", NULL, 0, tokens, tokens_count);
	send_to_server(sockfd, message);
	response = receive_from_server(sockfd);

	/* Se extrage statusul din raspunsul serverului.*/
	char status[3] = "";
	strncpy(status, response + 9, 3);

	/* Se verifica statusul raspunsului.*/
	if (strcmp(status, "200") == 0) {
		fprintf(stdout, "OK. Book is deleted.\n");
	}  else if (strcmp(status, "500") == 0) {
		fprintf(stdout, "Error when decoding token!\n");
	} else {
		char *payload;
		payload = strstr(response, "{");

		if (strcmp(status, "404") == 0) {
			fprintf(stdout, "Invalid ID. The book does not exist. No book was deleted.\n");
		} else {
			fprintf(stdout, "%s\n", payload);
		}
	}
	return;
}

/* Functie ce realizeaza delogarea clientului.*/
void logout(
	int sockfd,
	char *host,
	char *url,
	char **cookies,
	int cookies_count) {

	char *message;
	char *response;

	/* Se compune mesajul in format JSON ce va fi trimis catre server.*/
	message = compute_get_request(host, url, "", cookies, cookies_count, NULL, 0);
	send_to_server(sockfd, message);
	response = receive_from_server(sockfd);

	/* Se extrage statusul din raspunsul serverului.*/
	char status[3] = "";
	strncpy(status, response + 9, 3);

	/* Se verifica statusul raspunsului.*/
	if (strcmp(status, "200") == 0) {
		fprintf(stdout, "OK. Logout successful.\n");
	} else {
		char *payload;
		payload = strstr(response, "{");
		fprintf(stdout, "%s\n", payload);
	}

	return;
}

/* Functie ce elibereaza memoria alocata.*/
void free_data(char **body_data, char **cookies) {
	int i;

	/* Eliberare memorie alocata pentru "body_data".*/
	for (i = 0; i < 10; i++) {
        free(body_data[i]);
    }
    free(body_data);

    /* Eliberarea memorie alocata pentru cookie-uri.*/
    for (i = 0; i < 10; i++) {
        free(cookies[i]);
    }
    free(cookies);

    return;
}

int main(int argc, char *argv[])
{

    int sockfd;

    /* Alocam spatiu pentru datele ce vor fi convertite in format JSON.*/
    char **body_data = (char**)calloc(10, sizeof(char*));
    if (body_data == NULL) {
        perror("Unable to allocate memory for body_data");
    }
    for (int i = 0; i < 10; i++) {
        body_data[i] = (char*)calloc(100, sizeof(char));
        if (body_data[i] == NULL) {
            perror("Unable to allocate memoru for data");
            exit(1);
        }
    }

    /* Alocam memorie pentru cookie-uri.*/
    char **cookies = (char**)calloc(10, sizeof(char*));
    if (cookies == NULL) {
        perror("Unable to allocate memory for cookies");
        exit(1);
    }
    for (int i = 0; i < 10; i++) {
        cookies[i] = (char*)calloc(400, sizeof(char));
        if (cookies[i] == NULL) {
            perror("Unable to allocate memory for cookie");
            exit(1);
        }
    }

    /* Alocam memorie pentru token-uri.*/
    char **tokens = (char**)calloc(10, sizeof(char*));
    if (tokens == NULL) {
    	perror("Unable to allocate memory for tokens.");
    	exit(1);
    }
    for (int i = 0; i < 10; i++) {
        tokens[i] = (char*)calloc(400, sizeof(char));
        if (tokens[i] == NULL) {
            perror("Unable to allocate memory for token");
            exit(1);
        }
    }

    /* 
    	Setam host-ul si content-type ce vor fi aceleasi pentru fiecare
    	comanda.
    */
    char host[] = "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com";
    char content_type[] = "application/json";

    char command[15];
    char cookie[400] = "error";
    char token[500] = "error";

    int cookies_count = 0;	/* Variabila ce indica faptul ca suntem logati.*/
    int tokens_count = 0;	/* Variabila ce indica faptul ca avem 
    							acces la biblioteca.*/
    int is_logged_user = 0; /* Variabila ce va asigura faptul ca avem un 
    							user logat deja intr-un cont.*/

    while (1) {
    	/* Citesc comanda de la tasatura */
    	fscanf(stdin, "%s", command);

    	if (strcmp(command, "register") == 0) {
    		char username[50];
    		char password[50];

    		if (is_logged_user == 1) {
    			fprintf(stdout, "You can not register, you are already logged in.\n");
    			continue;
    		}

    		/* Se seteaza url-ul corespunzator comenzii.*/
    		char url[] = "/api/v1/tema/auth/register";

    		/* Se citeste username-ul clientului.*/
    		fprintf(stdout, "username=");
    		fscanf(stdin, "%s", username);

    		/* Se citeste parola sa.*/
    		fprintf(stdout, "password=");
    		fscanf(stdin, "%s", password);

    		/* Se redeschide conexiunea cu serverul.*/
    		sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
    		
    		register_client(sockfd, username, password, body_data, 
    						4, host, url, content_type);

    	} else if (strcmp(command, "login") == 0) {
    		char username[50];
    		char password[50];

    		/* 
    			Verificam daca suntem deja intr-un cont, nu putem da login
    			in alt cont.
    		*/
    		if (is_logged_user == 1) {
    			fprintf(stdout, "You are already logged with an username.\n");
    			continue;
    		}

    		/* Se seteaza url-ul corespunzator comenzii.*/
    		char url[] = "/api/v1/tema/auth/login";

    		/* Se citeste username-ul clientului.*/
    		fprintf(stdout, "username=");
    		fscanf(stdin, "%s", username);

    		/* Se citeste parola sa.*/
    		fprintf(stdout, "password=");
    		fscanf(stdin, "%s", password);

    		/* Se redeschide conexiunea cu serverul.*/
    		sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
    		
    		login_client(sockfd, username, password, body_data, 
    						4, host, url, content_type, cookie, &is_logged_user);

    		/* Se pastreaza cookie-ul rezultat din raspunsul serverului.*/
    		strcpy(cookies[0], cookie);
    		cookies_count = 1;

    	} else if (strcmp(command, "enter_library") == 0) {

    		/* Se seteaza url-ul corespunzator comenzii.*/
    		char url[] = "/api/v1/tema/library/access";

    		/* Se redeschide conexiunea.*/
    		sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
    		
    		enter_library(sockfd, cookies, cookies_count, host, url, token);
    		
    		/* Se pastreaza token-ul rezultat din raspunsul server-ului.*/
    		strcpy(tokens[0], token);
    		tokens_count = 1;
   
    	} else if (strcmp(command, "get_books") == 0) {

    		/* Se seteaza url-ul corespunzator comenzii.*/
    		char url[] = "/api/v1/tema/library/books";

    		/* Se redeschide conexiunea cu serverul.*/
    		sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
    		
    		get_books(sockfd, host, url, tokens, tokens_count);
    	
    	} else if (strcmp(command, "get_book") == 0) {

    		/* Se seteaza url-ul corespunzator comenzii.*/
    		char url[] = "/api/v1/tema/library/books/";

    		/* Se citeste id-ul cartii.*/
    		char book_id[50];
    		fprintf(stdout, "id=");
    		fscanf(stdin,"%s", book_id);

    		/* Se completeaza url-ul specific comenzii.*/
    		strncat(url, book_id, strlen(book_id));

    		/* Se redeschide conexiunea cu serverul.*/
    		sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
    		
    		get_book(sockfd, host, url, tokens, tokens_count);

    	} else if (strcmp(command, "add_book") == 0) {
    		
    		/* Se seteaza url-ul corespunzator comenzii.*/
    		char url[] = "/api/v1/tema/library/books";

    		fgetc(stdin);

    		/* Se citesc datele despre cartea pe care vrem sa o adaugam.*/
    		char title[100];
    		fprintf(stdout, "title=");
    		fgets(title, 100, stdin);
    		title[strlen(title) - 1] = '\0';

    		char author[100];
    		fprintf(stdout, "author=");
    		fgets(author, 100, stdin);
    		author[strlen(author) - 1] = '\0';

    		char genre[100];
    		fprintf(stdout, "genre=");
    		fgets(genre, 100, stdin);
    		genre[strlen(genre) - 1] = '\0';

    		char publisher[100];
    		fprintf(stdout, "publisher=");
    		fgets(publisher, 100, stdin);
    		publisher[strlen(publisher) - 1] = '\0';

    		char page_count[10];
    		fprintf(stdout, "page_count=");
    		fscanf(stdin, "%s", page_count);
    		page_count[strlen(page_count) - 1] = '\0';

    		/* Se redeschide conexiunea cu serverul.*/
    		sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
    		
    		add_book(sockfd, title, author, genre, publisher, page_count,
    				body_data, 10, host, url, content_type, tokens, tokens_count);
    		
    	} else if (strcmp(command, "delete_book") == 0) {

    		/* Se seteaza url-ul corespunzator comenzii.*/
    		char url[] = "/api/v1/tema/library/books/";

    		/* Se citeste id-ul cartii ce se doreste a fi stearsa.*/
    		char book_id[50];
    		fprintf(stdout, "id=");
    		fscanf(stdin,"%s", book_id);

    		/* Se completeaza url-ul specific comenzii.*/
    		strncat(url, book_id, strlen(book_id));
    		
    		/* Se redeschide conexiunea cu serverul.*/
    		sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
    		
    		delete_book(sockfd, host, url, tokens, tokens_count);
    	} else if (strcmp(command, "logout") == 0) {

    		/* Se seteaza url-ul corespunzator comenzii.*/
    		char url[] = "/api/v1/tema/auth/logout";

    		/* Se redeschide conexiunea cu serverul.*/
    		sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
    		
    		logout(sockfd, host, url, cookies, cookies_count);
    		
    		/* 
    			Se actualizeaz faptul ca clientul s-a delogat si nu are 
    			nici acces la biblioteca.
    		*/
    		tokens_count = 0;
    		cookies_count = 0;
    		is_logged_user = 0;
    
    	} else if (strcmp(command, "exit") == 0) {
    		break;
    	} else {
    		fprintf(stderr, "Wrong command. Please try again with another one valid.\n");
    		continue;
    	}
    }
  
  	/* Se elibereaza memoria si se inchide socket-ul.*/
  	free_data(body_data, cookies);
    close(sockfd);
    return 0;
}
