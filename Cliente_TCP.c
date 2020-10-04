#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>

#define PORT "58001"
#define IP "tejo.tecnico.ulisboa.pt"
#define SIZE 128

int main(void)
{
    struct addrinfo hints, *res;
    int fd, n;
    ssize_t nbytes,nleft,nwritten,nread;
    char *ptr,buffer[SIZE];
    struct sigaction act;

    ptr=strcpy(buffer,"Hello!\n");
    nbytes=7;

    fd=socket(AF_INET, SOCK_STREAM,0);//TCP socket
    if(fd==-1)/*error*/exit(1);

    memset(&hints, 0, sizeof hints);
    hints.ai_family=AF_INET;//IPv4
    hints.ai_socktype=SOCK_STREAM;//TCP socket

    memset(&hints, 0, sizeof act);
    act.__sigaction_handler=SIG_IGN;

    if(sigaction(SIGPIPE,&act,NULL)==-1)/*error*/exit(1);

    n=getaddrinfo(IP, PORT, &hints, &res);  
    if(n!=0)/*error*/exit(1);

    n=connect(fd, res->ai_addr, res->ai_addrlen);
    if(n==-1)/*error*/exit(1);

    nleft=nbytes;
    while(nleft>0)
    {
        nwritten = write(fd,ptr,nleft);
        if (nwritten<=0)/*error*/exit(1);
        nleft-=nwritten;
        ptr+=nwritten;
    }
    nleft=nbytes; 
    ptr=buffer;
    while(nleft>0)
    {
        nread= read(fd,ptr,nleft);
        if(nread==-1)/*error*/ exit(1);
        else if(nread==0) break;//closed by peer
        nleft--=nread;
        ptr+=nread;
    }
    nread=nbytes-nleft;  
    close(fd);

    write(1, "echo: ", 6);//stout
    write(1, buffer, nread);
    exit(0);
}