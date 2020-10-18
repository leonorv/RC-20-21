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
#define PORT "58011"
#define IP "tejo.tecnico.ulisboa.pt"
#define SIZE 128

extern int errno;
char PDIP[SIZE], ASIP[SIZE], PDport[SIZE] = "57030", ASport[SIZE] = "58030";
char fixedReg[SIZE];

void processInput(int argc, char* const argv[]) {
    //faltam verificacoes de qtds de argumentos etc acho eu
    if (argc < 2)
        exit(1);
    strcpy(PDIP, argv[1]);
    //PDIP = strdup(argv[1]);
    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            if (strlen(argv[i + 1]) > SIZE) exit(1);
            strcpy(PDport, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-n") == 0) {
            if (strlen(argv[i + 1]) > SIZE) exit(1);
            //ASIP = strdup(argv[i + 1]);
            strcpy(ASIP, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-p") == 0) {
            if (strlen(argv[i + 1]) > SIZE) exit(1);
            //ASport = strdup(argv[i + 1]);
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
<<<<<<< HEAD
    int udpServerSocket,udpClientSocket, afd = 0, errcode_c,errcode_s,fd;
    fd_set readfds;
    int maxfd, retval;
    ssize_t n;
    socklen_t addrlen_c, addrlen_s;
    struct addrinfo hints_c,hints_s, *res_c, *res_s;
    struct sockaddr_in addr_c, addr_s;
    char buffer[SIZE], msg[SIZE];
    char command[3], uid[5], password[8];

    if (gethostname(ASIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));
=======
    int udpServerSocket, afd = 0, errcode;
    fd_set readfds;
    int maxfd, retval;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[SIZE], msg[SIZE];
    char command[3], uid[5], password[8];

    if (gethostname(ASIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));


    udpServerSocket = socket(AF_INET, SOCK_DGRAM, 0);//UDP socket
    if(udpServerSocket == -1)/*error*/exit(1);
>>>>>>> 716cfb8107dcd4782dd8026def24fbf435e63b45

    udpClientSocket = socket(AF_INET, SOCK_DGRAM, 0);//UDP socket
        if(udpClientSocket == -1)/*error*/exit(1);

    memset(&hints_c, 0, sizeof hints_c);
    hints_c.ai_family = AF_INET;//IPv4
    hints_c.ai_socktype = SOCK_DGRAM;//UDP socket

<<<<<<< HEAD
     errcode_c = getaddrinfo(IP, PORT, &hints_c ,&res_c);
        if (errcode_c != 0)/*error*/exit(1);
    
    udpServerSocket = socket(AF_INET, SOCK_DGRAM, 0);//UDP socket
    if(udpServerSocket == -1)/*error*/exit(1);

    memset(&hints_s, 0, sizeof hints_s);
    hints_s.ai_family = AF_INET;//IPv4
    hints_s.ai_socktype = SOCK_DGRAM;//UDP socket
    hints_s.ai_flags = AI_PASSIVE;

     errcode_s = getaddrinfo(NULL, PORT, &hints_s ,&res_s);
        if (errcode_s != 0)/*error*/exit(1);

    if(bind(udpServerSocket,res_s->ai_addr,res_s->ai_addrlen) < 0 )
    {
        printf (" Unable to bind socket \n");
    }
    else
    {
        printf (" Bound to socket .\n");
    }

    processInput(argc, argv);
    
    while(1){
        FD_ZERO(&readfds);
        FD_SET(afd, &readfds);
        FD_SET(udpClientSocket, &readfds);
        FD_SET(udpServerSocket, &readfds);
        maxfd = max(udpClientSocket, udpServerSocket);
=======
    processInput(argc, argv);

    while(1){
        FD_ZERO(&readfds);
        FD_SET(afd, &readfds);
        FD_SET(udpServerSocket, &readfds);
        maxfd = max(udpServerSocket, afd);
>>>>>>> 716cfb8107dcd4782dd8026def24fbf435e63b45

        retval = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (retval <= 0)/*error*/exit(1);
        
        for (; retval; retval--){
            if(FD_ISSET(afd, &readfds)){
                FD_CLR(afd, &readfds);
                fgets(msg, SIZE, stdin);
                strtok(msg, "\n");
                if (strcmp(msg, "exit") == 0) {
<<<<<<< HEAD
                    freeaddrinfo(res_c);
                    freeaddrinfo(res_s);
                    close(udpClientSocket);
                    close(udpServerSocket);
                    exit(1);
                }
                else {
                    strcat(strcat(msg, " "), fixedReg);              
                    n = sendto(udpClientSocket, msg, strlen(msg), 0, res_c->ai_addr, res_c->ai_addrlen);
                    if (n == -1)/*error*/exit(1);       
                }
                memset(msg, '\0', SIZE * sizeof(char));
            }
            else if (FD_ISSET(udpClientSocket, &readfds)) {
                FD_CLR(udpClientSocket, &readfds);
                addrlen_c=sizeof(addr_c);
                n = recvfrom(udpClientSocket, buffer, 128, 0, (struct sockaddr*)&addr_c, &addrlen_c);
                    if (n == -1)/*error*/exit(1);

                write(1, buffer, n);
                memset(buffer, '\0', SIZE * sizeof(char));
             }
             else if (FD_ISSET(udpServerSocket, &readfds)) {
                printf("xxxxxxxxxxxxxxxxxxxxxxxxxxx");
                FD_CLR(udpServerSocket, &readfds);

                addrlen_s=sizeof(addr_s);
                n = recvfrom(udpServerSocket, buffer, 128, 0, (struct sockaddr*)&addr_s, &addrlen_s);
                    if (n == -1)/*error*/exit(1);

                write(1, buffer, n);

                n = sendto(udpServerSocket, msg, strlen(buffer), 0, res_s->ai_addr, res_s->ai_addrlen);
                    if (n == -1)/*error*/exit(1);   

=======
                    freeaddrinfo(res);
                    close(udpServerSocket);
                    exit(1);
                }
                else {
                    strcat(strcat(msg, " "), fixedReg);    
                    printf("to send : %s\n", msg);               
                    n = sendto(udpServerSocket, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen);
                    if (n == -1)/*error*/exit(1);                    
                }
                
                memset(msg, '\0', SIZE * sizeof(char));
            }
            else if (FD_ISSET(udpServerSocket, &readfds)) {
                FD_CLR(udpServerSocket, &readfds);
                addrlen=sizeof(addr);
                n = recvfrom(udpServerSocket, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
                    if (n == -1)/*error*/exit(1);

                write(1, buffer, n);
>>>>>>> 716cfb8107dcd4782dd8026def24fbf435e63b45
                memset(buffer, '\0', SIZE * sizeof(char));
             }
        }
    }
<<<<<<< HEAD
=======
    freeaddrinfo(res);
    close(udpServerSocket);
>>>>>>> 716cfb8107dcd4782dd8026def24fbf435e63b45
}