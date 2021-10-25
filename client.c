#include "helpers.h"
#include <arpa/inet.h>
#include <netdb.h>
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
    fprintf(stderr, "Usage: %s server_address server_port\n", file);
    exit(0);
}

int main(int argc, char *argv[]) {
    int sockfd, n, ret;
    struct sockaddr_in serv_addr;
    char buffer[BUFLEN];
    char message[BUFLEN];

    if (argc < 3) {
        usage(argv[0]);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    ret = inet_aton(argv[1], &serv_addr.sin_addr);
    DIE(ret == 0, "inet_aton");

    ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "connect");

    fd_set read_fds; // multimea de citire folosita in select()
    fd_set tmp_fds;  // multime folosita temporar
    int fdmax;       // valoare maxima fd din multimea read_fds
    fdmax = sockfd;

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    FD_SET(sockfd, &read_fds);
    FD_SET(0, &read_fds);
    tmp_fds = read_fds;

    while (1) {
        tmp_fds = read_fds;
        memset(message, 0, BUFLEN);

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        if (FD_ISSET(0, &tmp_fds)) {

            // se citeste de la tastatura
            memset(buffer, 0, BUFLEN);
            fgets(buffer, BUFLEN - 1, stdin);
            if (!strcmp(buffer, "exit\n")) {
                break;
            }

            n = send(sockfd, buffer, strlen(buffer), 0);
            DIE(n < 0, "send");
        }

        if (FD_ISSET(sockfd, &tmp_fds)) {

            int rec_msg = recv(sockfd, message, sizeof(message), 0);
            if (rec_msg < 0) {
                perror("Can't receive message from server!\n");
            }
            if (rec_msg == 0) {
                break;
            }
	    if(!strcmp(message, "LIST")) {
                 int nr = 0;
                 memset(message, 0, BUFLEN);
                 int rec_msg = recv(sockfd, message, sizeof(message), 0);
		 nr = atoi(message);
		 if (rec_msg < 0) {
                    perror("Can't receive message from server!\n");
            	 }
            	
		 printf("Afisez %d fisiere disponibile:\n", nr);
		 
		 for(int k = 0; k<nr; k++) {
			struct file filek;
		 	rec_msg = recv(sockfd, &filek, sizeof(filek), 0);
		 	if (rec_msg < 0) {
                    		perror("Can't receive message from server!\n");
            	 	}
            	 	if (rec_msg == 0) {
                		break;
            	 	}
			printf("%s\n", filek.filename);
		}
		
	    }
	    else if (!strcmp(message, "SHARE")) {
		// do nothing
	    }
	    else { // primeste un filename si trebuie sa trimita tot fisierul inapoi
	    }

            
        }
    }

    close(sockfd);

    return 0;
}
