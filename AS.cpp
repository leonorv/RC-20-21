#include <stdlib.h>
#include <stdio.h>
#include <map> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h> 


using namespace std;

#define max(A,B)((A)>=(B)?(A):(B))
//#define IP "tejo.tecnico.ulisboa.pt"
#define SIZE 128

extern int errno;

char PDIP[SIZE], PDport[SIZE] = "57030", ASIP[SIZE], ASport[SIZE] = "58030", FSIP[SIZE], FSport[SIZE] = "59030";

const int maxUsers = 5;
FILE *fptr;
int udpServerSocket, tcpServerSocket, udpClientSocket;
fd_set readfds;
int maxfd, retval;
int fd, newfd, errcode;
struct addrinfo hints_uc, hints_us, hints_ts, *res_uc, *res_ts, *res_us;
struct sockaddr_in addr;
socklen_t addrlen;
ssize_t n, nread, nw;
char buffer[SIZE], command[4], password[SIZE], uid[SIZE], *ptr;
int connectedUsers = 0;
int fdClients[maxUsers];
char *dirName;
char pdport[SIZE], pdip[SIZE];

void processInput(int argc, char* const argv[]) {
    if (argc%2 != 1) {
        printf("Parse error!\n");
        exit(1);
    }
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            strcpy(ASport, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-v") == 0) {
            //Ativar verbose mode
            continue;
        }
    }
}

void setupUDPClientSocket() {
    udpClientSocket = socket(AF_INET, SOCK_DGRAM, 0); //UDP socket
    if (udpClientSocket == -1) /*error*/exit(1);
    memset(&hints_uc, 0, sizeof hints_uc);
    hints_uc.ai_family = AF_INET; //IPv4
    hints_uc.ai_socktype = SOCK_DGRAM; //UDP socket
    errcode = getaddrinfo(PDIP, PDport, &hints_uc, &res_uc);
    //guardar info somehow
    memset(PDIP, '\0', SIZE * sizeof(char));
    memset(PDport, '\0', SIZE * sizeof(char));
    if (errcode != 0) /*error*/ exit(1);
}

void setupUDPServerSocket() {
    udpServerSocket = socket(AF_INET, SOCK_DGRAM, 0);//UDP socket
    if(udpServerSocket == -1)/*error*/exit(1);
    memset(&hints_us, 0, sizeof hints_us);
    hints_us.ai_family = AF_INET;//IPv4
    hints_us.ai_socktype = SOCK_DGRAM;//UDP socket
    hints_us.ai_flags = AI_PASSIVE;
    errcode = getaddrinfo(NULL, ASport, &hints_us, &res_us);
    if (errcode != 0)/*error*/exit(1);
    if (bind(udpServerSocket, res_us->ai_addr, res_us->ai_addrlen) < 0) exit(1);
}

void setupTCPServerSocket() {
    tcpServerSocket = socket(AF_INET, SOCK_STREAM, 0);//TCP socket
    if(tcpServerSocket == -1)/*error*/exit(1);
    // printf("tcp != -1\n");
    memset(&hints_ts, 0, sizeof hints_ts);
    hints_ts.ai_family = AF_INET;//IPv4
    hints_ts.ai_socktype = SOCK_STREAM;//TCP socket
    hints_ts.ai_flags = AI_PASSIVE;
    errcode = getaddrinfo(NULL, ASport, &hints_ts, &res_ts);
    // printf("getaddrinfo\n");
    if (errcode != 0)/*error*/exit(1);
    if (bind(tcpServerSocket, res_ts->ai_addr, res_ts->ai_addrlen) < 0) exit(1);
}


