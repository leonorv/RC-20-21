#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#define PORT "58001"
#define SIZE 128

int main(void)
{
    struct addrinfo hints,* res;
    int fd, newfd ,errcode; 
    ssize_t n,nw;
    struct sockaddr_in addr; 
    socklen_t addrlen;
    char *ptr,buffer[SIZE];

    if((fd=socket(AF_INET,SOCK_STREAM,0))==-1) exit(1);//error

    memset(&hints, sizeof hints);
    hints.ai_family=AF_INET;//IPv4
    hints.ai_socktype=SOCK_STREAM;//TCP socket
    hints.ai_flags=AI_PASSIVE;

    if((errcode=getaddrinfo(NULL,PORT,&hints,&res))!= 0)/*error*/exit(1);

    if(bind(fd,res->ai_addr,res->ai_addrlen)==-1)/*error*/exit(1);
    if(listen(fd,5)==-1)/*error*/exit(1);

    while(1)
    { 
        addrlen=sizeof(addr);
        if((newfd=accept(fd,(struct sockaddr*)&addr,&addrlen))==-1)/*error*/exit(1);
        while((n=read(newfd,buffer,SIZE))!=0
        {
            if(n==-1)/*error*/exit(1);
            ptr=&buffer[0];
            while(n>0)
            {
                if((nw=write(newfd,ptr,n))<=0)/*error*/exit(1);
                 n-=nw; 
                 ptr+=nw;
            } 
        }
    close(newfd);
    }
//freeaddrinfo(res);close(fd);exit(0);
}