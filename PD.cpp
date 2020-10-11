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
    int udpServerSocket, afd = 0, errcode;
    fd_set readfds;
    int maxfd, retval;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[SIZE], msg[SIZE];
    char ASIPbuffer[SIZE];
    char command[3], uid[5], password[8];

    if (gethostname(ASIPbuffer ,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));
    strcpy(ASIP, ASIPbuffer);


    udpServerSocket = socket(AF_INET, SOCK_DGRAM, 0);//UDP socket
    if(udpServerSocket == -1)/*error*/exit(1);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;//IPv4
    hints.ai_socktype = SOCK_DGRAM;//UDP socket

    errcode = getaddrinfo(IP, PORT, &hints ,&res);
    if (errcode != 0)/*error*/exit(1);

    processInput(argc, argv);

    while(1){
        FD_ZERO(&readfds);
        FD_SET(afd, &readfds);
        FD_SET(udpServerSocket, &readfds);
        maxfd = max(udpServerSocket, afd);

        retval = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (retval <= 0)/*error*/exit(1);
        
        for (; retval; retval--){
            if(FD_ISSET(afd, &readfds)){
                FD_CLR(afd, &readfds);
                fgets(msg, SIZE, stdin);
                strtok(msg, "\n");
                if (strcmp(msg, "exit") == 0) {
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
                memset(buffer, '\0', SIZE * sizeof(char));
             }
        }
    }
    freeaddrinfo(res);
    close(udpServerSocket);
}