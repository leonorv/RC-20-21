#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
using namespace std;

#define PORT "58002"
// #define IP "192.168.1.10"
#define SIZE 128

struct addrinfo hints, *res;
int fd, errcode;
struct sockaddr_in addr;
socklen_t addrlen;
ssize_t n, nread;
char buffer[SIZE];
char *pdip, *asip, *pd_port, *as_port;
char host[256];
char uid[5];
char pass[8];

static void parseArgs(long argc, char* const argv[]) {
    //verificacoes
    /*pdip = argv[1];
    pd_port = argv[2];
    asip = argv[3];
    as_port = argv[4];*/
}
    
int main(int argc, char* argv[]) {
    
    int hostname = gethostname(host, sizeof(host));
    pdip = host;
    asip = host;
    
    cin.getline(buffer, SIZE);
    //parseArgs(argc, argv);
    sscanf(buffer, "REG %s %s", uid, pass);
    printf("%s\n", uid);
    printf("%s\n", pass);

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        exit(1); //error

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;//IPv4
    hints.ai_socktype = SOCK_DGRAM;//UDP socket
    hints.ai_flags = AI_PASSIVE;

    if ((errcode = getaddrinfo(NULL, PORT, &hints, &res)) != 0) 
        /*error*/ exit(1);
    
    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) {
        
        exit(1);
    }
        /*error*/

    while(1) {
        addrlen = sizeof(addr);
        nread = recvfrom(fd, buffer, SIZE, 0, (struct sockaddr*)&addr, &addrlen);
        if(nread == -1)
            //error
            exit(1);
        n = sendto(fd, buffer, nread, 0, (struct sockaddr*)&addr, addrlen);
        if(n==-1)
            //error
            exit(1);
        
    }
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