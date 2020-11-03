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

char ASport[SIZE] = "58030", ASIP[SIZE], PDport[SIZE], PDIP[SIZE] , FSport[SIZE], FSIP[SIZE];

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
char buffer[SIZE], command[SIZE], password[SIZE], uid[SIZE], *ptr;
int connectedUsers = 0;
int fdClients[maxUsers];
char dirName[SIZE];
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
    memset(&hints_ts, 0, sizeof hints_ts);
    hints_ts.ai_family = AF_INET;//IPv4
    hints_ts.ai_socktype = SOCK_STREAM;//TCP socket
    hints_ts.ai_flags = AI_PASSIVE;
    errcode = getaddrinfo(NULL, ASport, &hints_ts, &res_ts);
    if (errcode != 0)/*error*/exit(1);
    if (bind(tcpServerSocket, res_ts->ai_addr, res_ts->ai_addrlen) < 0) exit(1);
}


int checkRegisterInput(char buffer[SIZE]) {    
    char filename[SIZE];

    sscanf(buffer, "REG %[0-9] %[0-9a-zA-Z] %[0-9.] %[0-9]\n", uid, password, pdip, pdport);
    if (uid == NULL || strlen(uid) != 5) return 0;
    if (password == NULL || strlen(password) != 8) return 0;
    if (pdip == NULL) return 0;
    if (pdport == NULL || strlen(pdport) != 5) return 0;

    strcpy(dirName, "users/");
    strcat(dirName, uid);
    int error = mkdir(dirName, 0777);

    strcpy(filename, dirName);
    strcat(filename, "/password.txt");

    if (error == -1) { 
        /* If User has already been registered */
        char passBuffer[9];
        FILE *f = fopen(filename, "r");
        fgets(passBuffer, strlen(passBuffer), f);
        /* Check password */
        if (strcmp(passBuffer, password) != 0) /*password is incorrect*/ return 0;
        fclose(f);
        memset(passBuffer, '\0', 9 * sizeof(char));

    }
    else {
        /* If User has not been registered */
        /* Create user directory */
        ofstream userPass; 
        userPass.open(filename);
        userPass << password;
        userPass.close();
    }

    memset(filename, '\0', SIZE * sizeof(char));
    strcpy(filename, dirName);
    strcat(filename, "/reg.txt");

    ofstream userCred; 
    userCred.open(filename, ofstream::out | ofstream::trunc);
    userCred.close();
    userCred.open(filename);
    userCred << pdip;
    userCred << '\n';
    userCred << pdport;
    userCred.close();

    memset(dirName, '\0', SIZE * sizeof(char));
    memset(filename, '\0', SIZE * sizeof(char));
    memset(uid, '\0', SIZE * sizeof(char));
    memset(password, '\0', SIZE * sizeof(char));
    memset(pdip, '\0', SIZE * sizeof(char));
    memset(pdport, '\0', SIZE * sizeof(char));

    return 1;
}

int checkUnregisterInput(char buffer[SIZE]) {
    char filename[SIZE];

    sscanf(buffer, "UNR %[0-9] %[0-9a-zA-Z]\n", uid, password);

    strcpy(dirName, "users/");
    strcat(dirName, uid);

    int error = mkdir(dirName, 0777);

    if (error == -1) {  /* If User had already been registered */

        strcpy(filename, dirName);
        strcat(filename, "/password.txt");
        string inFilePass;
        ifstream inFile;
        inFile.open(filename);
        getline(inFile, inFilePass);
        if (strcmp(inFilePass.c_str(), password) != 0) {
            /*password is incorrect*/ 
            return 0;
        }
        inFile.close();

        /* delete directory */
        remove(filename); /* remove password.txt */
        memset(filename, '\0', SIZE * sizeof(char));
        strcpy(filename, dirName);
        strcat(filename, "/reg.txt");   
        remove(filename); /* remove reg.txt */
        rmdir(dirName);
        return 1;    
    }
    else { /* user directory does not exist */
        rmdir(dirName);
        return 0;
    }
}

