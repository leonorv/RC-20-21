#include <unistd.h>
#include <stdlib.h>
#include <string.h>
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
char *pdip, *asip;
char host[256];
// int pd_port, as_port;

static void parseArgs(long argc, char* const argv[]) {
    // verificacoes
    //pdip = argv[1];
    // outras flags
}

void processInput() {
    string line;
    getline(cin, line);
    cout << line << endl;
}

    
int main(int argc, char* argv[]) {

    int hostname = gethostname(host, sizeof(host));
    pdip = host;
    asip = host;
    
    parseArgs(argc, argv);
    
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
        processInput();
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