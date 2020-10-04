#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define PORT "58002"
#define IP "192.168.1.10"
#define SIZE 128

extern int errno;

int main(void)
{
    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints,*res;
    struct sockaddr_in addr;
    char buffer[SIZE];

    char msg[SIZE];
    scanf("%s", msg);

    fd=socket(AF_INET,SOCK_DGRAM,0);//UDP socket
    if(fd==-1)
        /*error*/exit(1);

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;//IPv4
    hints.ai_socktype=SOCK_DGRAM;//UDP socket

    errcode=getaddrinfo(IP,PORT,&hints,&res);
    if(errcode!=0)/*error*/exit(1);

    n=sendto(fd,msg,7,0,res->ai_addr,res->ai_addrlen);
    if(n == -1)/*error*/exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
    if(n == -1)/*error*/exit(1);

    write(1, "echo: ", 6);//stdout
    write(1, buffer, n);

    freeaddrinfo(res);
    close(fd);
exit(0);
}
