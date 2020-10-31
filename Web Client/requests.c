#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *query_params,
                            char **cookies, int cookies_count,
                            char **tokens, int tokens_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    memset(line, 0, LINELEN);
    char *buffer = (char*)calloc(LINELEN, sizeof(char));

    if (cookies != NULL) {
       sprintf(line, "Cookie: ");

       int i;
       for (i = 0; i < cookies_count; i++) {
            sprintf(buffer, "%s", cookies[i]);
            if (i) {
                strcat(line, ";");
            }
            strcat(line, buffer);
       }

       compute_message(message, line);
       memset(line, 0, LINELEN);
    }

    //Step 4: add tokens
    memset(line, 0, LINELEN);
    char *buff_tokens = (char*)calloc(LINELEN, sizeof(char));

    if (tokens != NULL) {
       sprintf(line, "Authorization: Bearer ");

       int i;
       for (i = 0; i < tokens_count; i++) {
            sprintf(buff_tokens, "%s", tokens[i]);
            if (i) {
                strcat(line, ";");
            }
            strcat(line, buff_tokens);
       }

       compute_message(message, line);
       memset(line, 0, LINELEN);
    }

    // Step 5: add final new line
    compute_message(message, "");
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char **cookies, int cookies_count,
                            char **tokens, int tokens_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Step 2: add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);

    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */
    memset(line, 0, LINELEN);
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    char *buff = (char*)calloc(LINELEN, sizeof(char*));
    int i;

    for (i = 0; i < body_data_fields_count; i++) {
        if (i != 0 && i != (body_data_fields_count - 1)) {
            if (i % 2 == 0) {
                sprintf(buff, ",\"%s\"", body_data[i]);
            } else if (i % 2 == 1) {
                sprintf(buff, ":\"%s\"", body_data[i]);
            }

        } else if (i == (body_data_fields_count - 1)) {
            sprintf(buff, ":\"%s\"}", body_data[i]);
        } else if (i == 0) {
            sprintf(buff, "{\"%s\"", body_data[i]);
        } 

        strcat(body_data_buffer, buff);
    }

    sprintf(line, "Content-Length: %ld", strlen(body_data_buffer));
    compute_message(message, line);

    // Step 4 (optional): add cookies
    memset(line, 0, LINELEN);

    if (cookies != NULL) {
       sprintf(line, "Cookie:");

       for(i = 0; i < cookies_count; i++) {
            sprintf(buff, " %s", cookies[i]);
            if (i) {
                strcat(line, ";");
            } 
            strcat(line, buff);
       }

       compute_message(message, line);
       memset(line, 0, LINELEN);
    }

    //Step 5: add tokens
    memset(line, 0, LINELEN);
    char *buff_tokens = (char*)calloc(LINELEN, sizeof(char));

    if (tokens != NULL) {
       sprintf(line, "Authorization: Bearer ");

       int i;
       for (i = 0; i < tokens_count; i++) {
            sprintf(buff_tokens, "%s", tokens[i]);
            if (i) {
                strcat(line, ";");
            }
            strcat(line, buff_tokens);
       }

       compute_message(message, line);
       memset(line, 0, LINELEN);
    }

    // Step 6: add new line at end of header
    compute_message(message, "");

    // Step 7: add the actual payload data
    memset(line, 0, LINELEN);
    compute_message(message, body_data_buffer);

    free(line);
    return message;
}

char *compute_delete_request(char *host, char *url, char *query_params,
                            char **cookies, int cookies_count,
                            char **tokens, int tokens_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    memset(line, 0, LINELEN);
    char *buffer = (char*)calloc(LINELEN, sizeof(char));

    if (cookies != NULL) {
       sprintf(line, "Cookie: ");

       int i;
       for (i = 0; i < cookies_count; i++) {
            sprintf(buffer, "%s", cookies[i]);
            if (i) {
                strcat(line, ";");
            }
            strcat(line, buffer);
       }

       compute_message(message, line);
       memset(line, 0, LINELEN);
    }

    //Step 4: add tokens
    memset(line, 0, LINELEN);
    char *buff_tokens = (char*)calloc(LINELEN, sizeof(char));

    if (tokens != NULL) {
       sprintf(line, "Authorization: Bearer ");

       int i;
       for (i = 0; i < tokens_count; i++) {
            sprintf(buff_tokens, "%s", tokens[i]);
            if (i) {
                strcat(line, ";");
            }
            strcat(line, buff_tokens);
       }

       compute_message(message, line);
       memset(line, 0, LINELEN);
    }

    // Step 5: add final new line
    compute_message(message, "");
    return message;
}
