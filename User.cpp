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
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>

using namespace std;

#define SIZE 128
#define max(A,B)((A) >= (B)?(A):(B))

extern int errno;
int tcpSocket_AS, tcpSocket_FS, errcode, rID, vc, tid, afd = 0;
struct addrinfo hints_AS, hints_FS, *res_AS, *res_FS;
struct sockaddr_in addr_AS, addr_FS;
socklen_t addrlen_AS, addrlen_FS;
char ASIP[SIZE], ASport[SIZE] = "58030", FSIP[SIZE], FSport[SIZE] = "59030";
fd_set readfds;
int maxfd, retval;
ssize_t n, nbytes, nleft;
char buffer[SIZE], msg[SIZE], command[SIZE], fop[SIZE], filename[SIZE], filenameTemp[SIZE], uid[SIZE], uidTemp[SIZE];


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

void treatRLS() {
    char command[SIZE];
    char buffer[SIZE];
    char fname[SIZE];
    int n_files, fsize;
    char *token;

    FILE* tempFile;
    tempFile = fopen("temp.txt", "w");    
    if (!tempFile)
        perror("fopen");
    int size = 0;

    int n = read(tcpSocket_FS, buffer, SIZE);
    while (n != 0) {
        size += n;
        fwrite(buffer, sizeof(char), sizeof(buffer), tempFile);
        memset(buffer, '\0', n * sizeof(char));
        n = read(tcpSocket_FS, buffer, SIZE);
    }
    fwrite("\0", sizeof(char), sizeof(char), tempFile);
    fclose(tempFile);
    tempFile = fopen("temp.txt", "r");    
    if (!tempFile)
        perror("fopen");

    
    char fileBuffer[size + 1];
    
    if (!fgets(fileBuffer, (size)*sizeof(char), tempFile)) perror("oops");
    token = strtok(fileBuffer, " "); //token has n_files or error


    if (strcmp(token, "EOF\n") == 0) {
        printf("No files to list\n");
        return;
    }
    else if (strcmp(token, "IRV\n") == 0) {
        printf("AS validation error\n");
        return;
    }
    else if (strcmp(token, "ERR\n") == 0) {
        printf("Invalid request\n");
        return;
    }

    n_files = atoi(token);

    printf("%d files:\n", n_files);
    
    for (int i = 0; i < n_files; i++) {
        char size[10];
        token = strtok(NULL, " ");
        strcpy(fname, token);
        token = strtok(NULL, " ");
        strcpy(size, token);
        fsize = atoi(size);
        printf("%d - %s %d\n", i + 1, fname, fsize);
    }
    fclose(tempFile);
    remove("temp.txt");
}

void treatRRT() {
    //RRT status [Fsize data]
    char fileBuff[SIZE];
    char buff[SIZE];
    char fsize[10];
    char path[SIZE] = "files/";
    int size;
    strcat(path, filename);
    strcat(path, "\0");


    int n = read(tcpSocket_FS, buff, 3);

    if (strcmp(buff, "OK ") != 0) { //if status != OK
        if (strcmp(buff, "EOF") == 0) {
        printf("File not available\n");
            return;
        }
        else if (strcmp(buff, "NOK") == 0) {
            printf("No content available for this user\n");
            return;
        }
        else if (strcmp(buff, "INV") == 0) {
            printf("AS validation error\n");
            return;
        }
        else if (strcmp(buff, "ERR") == 0) {
            printf("Invalid request\n");
            return;
        }

    }

    do {
        memset(buff, '\0', sizeof(char) * strlen(buff));
        n = read(tcpSocket_FS, buff, 1);
        buff[1] = '\0';
        strcat(fsize, buff);
    }   while (strcmp(buff, " ") != 0);

    size = atoi(fsize);
    printf("size: %d. fsize %s.\n", size, fsize); 

    printf("File name: %s\n", filename);
    printf("Path: %s\n", path);

    FILE *f;
    printf("Path before: %s\n", path);
    f = fopen(path, "wb");
    printf("Path after: %s\n", path);
    do {
        n = read(tcpSocket_FS, fileBuff, SIZE);
        size -= n;
        fwrite(fileBuff, 1, n, f);
        memset(fileBuff, '\0', strlen(fileBuff));
    } while (size > 0);

    printf("final path print: %s\n", path);
    fclose(f);    
}

int setTCPClientFS() {
    tcpSocket_FS = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket_FS == -1) { perror("socket FS"); return 0; }
    hints_FS.ai_family = AF_INET;
    hints_FS.ai_socktype = SOCK_STREAM;
    printf("fazendo get addr\n"); fflush(stdout);
    errcode = getaddrinfo(FSIP, FSport, &hints_FS, &res_FS);  
    printf("fez get addr\n"); fflush(stdout);
    if (errcode != 0) { perror("FS addr info"); return 0; }
    int n = connect(tcpSocket_FS, res_FS->ai_addr, res_FS->ai_addrlen);
    if (n == -1) { perror("socket FS"); return 0; }
    printf("chegou aqui\n");
    FD_SET(tcpSocket_FS, &readfds);
    maxfd = max(maxfd, tcpSocket_FS); 
    return 1;
}

