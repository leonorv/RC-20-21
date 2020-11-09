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

char ASIP[SIZE], ASport[SIZE] = "58030", FSIP[SIZE], FSport[SIZE] = "59030";

void processInput(int argc, char* const argv[]) {
    if (argc%2 != 1) {
        printf("Parse error!\n");
        exit(1);
    }
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-q") == 0) {
            strcpy(FSport, argv[i + 1]);
            continue;
        }
        if (strcmp(argv[i], "-n") == 0) {
            strcpy(ASIP, argv[i + 1]);
            continue;
        }
        if (strcmp(argv[i], "-p") == 0) {
            strcpy(ASport, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-v") == 0) {
            //Ativar verbose mode
            continue;
        }
    }
}

int main(int argc, char* argv[]) {
    const int maxUsers = 5;
    int udpClientSocket, tcpServerSocket;
    fd_set readfds;
    int maxfd, retval;
    struct addrinfo hints_udp, hints_tcp, *res_udp, *res_tcp;
    int fd, newfd, errcode;
    struct sockaddr_in addr_udp, addr_tcp, addr;
    socklen_t addrlen_udp, addrlen_tcp, addrlen;
    ssize_t n, nread, nw;
    int fdClients[maxUsers];
    char *ptr, buffer[SIZE], check[3], command[4], password[8], uid[6]="", filename[50], vc[5], op_name[16], tid[5], fop[3];

    if (gethostname(ASIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));
    
    if (gethostname(FSIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));

    /*==========================
    Setting up UDP Server Socket
    ==========================*/
    udpClientSocket = socket(AF_INET, SOCK_DGRAM, 0);//UDP socket
    if(udpClientSocket == -1)/*error*/exit(1);

    memset(&hints_udp, 0, sizeof hints_udp);
    hints_udp.ai_family = AF_INET;//IPv4
    hints_udp.ai_socktype = SOCK_DGRAM;//UDP socket

    errcode = getaddrinfo(ASIP, ASport, &hints_udp, &res_udp);
    if (errcode != 0)/*error*/exit(1);

    /*==========================
    Setting up TCP Server Socket
    ==========================*/
    tcpServerSocket = socket(AF_INET, SOCK_STREAM, 0);//TCP socket
    if(tcpServerSocket == -1)/*error*/exit(1);
    memset(&hints_tcp, 0, sizeof hints_tcp);
    hints_tcp.ai_family = AF_INET;//IPv4
    hints_tcp.ai_socktype = SOCK_STREAM;//TCP socket
    hints_tcp.ai_flags = AI_PASSIVE;
    errcode = getaddrinfo(NULL, FSport, &hints_tcp, &res_tcp);
    if (errcode != 0)/*error*/exit(1);
    if (bind(tcpServerSocket, res_tcp->ai_addr, res_tcp->ai_addrlen) < 0) exit(1);

    /* Initialize TCP babies fds */
    for (int i = 0; i < maxUsers; i++) {   
        fdClients[i] = 0;
    } 

    /*==========================
        process standard input
    ==========================*/
    processInput(argc, argv);

    if (listen(tcpServerSocket, maxUsers) == -1)
        /*error*/ perror("listen tcpserversocket");

    while (1){
        FD_ZERO(&readfds);
        FD_SET(udpClientSocket, &readfds);
        FD_SET(tcpServerSocket, &readfds);
        maxfd = tcpServerSocket;

        /*==========================
        Create child sockets
        ==========================*/
        for (int i = 0 ; i < maxUsers ; i++) {   
            //socket descriptor  
            fd = fdClients[i];   
                 
            //if valid socket descriptor then add to read list  
            if(fd > 0)   
                FD_SET(fd , &readfds);   
                 
            //highest file descriptor number, need it for the select function  
            if(fd > maxfd)   
                maxfd = fd;   
        }
        
        /*==========================
        Pick the active file descriptor(s)
        ==========================*/
        maxfd = max(maxfd, udpClientSocket);
        retval = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (retval <= 0)/*error*/exit(1);
        
        for (; retval; retval--) {
            if(FD_ISSET(udpClientSocket, &readfds))
            {
                addrlen_udp = sizeof(addr_udp);
                n = recvfrom(udpClientSocket, buffer, 128, 0, (struct sockaddr*) &addr_udp, &addrlen_udp);
                if (n == -1)/*error*/exit(1);
                buffer[n] = '\0';

                /*separate command*/
                char *token = strtok(buffer, " ");
                strcpy(command, token);

                if (strcmp(command, "CNF") == 0) {
                    token = strtok(NULL, " ");
                    strcpy(uid, token);
                    token = strtok(NULL, " ");
                    strcpy(tid, token);
                    token = strtok(NULL, " ");
                    strcpy(fop, token);
                    token = strtok(NULL, " ");
                    strcpy(filename, token);

                    if (strcmp(fop, "U") == 0) {
                        strcpy(buffer,"RUP");
                    }
                    else if (strcmp(fop, "D") == 0) {
                        strcpy(buffer,"RDL");
                    }
                    else if (strcmp(fop, "R") == 0) {
                        strcpy(buffer,"RRT");
                    }
                    else if (strcmp(fop, "L") == 0) {
                        strcpy(buffer,"RLS");
                    }
                    else if (strcmp(fop, "X") == 0) {
                        strcpy(buffer,"RRM");
                    }
                    else if (strcmp(fop, "E") == 0) {
                        strcpy(buffer,"E");
                    }
                else
                {
                    printf("ERR\n");
                }  
                    
                    printf("operation validated\n");

                /*send confirmation message to AS*/
                n = sendto(tcpServerSocket, buffer, strlen(buffer), 0, (struct sockaddr*) &addr_tcp, addrlen_tcp);
                if (n == -1)/*error*/exit(1);   
                memset(buffer, '\0', SIZE * sizeof(char));
            }
            }
            else if(FD_ISSET(tcpServerSocket, &readfds))
            {
                addrlen_tcp = sizeof(addr_tcp);
                if ((newfd = accept(tcpServerSocket, (struct sockaddr*)&addr_tcp, &addrlen_tcp)) == -1) { perror("accept tcp server socket"); exit(1); }

                //add new socket to array of sockets  
                for (int i = 0; i < maxUsers; i++) {   
                    //if position is empty  
                    if(fdClients[i] == 0 ) {  
                        fdClients[i] = newfd;   
                        break;   
                    }   
                }
            }
            for (int i = 0; i < maxUsers; i++) {   
                fd = fdClients[i];  

                if (FD_ISSET(fd, &readfds)) { 

                    int n = read(fd, buffer, SIZE);

                    if (n == 0) {
                        getpeername(fd, (struct sockaddr*)&addr, (socklen_t*)&addrlen);

                        //Close the socket and mark as 0 in list for reuse
                        close(fd);
                        fdClients[i] = 0;
                        // /* error */ exit(1);
                    }
                    else if (n == -1) { perror("fdclients read"); exit(1); }
                    else {
                        char *token = strtok(buffer, " ");
                        strcpy(command, token);

                        if (strcmp(command, "LST") == 0) {
                            token = strtok(NULL, " ");
                            strcpy(check, token);
                        }
                        else if (strcmp(command, "RTV") == 0) {
                            token = strtok(NULL, " ");
                            strcpy(check, token);
                        }
                        else if (strcmp(command, "UPL") == 0) {
                            token = strtok(NULL, " ");
                            strcpy(check, token);
                        }
                        else if (strcmp(command, "DEL") == 0) {
                            token = strtok(NULL, " ");
                            strcpy(check, token);
                        }
                        else if (strcmp(command, "REM") == 0) {
                            token = strtok(NULL, " ");
                            strcpy(check, token);
                        }
                        else
                        {
                            printf("ERR\n");
                        }  
                        
                        strcpy(buffer,"VLD");

                        /*send confirmation message to AS*/
                        n = sendto(udpClientSocket, buffer, strlen(buffer), 0, (struct sockaddr*) &addr_udp, addrlen_udp);
                        if (n == -1)/*error*/exit(1);   
                    }
                    memset(buffer, '\0', SIZE * sizeof(char));    
                }
            }
        }
    }
}      