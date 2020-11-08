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

char ASIP[SIZE], ASport[SIZE] = "58030", FSIP[SIZE], FSport[SIZE] = "59011";

void processInput(int argc, char* const argv[]) {
    if (argc%2 != 1) {
        printf("Parse error!\n");
        exit(1);
    }
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            memset(ASport, '\0', SIZE * sizeof(char));
            strcpy(ASport, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-n") == 0) {
            memset(ASIP, '\0', SIZE * sizeof(char));
            strcpy(ASIP, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-m") == 0) {
            memset(FSIP, '\0', SIZE * sizeof(char));
            strcpy(FSIP, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-q") == 0) {
            memset(FSport, '\0', SIZE * sizeof(char));
            strcpy(FSport, argv[i + 1]);
            continue;
        }
    }
}

int main(int argc, char* const argv[]) {
    int tcpSocket_AS, tcpSocket_FS, errcode, rID, vc, tid, afd = 0;
    fd_set readfds;
    int maxfd, retval;
    ssize_t n, nbytes, nleft;
    socklen_t addrlen_AS, addrlen_FS;
    struct addrinfo hints_AS, hints_FS, *res_AS, *res_FS;
    struct sockaddr_in addr_AS, addr_FS;
    char *ptr, buffer[SIZE], msg[SIZE], command[SIZE], fop[SIZE], filename[SIZE], uid[SIZE], uidTemp[SIZE];

    if (gethostname(FSIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));

    if (gethostname(ASIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));
        
    /*==========================
        process standard input
    ==========================*/
    processInput(argc, argv);
  
    /*==========================
        Setting up TCP Socket
    ==========================*/
    tcpSocket_AS = socket(AF_INET, SOCK_STREAM, 0);//TCP socket
    if (tcpSocket_AS == -1)/*error*/exit(1);
   
    memset(&hints_AS, 0, sizeof(hints_AS));
    hints_AS.ai_family = AF_INET;//IPv4
    hints_AS.ai_socktype = SOCK_STREAM;//TCP socket

    errcode = getaddrinfo(ASIP, ASport, &hints_AS, &res_AS);  
    if (errcode != 0)/*error*/exit(1);

    n = connect(tcpSocket_AS, res_AS->ai_addr, res_AS->ai_addrlen);
        if (n == -1)/*error*/{ exit(1); } 

    tcpSocket_FS = socket(AF_INET, SOCK_STREAM, 0);//TCP socket
    if (tcpSocket_FS == -1)/*error*/exit(1);
   
    memset(&hints_FS, 0, sizeof(hints_FS));
    hints_FS.ai_family = AF_INET;//IPv4
    hints_FS.ai_socktype = SOCK_STREAM;//TCP socket

    errcode = getaddrinfo(FSIP, FSport, &hints_FS, &res_FS);  
    if (errcode != 0)/*error*/exit(1);

    n = connect(tcpSocket_FS, res_FS->ai_addr, res_FS->ai_addrlen);
        if (n == -1)/*error*/{ exit(1); } 

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(afd, &readfds);
        FD_SET(tcpSocket_AS, &readfds);
        FD_SET(tcpSocket_FS, &readfds);

        /*==========================
        Pick the active file descriptor(s)
        ==========================*/
        maxfd = max(tcpSocket_AS, tcpSocket_FS);
        retval = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (retval <= 0) /*error*/ exit(1);

        for (; retval; retval--) {
            if(FD_ISSET(afd, &readfds)){

                fgets(msg, SIZE, stdin);
                ptr = (char*) malloc(strlen(msg) + 1);
                strcpy(ptr, msg);
                strcat(ptr, "\0");

                if (strcmp(msg, "exit\n") == 0) {
                    /*====================================================
                    Free data structures and close socket connections
                    ====================================================*/
                    freeaddrinfo(res_AS);
                    freeaddrinfo(res_FS);
                    close(tcpSocket_AS);
                    close(tcpSocket_FS);
                    exit(1);
                } 
                else {
                    /*====================================================
                    Send message from stdin to the server
                    ====================================================*/
                    char temp[SIZE];
                    strcpy(temp, msg);
                    char *token = strtok(temp, " ");
                    strcpy(command, token);

                    if(strcmp(command, "login") == 0 || strcmp(command, "req") == 0 || strcmp(command, "val") == 0) {
                        if (strcmp(command, "login") == 0) {
                            token = strtok(NULL, " ");
                            strcpy(uidTemp, token);

                            token = strtok(NULL, " "); /* token has password */
                            // strcpy(password, token);
                            sprintf(ptr, "LOG %s %s", uidTemp, token);

                        }
                        else if (strcmp(command, "req") == 0) {

                            memset(fop, '\0', strlen(fop) * sizeof(char));
                            memset(filename, '\0', strlen(filename) * sizeof(char));
                            // memset(uid, '\0', strlen(uid) * sizeof(char));
                            rID = rand() % 9000 + 1000;
                            sscanf(msg, "req %s %s\n", fop, filename);
                            sprintf(ptr, "REQ %s %d %s %s\n", uid, rID, fop, filename);

                        }
                        else if (strcmp(command, "val") == 0) {
                            token = strtok(NULL, " ");
                            vc = atoi(token);
                            sprintf(ptr, "AUT %s %d %d\n", uid, rID, vc);
                        }
                        nbytes = strlen(ptr);
                        nleft = nbytes;

                        n = write(tcpSocket_AS, ptr, nleft);
                        if (n <= 0) exit(1);
                    }
                    else if(strcmp(command, "upload") == 0 || strcmp(command, "retrieve") == 0 || strcmp(command, "delete") == 0 || strcmp(command, "remove") == 0 || strcmp(command, "list") == 0) {
                        if (strcmp(command, "upload") == 0) {
                        
                        }
                        else if (strcmp(command, "retrieve") == 0) {
                        
                        }
                        else if (strcmp(command, "delete") == 0) {
                        
                        }
                        else if (strcmp(command, "remove") == 0) {
                        
                        }
                        else if (strcmp(command, "list") == 0) {
                        
                        }
                        nbytes = strlen(ptr);
                        nleft = nbytes;

                        n = write(tcpSocket_FS, ptr, nleft);
                        if (n <= 0) exit(1);
                    }
                    else {
                        /* continue if incorrect command */
                        perror("invalid request");
                        continue;
                    }
                }
            }
            else if(FD_ISSET(tcpSocket_AS, &readfds)) {

                char *token;

                n = read(tcpSocket_AS, buffer, SIZE);
                if (n == -1)  exit(1);
                buffer[n] = '\0';

                token = strtok(buffer, " ");
                strcpy(command, token);

                if (strcmp(command, "RLO") == 0) {
                    token = strtok(NULL, " ");
                    if (strcmp(token, "OK\n") == 0) {
                        printf("You are now logged in.\n"); 
                        strcpy(uid, uidTemp);
                        memset(uidTemp, '\0', strlen(uidTemp) * sizeof(char));
                    }
                    else if (strcmp(token, "NOK\n") == 0)
                        printf("Failed to log in. Incorrect user ID or password.\n"); 
                    else
                        perror("RLO incorrect status");
                }
                else if (strcmp(command, "RRQ") == 0) {
                    //verification for ok/nok/errors
                    token = strtok(NULL, " ");
                    if (strcmp(token, "EFOP\n") == 0)
                        printf("Operation not supported.\n");
                    else if (strcmp(token, "ELOG\n") == 0)
                        printf("User not logged in.\n");
                    else if (strcmp(token, "EPD\n") == 0)
                        printf("Unable to connect to personal device.\n");
                    else if (strcmp(token, "EUSER\n") == 0)
                        printf("Incorrect user.\n");
                    else 
                        printf("Request accepted\n"); 
                }
                else if (strcmp(command, "RAU") == 0) {
                    token = strtok(NULL, " ");
                    tid = atoi(token);
                    if(tid == 0) {
                        printf("Authentication failed! (TID=%d)\n", tid); 
                    }
                    else if (tid != 0) {
                        printf("Authenticated! (TID=%d)\n", tid); 
                    }
                }
                memset(buffer, '\0', SIZE * sizeof(char));
                free(ptr);
            }
            else if(FD_ISSET(tcpSocket_FS, &readfds)) {

                char *token;

                n = read(tcpSocket_AS, buffer, SIZE);
                if (n == -1)  exit(1);
                buffer[n] = '\0';

                token = strtok(buffer, " ");
                strcpy(command, token);

                if (strcmp(command, "RLS") == 0) {
                 
                }
                else if (strcmp(command, "RRT") == 0) {
                 
                }
                else if (strcmp(command, "RUP") == 0) {
                 
                }
                else if (strcmp(command, "RDL") == 0) {
                 
                }
                else if (strcmp(command, "RRM") == 0) {
                 
                }
                memset(buffer, '\0', SIZE * sizeof(char));
            }
        }
    }
}