int checkLoginInput(char buffer[SIZE]) {
    char filename[SIZE];

    sscanf(buffer, "LOG %[0-9] %[0-9a-zA-Z]\n", uid, password);

    strcpy(dirName, "users/");
    strcat(dirName, uid);

    int error = mkdir(dirName, 0777);

    if (error == -1) {  /* If User had already been registered */

        strcpy(filename, dirName);
        strcat(filename, "/password.txt");
        string inFilePass;
        ifstream inFile;
        inFile.open(filename);
        getline(inFile, inFilePass);
        if (strcmp(inFilePass.c_str(), password) != 0) {
            /*password is incorrect*/ 
            return 0;
        }
        inFile.close();

        /* Create temp login file */
        char tempfileName[SIZE];
        strcpy(tempfileName, dirName);
        strcat(tempfileName, "/login.txt");
        ofstream userLogReg; 
        userLogReg.open(tempfileName);
        userLogReg.close();

        return 1;    
    }
    else { /* user directory does not exist */
        rmdir(dirName);
        return 0;
    }


}



int main(int argc, char* argv[]) {
    if (gethostname(ASIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n", strerror(errno));

    int check = mkdir("users", 0777);

    setupUDPServerSocket();
    setupTCPServerSocket();

    /* Initialize TCP babies fds */
    for (int i = 0; i < maxUsers; i++) {   
        fdClients[i] = 0;
    } 
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

                fflush(stdout);

                /* get command code */
                strncpy(command, buffer, 3);

                if (strcmp(command, "REG") == 0) {
                    if (checkRegisterInput(buffer)) {
                        strcpy(buffer, "RRG OK\n");
                    }
                    else 
                        strcpy(buffer, "RRG NOK\n");
                }
                else if (strcmp(command, "UNR") == 0) {
                    if (checkUnregisterInput(buffer)) {
                        strcpy(buffer, "RUN OK\n");
                    }
                    else
                        strcpy(buffer, "RUN NOK\n");
                }
                
                /*sends ok or not ok to pd*/
                n = sendto(udpServerSocket, buffer, n, 0, (struct sockaddr*) &addr, addrlen);
                if (n == -1)/*error*/exit(1); 
                memset(command, '\0', SIZE * sizeof(char));
                memset(buffer, '\0', SIZE * sizeof(char));
            }
            else if(FD_ISSET(tcpServerSocket, &readfds)) {
                addrlen = sizeof(addr);
                if ((newfd = accept(tcpServerSocket, (struct sockaddr*)&addr, &addrlen)) == -1) /*error*/ exit(1);

                //add new socket to array of sockets  
                for (int i = 0; i < maxUsers; i++) {   
                    //if position is empty  
                    if(fdClients[i] == 0 ) {  
                        fdClients[i] = newfd;   
                        break;   
                    }   
                } 
            }

                for (int i = 0; i < maxUsers; i++) {   
                    fd = fdClients[i];   
                    if (FD_ISSET(fd, &readfds)) {   
                        int n = read(fd, buffer, SIZE);
                        if (n == 0) {
                            
                            getpeername(fd , (struct sockaddr*)&addr , (socklen_t*)&addrlen);
                            printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(addr.sin_addr) , ntohs(addr.sin_port));

                            //Close the socket and mark as 0 in list for reuse
                            close(fd);
                            fdClients[i] = 0;
                            /* error */ exit(1);
                        }
                        else if (n == -1) 
                            /* error */ exit(1);
                        else {
                             /* get command code */
                        strncpy(command, buffer, 3);

                            if (strcmp(command, "LOG") == 0) {
                                if (checkLoginInput(buffer))
                                    strcpy(buffer, "RLO OK\n");
                                else 
                                    strcpy(buffer, "RLO NOK\n");

                                /* error in sending */
                                if (send(fd, buffer, strlen(buffer), 0) != strlen(buffer))
                                    perror("RLO send");
                        
                            }
                        }  

                       

                    }   
                } 
            }
        }
    }