int main(int argc, char* const argv[]) {


    if (gethostname(FSIP, SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));

    if (gethostname(ASIP,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));

    int check = mkdir("files", 0777);
        
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
                strcpy(buffer, msg);
                strcat(buffer, "\0");


                if (strcmp(msg, "exit\n") == 0) {
                    /*====================================================
                    Free data structures and close socket connections
                    ====================================================*/
                    if (res_AS) freeaddrinfo(res_AS);
                    if (res_FS) freeaddrinfo(res_FS);
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

                    strcat(command, "\0");

                    if (strcmp(command, "login") == 0 || strcmp(command, "req") == 0 || strcmp(command, "val") == 0) {
                        if (strcmp(command, "login") == 0) {
                            token = strtok(NULL, " ");
                            strcpy(uidTemp, token);

                            token = strtok(NULL, " "); /* token has password */
                            sprintf(buffer, "LOG %s %s", uidTemp, token);
                        }
                        else if (strcmp(command, "req") == 0) {

                            memset(fop, '\0', strlen(fop) * sizeof(char));
                            memset(filenameTemp, '\0', strlen(filenameTemp) * sizeof(char));
                            rID = rand() % 9000 + 1000;
                            sscanf(msg, "req %s %s\n", fop, filenameTemp);
                            if (strlen(filenameTemp) != 0)
                                sprintf(buffer, "REQ %s %d %s %s\n", uid, rID, fop, filenameTemp);
                            else 
                                sprintf(buffer, "REQ %s %d %s\n", uid, rID, fop);

                        }
                        else if (strcmp(command, "val") == 0) {
                            token = strtok(NULL, " ");
                            vc = atoi(token);
                            sprintf(buffer, "AUT %s %d %d\n", uid, rID, vc);
                        }
                        else {
                            /* continue if incorrect command */
                            perror("invalid request");
                            continue;
                        }   
                        nbytes = strlen(buffer);
                        nleft = nbytes;

                        // REQ UID RID Fop [Fname]
                        n = write(tcpSocket_AS, buffer, nleft);
                        if (n <= 0) { perror("tcp write"); exit(1); }

                    }
                    else if(strcmp(command, "upload") == 0 || strcmp(command, "u") == 0 ||
                            strcmp(command, "retrieve") == 0 || strcmp(command, "r") == 0 ||
                            strcmp(command, "delete") == 0 || strcmp(command, "d") == 0 ||
                            strcmp(command, "remove") == 0 || strcmp(command, "x") == 0 || 
                            strcmp(command, "list\n") == 0 || strcmp(command, "l\n") == 0) {
                        if (!setTCPClientFS()) {
                            perror("setup FS socket");
                        }
                        if (strcmp(command, "upload") == 0 || strcmp(command, "u") == 0) {

                        }
                        else if (strcmp(command, "retrieve") == 0 || strcmp(command, "r") == 0) {
                            sprintf(buffer, "RTV %s %d %s\n", uid, tid, filenameTemp);
                        }
                        else if (strcmp(command, "delete") == 0 || strcmp(command, "d") == 0) {
                        
                        }
                        else if (strcmp(command, "remove\n") == 0 || strcmp(command, "x\n") == 0) {
                            
                        }
                        else if (strcmp(command, "list\n") == 0 || strcmp(command, "l\n") == 0) {
                            /* LST UID TID */
                            sprintf(buffer, "LST %s %d\n", uid, tid);
                        }
                        nbytes = strlen(buffer);
                        nleft = nbytes;


                        n = write(tcpSocket_FS, buffer, nleft);
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
                    else if (strcmp(token, "OK\n") == 0) {
                        printf("Request accepted\n"); 
                        if (strlen(filenameTemp)) {
                            memset(filename, '\0', strlen(filename) * sizeof(char));
                            strcpy(filename, filenameTemp);
                        }
                    }
                        
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
            }
            else if (FD_ISSET(tcpSocket_FS, &readfds)) {

                char *token;
                char bufferTemp[SIZE];
                memset(command, '\0', strlen(command)*sizeof(char));
                int n = read(tcpSocket_FS, command, 4);
                command[4] = '\0';
                printf("command: %s.\n", command);

                if (strcmp(command, "RLS ") == 0) {
                    treatRLS();
                }
                else if (strcmp(command, "RRT ") == 0) {
                    treatRRT();
                 
                }
                else if (strcmp(command, "RUP ") == 0) {
                 
                }
                else if (strcmp(command, "RDL ") == 0) {
                 
                }
                else if (strcmp(command, "RRM ") == 0) {
                 
                }
                close(tcpSocket_FS);
                tcpSocket_FS = -1;
            }
            memset(buffer, '\0', SIZE * sizeof(char));
        }
    }
}