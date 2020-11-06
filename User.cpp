#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>

using namespace std;

//#define IP "tejo.tecnico.ulisboa.pt"
#define SIZE 128

extern int errno;

char ASIP[SIZE], ASport[SIZE] = "58030", FSIP[SIZE], FSport[SIZE] = "59030";

void processInput(int argc, char* const argv[]) {
    if (argc%2 != 1) {
        printf("Parse error!\n");
        exit(1);
    }
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            strcpy(ASport, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-n") == 0) {
            strcpy(ASIP, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-m") == 0) {
            strcpy(FSIP, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-q") == 0) {
            strcpy(FSport, argv[i + 1]);
            continue;
        }
    }
}

int main(int argc, char* const argv[]) {
    int tcpSocket, errcode, rID, vc, tid;
    ssize_t n, nbytes, nleft;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char *ptr, buffer[SIZE], msg[SIZE], command[4], fop[2], filename[50], uid[6]="";

    if (gethostname(FSIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));

    if (gethostname(ASIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));
  
    /*==========================
        Setting up TCP Socket
    ==========================*/
    tcpSocket = socket(AF_INET, SOCK_STREAM, 0);//TCP socket
    if (tcpSocket == -1)/*error*/exit(1);
   
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;//IPv4
    hints.ai_socktype = SOCK_STREAM;//TCP socket

    errcode = getaddrinfo(ASIP, ASport, &hints, &res);  
    if (errcode != 0)/*error*/exit(1);

    n = connect(tcpSocket, res->ai_addr, res->ai_addrlen);
        if (n == -1)/*error*/{exit(1);} 

    /*==========================
        process standard input
    ==========================*/
    processInput(argc, argv);

    while (1) {
        fgets(msg, SIZE, stdin);
        ptr = (char*) malloc(strlen(msg) + 1);
        strcpy(ptr, msg);
        strcat(ptr, "\0");

        if (strcmp(msg, "exit\n") == 0) {
            /*====================================================
            Free data structures and close socket connections
            ====================================================*/
            freeaddrinfo(res);
            close(tcpSocket);
            exit(1);
        } 

        else {
            /*====================================================
            Send message from stdin to the server
            ====================================================*/
            char *token = strtok(msg, " ");
            strcpy(command, token);

            if (strcmp(command, "LOG") == 0) {
                token = strtok(NULL, " ");
                strcpy(uid, token);
            }
            else if (strcmp(command, "REQ") == 0) {
                if (strcmp(uid,"") == 0) {
                    printf("NO UID\n");
                }
                else {
                    rID = rand() % 9000 + 1000;

                    int flag = 0;
                    while ( token != " " ) {
                        token = strtok(NULL, " ");
                        if (flag == 0) {
                            strcpy(fop, token);
                            flag = 1;
                        }
                        else if (flag == 1) {
                            if (token != NULL) {
                                strcpy(filename, token);
                                break;
                            }
                            else {
                                flag = 2;
                                break;
                            }
                        }
                    } //////////// mudar esta parte das flags
                    if (flag == 1) {
                         sprintf(ptr, "%s %s %d %s %s", command, uid, rID, fop, filename);
                    }
                    else if (flag == 2) {
                        //strcpy(command, "REQ");
                        sprintf(ptr, "REQ %s %d %s", uid, rID, fop);
                    }
                
                }
            }
            else if (strcmp(command, "VAL") == 0) {
                token = strtok(NULL, " ");
                vc = atoi(token);
                sprintf(ptr, "AUT %s %d %d\n", uid, rID, vc);
            }
            nbytes = strlen(ptr);
            nleft = nbytes;

            n = write(tcpSocket, ptr, nleft);
            if (n <= 0) exit(1);

            n = read(tcpSocket, buffer, SIZE);
            if (n == -1)  exit(1);
            buffer[n] = '\0';

            char *token_2 = strtok(buffer, " ");
            strcpy(command, token_2);

            if (strcmp(command, "RLO")==0) {
                printf("You are now logged in.\n"); 
            }
            else if (strcmp(command, "RAU") == 0) {
                token_2 = strtok(NULL, " ");
                tid = atoi(token_2);

                printf("Authenticated! (TID=%d)\n", tid); 
                }
            }

            memset(buffer, '\0', SIZE * sizeof(char));
            free(ptr);
    }
}