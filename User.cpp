#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>

#define PORT "58011"
#define IP "tejo.tecnico.ulisboa.pt"
#define SIZE 128

char ASIP[SIZE], ASport[SIZE] = "57030", FSIP[SIZE], FSport[SIZE] = "59030";

void processInput(int argc, char* const argv[]) {

    //faltam verificacoes de qtds de argumentos etc acho eu
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            strcpy(ASport, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-n") == 0) {
            strcpy(ASIP, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-m") == 0) {
            strcpy(FSIP, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-q") == 0) {
            strcpy(FSport, argv[i + 1]);
            continue;
        }
    }
}


int main(int argc, char* const argv[]) {
    struct addrinfo hints, *res;
    int fd, n;
    ssize_t nbytes, nleft, nwritten, nread;
    char *ptr, buffer[SIZE], msg[SIZE];
    struct sigaction act;

    gethostname(FSIP, SIZE);
    gethostname(ASIP, SIZE);

    processInput(argc, argv);

    fgets(msg, SIZE, stdin);

    ptr = (char*) malloc(strlen(msg) + 1);
    strcpy(ptr, msg);

    strcat(ptr, "\0");
    
    nbytes = strlen(ptr);

    fd = socket(AF_INET, SOCK_STREAM, 0);//TCP socket
    if (fd == -1)/*error*/exit(1);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;//IPv4
    hints.ai_socktype = SOCK_STREAM;//TCP socket

    memset(&hints, 0, sizeof(act));
    act.sa_handler = SIG_IGN;

    if (sigaction(SIGPIPE, &act, NULL) == -1)/*error*/exit(1);

    n = getaddrinfo(IP, PORT, &hints, &res);  
    if (n != 0)/*error*/exit(1);

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)/*error*/{
        //printf("%u", res->ai_addrlen);
        //printf("connect\n");
        exit(1);
    }

    nleft = nbytes;
    while (nleft > 0) {
        nwritten = write(fd, ptr, nleft);
        printf("write\n");
        if (nwritten <= 0)/*error*/exit(1);
        nleft -= nwritten;
        ptr += nwritten;
}
    nleft = nbytes; 
    ptr = buffer;
    while (nleft > 0) {
        nread = read(fd, ptr, nleft);
        printf("read: %ld\n", nread);
        if (nread == -1)/*error*/ exit(1);
        else if (nread == 0) break;//closed by peer
        nleft -= nread;
        ptr += nread;
    }
    nread = nbytes - nleft;  
    close(fd);

    write(1, buffer, nread);
    printf("escreveu");
    exit(0);
}