#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>

using namespace std;

#define max(A,B)((A)>=(B)?(A):(B))
#define SIZE 128

extern int errno;

char PDIP[SIZE], PDport[SIZE] = "57030", ASport[SIZE] = "58030", ASIP[SIZE];
char fixedReg[SIZE];

void processInput(int argc, char* const argv[]) {
    if (argc < 2)
        exit(1);
    strcpy(PDIP, argv[1]);
    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            if (strlen(argv[i + 1]) > SIZE) perror("incorrect PDport");
            memset(PDport, '\0', SIZE * sizeof(char));
            strcpy(PDport, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-n") == 0) {
            if (strlen(argv[i + 1]) > SIZE) perror("incorrect ASIP");
            memset(ASIP, '\0', SIZE * sizeof(char));
            strcpy(ASIP, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-p") == 0) {
            if (strlen(argv[i + 1]) > SIZE) perror("incorrect ASport");
            memset(ASport, '\0', SIZE * sizeof(char));
            strcpy(ASport, argv[i + 1]);
            continue;
        }
    }
    strcpy(fixedReg, PDIP);
    strcat(fixedReg, " ");
    strcat(fixedReg, PDport);
    strcat(fixedReg, "\n");
}

int main(int argc, char* argv[]){
    int udpServerSocket, udpClientSocket, afd = 0, errcode_c,errcode_s,fd;
    fd_set readfds;
    int maxfd, retval;
    ssize_t n;
    socklen_t addrlen_c, addrlen_s;
    struct addrinfo hints_c, hints_s, *res_c, *res_s;
    struct sockaddr_in addr_c, addr_s;
    char buffer[SIZE], msg[SIZE];
    char command[SIZE], uid[SIZE], password[SIZE], filename[SIZE], vc[SIZE], op_name[SIZE], fop[SIZE];
    // bool registered = false;

    if (gethostname(ASIP, SIZE) == -1)
        fprintf(stderr, "error: %s\n", strerror(errno));
    
    /*==========================
        process standard input
    ==========================*/
    processInput(argc, argv);

    /*==========================
    Setting up UDP Client Socket
    ==========================*/
    udpClientSocket = socket(AF_INET, SOCK_DGRAM, 0);//UDP socket
    if(udpClientSocket == -1)/*error*/exit(1);
    memset(&hints_c, 0, sizeof hints_c);
    hints_c.ai_family = AF_INET;//IPv4
    hints_c.ai_socktype = SOCK_DGRAM;//UDP socket
    errcode_c = getaddrinfo(ASIP, ASport, &hints_c ,&res_c);
    if (errcode_c != 0)/*error*/exit(1);
    
    /*==========================
    Setting up UDP Server Socket
    ==========================*/
    udpServerSocket = socket(AF_INET, SOCK_DGRAM, 0);//UDP socket
    if(udpServerSocket == -1)/*error*/exit(1);
    memset(&hints_s, 0, sizeof hints_s);
    hints_s.ai_family = AF_INET;//IPv4
    hints_s.ai_socktype = SOCK_DGRAM;//UDP socket
    hints_s.ai_flags = AI_PASSIVE;
    errcode_s = getaddrinfo(NULL, PDport, &hints_s, &res_s);
    if (errcode_s != 0)/*error*/exit(1);

    if (bind(udpServerSocket, res_s->ai_addr, res_s->ai_addrlen) < 0) exit(1);
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(afd, &readfds);
        FD_SET(udpClientSocket, &readfds);
        FD_SET(udpServerSocket, &readfds);

        /*==========================
        Pick the active file descriptor(s)
        ==========================*/
        maxfd = max(udpClientSocket, udpServerSocket);
        retval = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (retval <= 0) /*error*/ exit(1);
        
        for (; retval; retval--) {
            if(FD_ISSET(afd, &readfds)){
                memset(msg, '\0', strlen(msg) * sizeof(char));
                fgets(msg, SIZE, stdin);
                /* msg is input written in standard input*/
                strtok(msg, "\n");
                if (strcmp(msg, "exit") == 0) {
                    /*====================================================
                    Free data structures and close socket connections
                    ====================================================*/
                    // registered = false;
                    memset(msg, '\0', SIZE * sizeof(char));
                    strcpy(msg, "UNR ");
                    strcat(msg, uid);
                    strcat(msg, " ");
                    strcat(msg, password);
                    strcat(msg, "\n");

                    int n = sendto(udpClientSocket, msg, strlen(msg), 0, res_c->ai_addr, res_c->ai_addrlen);
                    if (n == -1 ) perror("error on sendto");
                }
                else {
                    /*====================================================
                    Send message from stdin to the server
                    ====================================================*/
                    /* msg = "REG 99999 password" */
                    // char temp[SIZE];
                    char temp[SIZE];
                    strcpy(temp, msg);
                    char *token = strtok(temp, " ");

                    if (strcmp(token, "reg") != 0) {
                        perror("invalid request");
                        break;
                    }
                    // if (registered) {
                    //     memset(temp, '\0', strlen(temp) * sizeof(char));
                    //     strcpy(temp, "already registered as user ");
                    //     strcat(temp, uid);
                    //     perror(temp);
                    //     break;
                    // }

                    memset(uid, '\0', strlen(uid) * sizeof(char));
                    memset(password, '\0', strlen(password) * sizeof(char));
                    sscanf(msg, "reg %s %s\n", uid, password);

                    strcat(strcat(strcat(strcpy(msg, "REG "), uid), " "), password);
                    strcat(strcat(msg, " "), fixedReg);      

                    n = sendto(udpClientSocket, msg, strlen(msg), 0, res_c->ai_addr, res_c->ai_addrlen);
                    if (n == -1)/*error*/exit(1);     
                }
            }
            if (FD_ISSET(udpClientSocket, &readfds)) {
                /*====================================================
                Receive message from AS in response to PD request
                ====================================================*/
                addrlen_c = sizeof(addr_c);
                n = recvfrom(udpClientSocket, buffer, 128, 0, (struct sockaddr*) &addr_c, &addrlen_c);
                if (n == -1)/*error*/exit(1);
                buffer[n] = '\0';

                if (strcmp(buffer, "RRG OK\n") == 0) {
                    printf("Registration successful\n");  
                }
                else if (strcmp(buffer, "RRG NOK\n") == 0) {
                    printf("Registration unsuccessful\n");  
                }
                else if (strcmp(buffer, "RUN OK\n") == 0) {
                    printf("Unregistration successful\n");  
                    
                    freeaddrinfo(res_c);
                    freeaddrinfo(res_s);
                    close(udpClientSocket);
                    close(udpServerSocket);
                    exit(1);
                }
                else if (strcmp(buffer, "RUN NOK\n") == 0) {
                    printf("Unregistration unsuccessful\n");  
                }
                else {
                    freeaddrinfo(res_c);
                    freeaddrinfo(res_s);
                    close(udpClientSocket);
                    close(udpServerSocket);
                    exit(1);
                }
                /* reset buffer */
                memset(buffer, '\0', SIZE * sizeof(char));
            }
            if (FD_ISSET(udpServerSocket, &readfds)) {
                /*====================================================
                Receive messages from AS unprovoked
                ====================================================*/
                addrlen_s = sizeof(addr_s);
                n = recvfrom(udpServerSocket, buffer, SIZE, 0, (struct sockaddr*) &addr_s, &addrlen_s);
                if (n == -1) /*error*/ perror("recv udpServerSocket");
                buffer[n] = '\0';

                /*separate command*/
                char *token = strtok(buffer, " ");
                strcpy(command, token);

                if (strcmp(command, "VLC") == 0) {
                    token = strtok(NULL, " ");
                    strcpy(uid, token);
                    token = strtok(NULL, " ");
                    strcpy(vc, token);
                    token = strtok(NULL, " ");
                    strcpy(fop, token);
                    if (strcmp(fop, "L\n") != 0 || strcmp(fop, "X\n") != 0) {
                        token = strtok(NULL, " ");
                        strcpy(filename, token);
                    } else {
                        filename[0] = '\0';
                    }
                
                    if (strcmp(fop, "U") == 0) {
                        strcpy(op_name, "upload:");
                    } 
                    else if (strcmp(fop, "R") == 0) {
                        strcpy(op_name, "retrieve:");
                    } 
                    else if (strcmp(fop, "D") == 0) {
                        strcpy(op_name, "retrieve:");
                    } 
                    else if (strcmp(fop, "L") == 0) {
                        strcpy(op_name, "list\n");
                    }  
                    else if (strcmp(fop, "X") == 0) {
                        strcpy(op_name, "remove\n");
                    }
                    else{
                        printf("ERROR\n");
                    }

                    if (strcmp(fop, "L") == 0 || strcmp(fop, "X") == 0) {
                        printf("VC=%s, %s", vc, op_name);
                    }
                    else {
                        printf("VC=%s, %s %s", vc, op_name, filename);
                    }

                    /*copy confirmation message to buffer
                      to send to AS*/
                    strcpy(buffer, "RVC OK\n");
                }

                /*send confirmation message to AS*/
                n = sendto(udpServerSocket, buffer, strlen(buffer), 0, (struct sockaddr*) &addr_s, addrlen_s);
                if (n == -1) /*error*/ exit(1);   

                memset(command, '\0', strlen(command) * sizeof(char));
                memset(vc, '\0', strlen(vc) * sizeof(char));
                memset(op_name, '\0', strlen(op_name) * sizeof(char));
                memset(fop, '\0', strlen(fop) * sizeof(char));
                memset(filename, '\0', strlen(filename) * sizeof(char));
                memset(buffer, '\0', strlen(buffer) * sizeof(char));
            }
        }
    }
}
