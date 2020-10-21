#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

using namespace std;

#define max(A,B)((A)>=(B)?(A):(B))
#define IP "tejo.tecnico.ulisboa.pt"
#define SIZE 128

extern int errno;
char PDIP[SIZE], ASIP[SIZE], PDport[SIZE] = "57030", ASport[SIZE] = "58011";
char fixedReg[SIZE];

void processInput(int argc, char* const argv[]) {
    if (argc < 2)
        exit(1);
    strcpy(PDIP, argv[1]);
    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            if (strlen(argv[i + 1]) > SIZE) exit(1);
            strcpy(PDport, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-n") == 0) {
            if (strlen(argv[i + 1]) > SIZE) exit(1);
            strcpy(ASIP, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-p") == 0) {
            if (strlen(argv[i + 1]) > SIZE) exit(1);
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
    int udpServerSocket,udpClientSocket, afd = 0, errcode_c,errcode_s,fd;
    fd_set readfds;
    int maxfd, retval;
    ssize_t n;
    socklen_t addrlen_c, addrlen_s;
    struct addrinfo hints_c, hints_s, *res_c, *res_s;
    struct sockaddr_in addr_c, addr_s;
    char buffer[SIZE], msg[SIZE];
    char command[4], uid[5], password[8], filename[50], vc[5], op_name[16], confirmation[8];
    // char fop[2];
    char fop[3];

    if (gethostname(ASIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));

    /*==========================
    Setting up UDP Client Socket
    ==========================*/
    udpClientSocket = socket(AF_INET, SOCK_DGRAM, 0);//UDP socket
        if(udpClientSocket == -1)/*error*/exit(1);

    memset(&hints_c, 0, sizeof hints_c);
    hints_c.ai_family = AF_INET;//IPv4
    hints_c.ai_socktype = SOCK_DGRAM;//UDP socket

    errcode_c = getaddrinfo(IP, ASport, &hints_c ,&res_c);
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

    /*==========================
        process standard input
    ==========================*/
    processInput(argc, argv);
    
    while (1){
        FD_ZERO(&readfds);
        FD_SET(afd, &readfds);
        FD_SET(udpClientSocket, &readfds);
        FD_SET(udpServerSocket, &readfds);

        /*==========================
        Pick the active file descriptor(s)
        ==========================*/
        maxfd = max(udpClientSocket, udpServerSocket);
        retval = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (retval <= 0)/*error*/exit(1);
        
        for (; retval; retval--){
            if(FD_ISSET(afd, &readfds)){
                fgets(msg, SIZE, stdin);
                /* msg is input written in standard input*/
                strtok(msg, "\n");
                if (strcmp(msg, "exit") == 0) {
                    /*====================================================
                    Free data structures and close socket connections
                    ====================================================*/
                    freeaddrinfo(res_c);
                    freeaddrinfo(res_s);
                    close(udpClientSocket);
                    close(udpServerSocket);
                    exit(1);
                }
                else {
                    /*====================================================
                    Send message from stdin to the server
                    ====================================================*/
                    strcat(strcat(msg, " "), fixedReg);              
                    n = sendto(udpClientSocket, msg, strlen(msg), 0, res_c->ai_addr, res_c->ai_addrlen);
                    if (n == -1)/*error*/exit(1);     
                }
                memset(msg, '\0', SIZE * sizeof(char));
            }
            if (FD_ISSET(udpClientSocket, &readfds)) {
                /*====================================================
                Receive message from AS in response to PD request
                ====================================================*/
                addrlen_c = sizeof(addr_c);
                n = recvfrom(udpClientSocket, buffer, 128, 0, (struct sockaddr*) &addr_c, &addrlen_c);
                if (n == -1)/*error*/exit(1);
                buffer[n] = '\0';

                write(1, buffer, n);
                if(strcmp(buffer, "RRG OK\n") == 0){
                    printf("Registration successful\n");  
                }
                /* reset buffer */
                memset(buffer, '\0', SIZE * sizeof(char));
            }
            if (FD_ISSET(udpServerSocket, &readfds)) {
                /*====================================================
                Receive messages from AS unprovoked
                ====================================================*/
                addrlen_s = sizeof(addr_s);
                n = recvfrom(udpServerSocket, buffer, 128, 0, (struct sockaddr*) &addr_s, &addrlen_s);
                if (n == -1)/*error*/exit(1);
                buffer[n] = '\0';

                /*separate command*/
                char *token = strtok(buffer, " ");
                strcpy(command, token);

                /*process VLCs*/
                /*VLC UID VC FOP FILENAME\n
                  or
                  VLC UID VC FOP\n
                */
                if (strcmp(command, "VLC") == 0) {
                    token = strtok(NULL, " ");
                    strcpy(uid, token);
                    token = strtok(NULL, " ");
                    strcpy(vc, token);
                    token = strtok(NULL, " ");
                    strcpy(fop, token);
                    if (strcmp(fop, "L") != 0 && strcmp(fop, "X") != 0) {
                        token = strtok(NULL, " ");
                        strcpy(filename, token);
                    } else {
                        filename[0] = '\0';
                    }
                    //int a_as_int = (int)'a';
                    if (strcmp(fop, "U") == 0) {
                        strcpy(op_name, "upload: ");
                    } 
                    else if (strcmp(fop, "R") == 0) {
                        strcpy(op_name, "retrieve: ");
                    } 
                    else if (strcmp(fop, "D") == 0) {
                        strcpy(op_name, "retrieve: ");
                    } 
                    else if (strcmp(fop, "L\n") == 0) {
                        strcpy(op_name, "list\n");
                    }  
                    else if (strcmp(fop, "X\n") == 0) {
                        strcpy(op_name, "remove\n");
                    }
                    else {
                        printf("Error!\n");
                    }
                    
                    /* Print confirmation code to terminal */
                    printf("VC=%s, %s%s", vc, op_name, filename);

                    /*copy confirmation message to buffer
                      to send to AS*/
                    strcpy(buffer, "RVC OK\n");
                    n = strlen(buffer);
                }
                /*send confirmation message to AS*/
                n = sendto(udpServerSocket, buffer, n, 0, (struct sockaddr*) &addr_s, addrlen_s);
                if (n == -1)/*error*/exit(1);   
                memset(buffer, '\0', SIZE * sizeof(char));
            }
        }
    }
}