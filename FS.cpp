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
    int udpClientSocket, tcpServerSocket;
    fd_set readfds;
    int maxfd, retval;
    struct addrinfo hints_udp, hints_tcp, *res_udp, *res_tcp;
    int fd, newfd, errcode;
    struct sockaddr_in addr_udp, addr_tcp;
    socklen_t addrlen_udp, addrlen_tcp;
    ssize_t n, nread, nw;
    char *ptr, buffer[SIZE], check[3], command[4], password[8], uid[6]="", filename[50], vc[5], op_name[16], tid[5];

    if (gethostname(ASIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));

    /*==========================
    Setting up UDP Server Socket
    ==========================*/
    udpClientSocket = socket(AF_INET, SOCK_DGRAM, 0);//TCP socket
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
    errcode = getaddrinfo(NULL, ASport, &hints_tcp, &res_tcp);
    if (errcode != 0)/*error*/exit(1);
    if (bind(tcpServerSocket, res_tcp->ai_addr, res_tcp->ai_addrlen) < 0) exit(1);

    /*==========================
        process standard input
    ==========================*/
    processInput(argc, argv);

    while (1){
        FD_ZERO(&readfds);
        FD_SET(udpClientSocket, &readfds);
        FD_SET(tcpServerSocket, &readfds);

        if(listen(tcpServerSocket,5)==-1)/*error*/exit(1);
        /*==========================
        Pick the active file descriptor(s)
        ==========================*/
        maxfd = max(tcpServerSocket, udpServerSocket);
        retval = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (retval <= 0)/*error*/exit(1);
        
        for (; retval; retval--) {
            if(FD_ISSET(udpServerSocket, &readfds))
            {
                addrlen_udp = sizeof(addr_udp);
                n = recvfrom(udpServerSocket, buffer, 128, 0, (struct sockaddr*) &addr_udp, &addrlen_udp);
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
                    strcpy(op_name, token);
                    token = strtok(NULL, " ");
                    strcpy(filename, token);
                }
                else
                {
                    printf("ERR\n");
                }  
                    /* Print confirmation code to terminal */
                    printf("VC=%s, %s%s", vc, op_name, filename);

                    /*copy confirmation message to buffer
                      to send to AS*/
                    strcpy(buffer, "RVC OK\n");
                    n = strlen(buffer);
                
                /*send confirmation message to AS*/
                n = sendto(udpServerSocket, buffer, n, 0, (struct sockaddr*) &addr_udp, addrlen_udp);
                if (n == -1)/*error*/exit(1);   
                memset(buffer, '\0', SIZE * sizeof(char));
            }
            else if(FD_ISSET(tcpServerSocket, &readfds))
            {
                addrlen_tcp=sizeof(addr_tcp);
                if ((newfd=accept(fd,(struct sockaddr*)&addr_tcp,&addrlen_tcp))==-1)/*error*/exit(1);
                while((n=read(newfd,buffer,SIZE))!=0) {
                if (n==-1)/*error*/exit(1);

                char *token = strtok(buffer, " ");
                strcpy(command, token);

                if (strcmp(command, "RUP") == 0) {
                    token = strtok(NULL, " ");
                    strcpy(check, token);
                }
                else
                {
                    printf("ERR\n");
                }  
                    ptr = &buffer[0];
                    while (n>0) {
                        if ((nw=write(newfd,ptr,n))<=0)/*error*/exit(1);
                        n-=nw; 
                        ptr+=nw;
                    } 
                close(newfd);
                }
            }
        }
    }
}      