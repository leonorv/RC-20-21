#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
using namespace std;

#define PORT "58011"
#define IP "tejo.tecnico.ulisboa.pt"
#define SIZE 128

extern int errno;
char PDIP[SIZE], ASIP[SIZE], PDport[6] = "57030", ASport[6] = "58030";

void processInput(int argc, char* const argv[]) {

    //faltam verificacoes de qtds de argumentos etc acho eu
    strcpy(PDIP, argv[1]);
    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            strcpy(PDport, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-n") == 0) {
            strcpy(ASIP, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-p") == 0) {
            strcpy(ASport, argv[i + 1]);
            continue;
        }
    }
}

int main(int argc, char* const argv[]){
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[SIZE];
    char msg[SIZE];
    char command[3];
    char uid[5];
    char password[8];
    
    scanf("%[^\t\n]", msg);

    gethostname(ASIP, SIZE);

    processInput(argc, argv);

    fd = socket(AF_INET, SOCK_DGRAM, 0);//UDP socket
    if(fd == -1)
        /*error*/exit(1);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;//IPv4
    hints.ai_socktype = SOCK_DGRAM;//UDP socket

    errcode = getaddrinfo(IP, PORT, &hints ,&res);
    if (errcode != 0)/*error*/exit(1);

    strcat(strcat(msg, " "), strcat(strcat(PDIP, " "), strcat(PDport, "\n")));

    n = sendto(fd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1)/*error*/exit(1);

    addrlen=sizeof(addr);
    n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
    if (n == -1)/*error*/exit(1);

    write(1, buffer, n);

    freeaddrinfo(res);
    close(fd);
    exit(0);
}

/* SELECT
int main(void) {
    char in_str[128];
    fd_set inputs, testfds;
    struct timeval timeout;
    int i, out_fds, n;
    FD_ZERO(&inputs); // Clear inputs
    FD_SET(0, &inputs); // Set standard input channel on
    printf("Size of fd_set: %d\n", sizeof(fd_set));
    printf("Value of FD_SETSIZE: %d\n", FD_SETSIZE);
    while(1) {
        testfds = inputs;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        printf("testfds byte: %d\n", ((char *)&testfds)[0]);
        out_fds = select(FD_SETSIZE, &testfds, (fd_set *)NULL, (fd_set *)NULL, &timeout);
        printf("Time = %d and %d\n", timeout.tv_sec, timeout.tv_usec);
        printf("testfds byte: %d\n", ((char *)&testfds)[0]);
        switch(out_fds) {
            case 0:
                printf("Timeout event\n");
                break;
            case -1:
                perror("select");
                exit(1);
            default:
                if(FD_ISSET(0,&testfds)) {
                    if((n=read(0,in_str,127))!=0) {
                        if(n==-1)
                        exit(1);
                        in_str[n]=0;
                        printf("From keyboard: %s\n",in_str);
                    }
                }
            }
        }
    }*/