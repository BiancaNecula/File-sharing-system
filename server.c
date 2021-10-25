#include "helpers.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct file {
    int sockfd;
    char filename[50];
};

void usage(char *file) {
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}


int main(int argc, char *argv[]) {

    int sockfd, newsockfd, portno;
    char buffer[1000];
    struct sockaddr_in serv_addr, cli_addr;
    int n, i, ret, send_msg;
    socklen_t clilen;

    struct file files[100];

    fd_set read_fds; // multimea de citire folosita in select()
    fd_set tmp_fds;  // multime folosita temporar
    int fdmax;       // valoare maxima fd din multimea read_fds

    if (argc < 2) {
        usage(argv[0]);
    }

   
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    portno = atoi(argv[1]);
    DIE(portno == 0, "atoi");

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    int enable = 1;

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        perror("setsocketopt");
        exit(1);
    }
    ret = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bind");

    ret = listen(sockfd, MAX_CLIENTS);
    DIE(ret < 0, "listen");

   
    FD_SET(sockfd, &read_fds);
    fdmax = sockfd;
    int j = 0;
    while (1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == sockfd) {
                    
                    clilen = sizeof(cli_addr);
                    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                    DIE(newsockfd < 0, "accept");
                    
                    FD_SET(newsockfd, &read_fds);
                    if (newsockfd > fdmax) {
                        fdmax = newsockfd;
                    }

                    printf("Noua conexiune de la %s, port %d, socket client %d\n",
                           inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
                } else {
               
                    memset(buffer, 0, BUFLEN);
                    n = recv(i, buffer, sizeof(buffer), 0);
                    DIE(n < 0, "recv");

                    if (!strncmp(buffer, "LIST", 4)) {
			printf("S-a primit de la clientul de pe socketul %d mesajul LIST.\n", i);
			send_msg = send(i, "LIST", sizeof("LIST"), 0);
			if (send_msg < 0) {
                            perror("Can't send message");
                        }
			memset(buffer, 0, BUFLEN);
                        memcpy(buffer, &j, sizeof(int));
			send_msg = send(i, buffer, sizeof(int), 0);
			if (send_msg < 0) {
                            perror("Can't send message");
                        }
			for(int k = 0; k<j; k++) {
				send_msg = send(i, &files[k], sizeof(struct file), 0);
                        	if (send_msg < 0) {
                            		perror("Can't send message");
                        	}
			}		
                        
                    } else {
			char *tok;
			tok = strtok(buffer, " ");
			if(!strcmp(tok, "SHARE")) {
				tok = strtok(NULL, "\n");
				strcpy(files[j].filename, tok);
				files[j].sockfd = i;
				printf("S-a primit de la clientul de pe socketul %d fisierul: %s\n", i, files[j].filename);
				j++;
				send_msg = send(i, "SHARE", sizeof("SHARE"), 0);
				if (send_msg < 0) {
                                  perror("Can't send message");
                                }
			}
			if(!strcmp(tok, "UNSHARE")) {
				tok = strtok(NULL, " \n");
				for(int k = 0; k<j; k++) {
					if(!strcmp(files[k].filename, tok) && files[k].sockfd == i) {
						// remove from array					
					}				
				}
			}
			if(!strcmp(tok, "DOWNLOAD")) {
				tok = strtok(NULL, " \n");
				for(int k = 0; k<j; k++) {
					if(!strcmp(files[k].filename, tok)) {
						send_msg = send(files[k].sockfd, buffer, strlen(buffer), 0);	
						if (send_msg < 0) {
                            				perror("Can't send message");
                        			}				
					}				
				}
			}
                        
                    }

                    if (n == 0) {
                        close(i);
                        FD_CLR(i, &read_fds);
                        printf("Socket-ul client %d a inchis conexiunea\n", i);

                    } else {
                        // printf("S-a primit de la clientul de pe socketul %d mesajul: %s\n", i, buffer);
                    }
                }
            }
        }
    }

    close(sockfd);

    return 0;
}
