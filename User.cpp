#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <iostream>

using namespace std;

// #define PORT "58011"
#define IP "tejo.tecnico.ulisboa.pt"
#define SIZE 128

char ASIP[SIZE], ASport[SIZE] = "58011", FSIP[SIZE], FSport[SIZE] = "59030";

void processInput(int argc, char* const argv[]) {

    //faltam verificacoes de qtds de argumentos etc acho eu
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
    struct addrinfo hints, *res;
    int tcpServerSocket, errcode, rID, vc;
    ssize_t n;
    ssize_t nbytes, nleft, nwritten, nread;
    char *ptr, buffer[SIZE], msg[SIZE], command[4], fop[2], filename[50], uid[6]="";

    gethostname(FSIP, SIZE);
    gethostname(ASIP, SIZE);
  
    tcpServerSocket = socket(AF_INET, SOCK_STREAM, 0);//TCP socket
    if (tcpServerSocket == -1)/*error*/exit(1);
   
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;//IPv4
    hints.ai_socktype = SOCK_STREAM;//TCP socket

    // errcode = getaddrinfo(IP, PORT, &hints, &res);  
    errcode = getaddrinfo(IP, ASport, &hints, &res);  
    if (errcode != 0)/*error*/exit(1);

    n = connect(tcpServerSocket, res->ai_addr, res->ai_addrlen);
        if (n == -1)/*error*/{exit(1);} 

    processInput(argc, argv);

    while (1) {
        fgets(msg, SIZE, stdin);
        ptr = (char*) malloc(strlen(msg) + 1);
        strcpy(ptr, msg);
        strcat(ptr, "\0");

        if (strcmp(msg, "exit\n") == 0) {
            close(tcpServerSocket);
            exit(1);
        } 

        else {
            char *token = strtok(msg, " ");
            strcpy(command, token);

            if (strcmp(command, "LOG") == 0) {
                token = strtok(NULL, " ");
                strcpy(uid, token);
                printf("You are now logged in.\n"); // isto tem que que aparecer se recebermos o RLO OK
            }
            else if (strcmp(command, "REQ") == 0) {
                if (strcmp(uid,"") == 0) {
                    printf("NO UID\n");
                }
                else {
                    rID = rand() % 9000 + 1000;

                    int flag=0;
                    while( token != NULL ) {
                        token = strtok(NULL, " ");
                        if(flag==0)
                        {
                            strcpy(fop, token);
                            flag=1;
                        }
                        else if(flag=1){
                            strcpy(filename, token);
                            break;
                        }
                    } //////////// mudar esta parte das flags
                    sprintf(ptr, "%s %s %d %s %s", command, uid, rID, fop, filename);
                
                }
            }
            else if (strcmp(command, "VAL") == 0) {
                token = strtok(NULL, " ");
                vc = atoi(token);
                sprintf(ptr, "AUT %s %d %d\n", uid, rID, vc);
                //printf("Authenticated! (TID=%d)\n"); // isto tem que que aparecer se recebermos o RAU ####
            }

            nbytes = strlen(ptr);
            nleft = nbytes;
            n = write(tcpServerSocket, ptr, nleft);
            if (n <= 0) exit(1);

            n = read(tcpServerSocket, buffer, SIZE);
            if (n == -1)  exit(1);
        
            write(1, buffer, n);   
        }
    }
}