int checkRegisterInput(char buffer[SIZE]) {
    
    /*=== process user ID ===*/

    sscanf(buffer, "REG %[0-9] %[0-9a-zA-Z] %[0-9.] %[0-9]\n", uid, password, pdip, pdport);
    if (uid == NULL || strlen(uid) != 5) return 0;
    if (password == NULL || strlen(password) != 8) return 0;
    if (pdip == NULL) return 0;
    if (pdport == NULL || strlen(pdport) != 5) return 0;

    dirName = strdup("users/");
    strcat(dirName, uid);
    // DIR* dir = opendir(dirName);
    // printf("%",dir);
    int error = mkdir(dirName, 0777);
    printf("erro do mkdir:%d\n", error); fflush(stdout);


    if (error == -1) { 
        // If User has already been registered
        char filename[SIZE];
        char passBuffer[9];
        printf("User has already been reg\n");
        fflush(stdout);
        
        strcpy(filename, dirName);
        strcpy(filename, "/password.txt");
        FILE *f = fopen(filename, "r");
        fgets(passBuffer, sizeof(passBuffer), f);
        printf("passbuffer: %s\n", passBuffer);
        // Check password
        if (strcmp(passBuffer, password) != 0) /*password is incorrect*/ return 0;

        fclose(f);
        memset(filename, '\0', SIZE * sizeof(char));
        memset(passBuffer, '\0', 9 * sizeof(char));


    }
    else {
        // If User has not been registered
        // Create user directory
        strcat(dirName, "/password.txt");
        ofstream userPass; 
        userPass.open(dirName);
        userPass << password;
        userPass.close();
    }
    memset(uid, '\0', SIZE * sizeof(char));
    memset(password, '\0', SIZE * sizeof(char));
    memset(pdip, '\0', SIZE * sizeof(char));
    memset(pdport, '\0', SIZE * sizeof(char));
}

int main(int argc, char* argv[]) {
    if (gethostname(ASIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n", strerror(errno));

    int check = mkdir("users", 0777);

    setupUDPServerSocket();
    setupTCPServerSocket();

    /*==========================
        process standard input
    ==========================*/
    processInput(argc, argv);


    if (listen(tcpServerSocket, maxUsers) == -1)
        /*error*/ exit(1);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(udpServerSocket, &readfds);
        FD_SET(tcpServerSocket, &readfds);
        maxfd = tcpServerSocket;

        /*==========================
        Create child sockets
        ==========================*/
        for (int i = 0 ; i < maxUsers ; i++) {   
            //socket descriptor  
            fd = fdClients[i];   
                 
            //if valid socket descriptor then add to read list  
            if(fd > 0)   
                FD_SET(fd , &readfds);   
                 
            //highest file descriptor number, need it for the select function  
            if(fd > maxfd)   
                maxfd = fd;   
        }   

        /*==========================
        Pick the active file descriptor(s)
        ==========================*/
        maxfd = max(udpServerSocket, maxfd);
        retval = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (retval <= 0)/*error*/exit(1);
        
        for (; retval; retval--) {
            if(FD_ISSET(udpServerSocket, &readfds)) {
                addrlen = sizeof(addr);
                n = recvfrom(udpServerSocket, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
                strncpy(command, buffer, 3);
                if (strcmp(command, "REG") == 0) {
                    printf("MIGO; CHEGAS AQUI??\n"); 
                    printf("buffer:%s\n", buffer); fflush(stdout);
                    checkRegisterInput(buffer);
                    strcat(uid, "/userID_reg.txt");
                    ofstream userCred(uid); 
                    userCred << pdip;
                    userCred << pdport;
                    userCred.close();
                }

                strcpy(buffer, "RRG OK\n");
                
                /*sends ok or not ok to pd*/
                n = sendto(udpServerSocket, buffer, n, 0, (struct sockaddr*) &addr, addrlen);
                if (n == -1)/*error*/exit(1); 
                memset(command, '\0', 4 * sizeof(char));
                memset(buffer, '\0', SIZE * sizeof(char));
            }
            else if(FD_ISSET(tcpServerSocket, &readfds)) {
                addrlen = sizeof(addr);
                if ((newfd = accept(fd, (struct sockaddr*)&addr,&addrlen)) == -1) /*error*/ exit(1);

                //add new socket to array of sockets  
                for (int i = 0; i < maxUsers; i++) {   
                    //if position is empty  
                    if(fdClients[i] == 0 ) {   
                        fdClients[i] = newfd;   
                        break;   
                    }   
                } 

                for (int i = 0; i < maxUsers; i++) {   
                    fd = fdClients[i];   
                    if (FD_ISSET(fd, &readfds)) {   

                        /*
                        //Check if it was for closing , and also read the  
                        //incoming message  
                        if ((n = read(fd, buffer, 1024)) == 0) {   
                            
                            //Close the socket and mark as 0 in list for reuse  
                            close(sd);   //// 
                            fdClients[i] = 0;   
                        }   
                        */
                        //Echo back the message that came in  
                        /*else {   
                            //set the string terminating NULL byte on the end  
                            //of the data read  
                            if (n == -1)  exit(1);
                            buffer[n] = '\0';   
                            send(fd , buffer , strlen(buffer) , 0 );   
                        }   */
                    }   
                } 


            }
        }
    }
}      