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

#define SIZE 128
#define max(A,B)((A)>=(B)?(A):(B))

extern int errno;
int tcpSocket_AS, tcpSocket_FS, errcode, rID, vc, tid, afd = 0;
struct addrinfo hints_AS, hints_FS, *res_AS, *res_FS;
struct sockaddr_in addr_AS, addr_FS;
socklen_t addrlen_AS, addrlen_FS;
char ASIP[SIZE], ASport[SIZE] = "58030", FSIP[SIZE], FSport[SIZE] = "59030";
fd_set readfds;
int maxfd, retval;

void processInput(int argc, char* const argv[]) {
    if (argc % 2 != 1) {
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

int setTCPClientFS() {
    tcpSocket_FS = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket_FS == -1) { perror("socket FS"); return 0; }
    memset(&hints_FS, 0, sizeof(hints_FS));
    hints_FS.ai_family = AF_INET;
    hints_FS.ai_socktype = SOCK_STREAM;
    errcode = getaddrinfo(FSIP, FSport, &hints_FS, &res_FS);  
    if (errcode != 0) { perror("FS addr info"); return 0; }
    int n = connect(tcpSocket_FS, res_FS->ai_addr, res_FS->ai_addrlen);
    if (n == -1) { perror("socket FS"); return 0; }
    FD_SET(tcpSocket_FS, &readfds);
    maxfd = max(maxfd, tcpSocket_FS); 
    return 1;
}

int main(int argc, char* const argv[]) {

    ssize_t n, nbytes, nleft;
    char *ptr, buffer[SIZE], msg[SIZE], command[SIZE], fop[SIZE], filename[SIZE], uid[SIZE], uidTemp[SIZE];

    if (gethostname(FSIP, SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));

    if (gethostname(ASIP,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));
        
    /*==========================
        process standard input
    ==========================*/
    processInput(argc, argv);
  
    /*==========================
        Setting up TCP Socket
    ==========================*/
    tcpSocket_AS = socket(AF_INET, SOCK_STREAM, 0);
        if (tcpSocket_AS == -1) { perror("socket AS"); exit(1); }
    memset(&hints_AS, 0, sizeof(hints_AS));
    hints_AS.ai_family = AF_INET;
    hints_AS.ai_socktype = SOCK_STREAM;
    errcode = getaddrinfo(ASIP, ASport, &hints_AS, &res_AS);  
        if (errcode != 0) { perror("AS addr info"); exit(1); }

    n = connect(tcpSocket_AS, res_AS->ai_addr, res_AS->ai_addrlen);
        if (n == -1) { perror("connect AS"); exit(1); }

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
        if (retval <= 0) { perror("select"); exit(1); }

        for (; retval; retval--) {
            memset(buffer, '\0', strlen(buffer) * sizeof(char));
            if (FD_ISSET(afd, &readfds)) {

                memset(msg, '\0', strlen(msg) * sizeof(char));
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

                    if (strcmp(command, "login") == 0 || strcmp(command, "req") == 0 || strcmp(command, "val") == 0) {
                        if (strcmp(command, "login") == 0) {
                            token = strtok(NULL, " ");
                            strcpy(uidTemp, token);

                            token = strtok(NULL, " "); /* token has password */
                            sprintf(ptr, "LOG %s %s", uidTemp, token);
                        }
                        else if (strcmp(command, "req") == 0) {

                            memset(fop, '\0', strlen(fop) * sizeof(char));
                            memset(filename, '\0', strlen(filename) * sizeof(char));
                            rID = rand() % 9000 + 1000;
                            sscanf(msg, "req %s %s\n", fop, filename);
                            if (strlen(filename) != 0)
                                sprintf(ptr, "REQ %s %d %s %s\n", uid, rID, fop, filename);
                            else 
                                sprintf(ptr, "REQ %s %d %s\n", uid, rID, fop);

                        }
                        else if (strcmp(command, "val") == 0) {
                            token = strtok(NULL, " ");
                            vc = atoi(token);
                            sprintf(ptr, "AUT %s %d %d\n", uid, rID, vc);
                        }
                        else {
                            /* continue if incorrect command */
                            perror("invalid request");
                            continue;
                        }   
                        nbytes = strlen(ptr);
                        nleft = nbytes;

                        // REQ UID RID Fop [Fname]
                        n = write(tcpSocket_AS, ptr, nleft);
                        if (n <= 0) { perror("tcp write"); exit(1); }

                    }
                    else if(strcmp(command, "upload") == 0 || strcmp(command, "u") == 0 ||
                            strcmp(command, "retrieve") == 0 || strcmp(command, "r") == 0 ||
                            strcmp(command, "delete") == 0 || strcmp(command, "d") == 0 ||
                            strcmp(command, "remove\n") == 0 || strcmp(command, "x\n") == 0 || 
                            strcmp(command, "list\n") == 0 || strcmp(command, "l") == 0) {
                        if (!setTCPClientFS()) {
                            perror("setup FS socket");
                        }
                        if (strcmp(command, "upload") == 0 || strcmp(command, "u") == 0) {
                            /* UPL UID TID Fname Fsize data */
                            // processUpload(msg);
                            //sprintf(ptr, "UPL %s %d %s %s\n", uid, tid, fname, Fsize);

                        }
                        else if (strcmp(command, "retrieve") == 0 || strcmp(command, "r") == 0) {
                        
                        }
                        else if (strcmp(command, "delete") == 0 || strcmp(command, "d") == 0) {
                        
                        }
                        else if (strcmp(command, "remove\n") == 0 || strcmp(command, "x\n") == 0) {
                        
                        }
                        else if (strcmp(command, "list\n") == 0 || strcmp(command, "l\n") == 0) {
                            /* LST UID TID */
                            sprintf(ptr, "LST %s %d\n", uid, tid);
                            printf("ptr: %s\n", ptr);

                        
                        }
                        nbytes = strlen(ptr);
                        nleft = nbytes;

                        n = write(tcpSocket_FS, ptr, nleft);
                        if (n <= 0) { perror("tcp write"); exit(1); }
                    }
                    else {
                        /* continue if incorrect command */
                        perror("invalid request");
                        continue;
                    }
                }
            }
            else if (FD_ISSET(tcpSocket_AS, &readfds)) {
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
                    else if (strcmp(token, "ERR\n") == 0)
                        printf("Incorrect request.\n");
                    else if (strcmp(token, "OK\n") == 0)
                        printf("Request accepted\n"); 
                    else perror("RRQ receive invalid token");
                }
                else if (strcmp(command, "RAU") == 0) {
                    int tidTemp;
                    token = strtok(NULL, " ");
                    tidTemp = atoi(token);
                    if (tidTemp == 0) {
                        printf("Authentication failed!\n"); 
                    }
                    else if (tidTemp != 0) {
                        tid = tidTemp;
                        printf("Authenticated! (TID=%d)\n", tidTemp); 
                    }
                }
                // memset(buffer, '\0', SIZE * sizeof(char));
                free(ptr);
            }
            else if (FD_ISSET(tcpSocket_FS, &readfds)) {


                char *token;

                n = read(tcpSocket_FS, buffer, SIZE);
                if (n == -1)  exit(1);
                buffer[n] = '\0';


                token = strtok(buffer, " ");
                //strcpy(command, token);

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
                close(tcpSocket_FS);
                tcpSocket_FS = -1;
            }
            memset(buffer, '\0', SIZE * sizeof(char));
        }
    }
}