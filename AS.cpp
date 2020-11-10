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
#include <string> 
#include <sys/stat.h> 

using namespace std;

#define max2(A,B)((A)>=(B)?(A):(B))
#define max(A,B,C)(A < B ? A : B) < C ? (A < B ? A : B) : C;
#define SIZE 128

#define ERR -1
#define EFOP -2
#define EUSER -3
#define ELOG -4
#define EPD -5

extern int errno;

char ASport[SIZE] = "58030", ASIP[SIZE], PDport[SIZE] = "57030", PDIP[SIZE] , FSport[SIZE] = "59030", FSIP[SIZE];

const int maxUsers = 5;
FILE *fptr;
int udpServerSocket, tcpServerSocket, udpClientSocket, udpServerSocket_FS;
fd_set readfds;
int maxfd, retval;
int fd, newfd, errcode;
struct addrinfo hints_uc, hints_us, hints_ts, *res_uc, *res_ts, *res_us;
struct sockaddr_in addr_uc, addr_us, addr_ts, addr_sfs, addr;
socklen_t addrlen_uc, addrlen_us, addrlen_ts, addrlen_sfs, addrlen;
ssize_t n, nread, nw;
char buffer[SIZE], command[SIZE], password[SIZE], uid[SIZE], *ptr, rid[SIZE], fop;
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

int setupUDPClientSocket(char PDIP[SIZE], char PDport[SIZE]) {
    udpClientSocket = socket(AF_INET, SOCK_DGRAM, 0); //UDP socket
        if (udpClientSocket == -1) /*error*/return 0;
    memset(&hints_uc, 0, sizeof hints_uc);
    hints_uc.ai_family = AF_INET; //IPv4
    hints_uc.ai_socktype = SOCK_DGRAM; //UDP socket
    errcode = getaddrinfo(PDIP, PDport, &hints_uc, &res_uc);
    memset(PDIP, '\0', SIZE * sizeof(char));
    memset(PDport, '\0', SIZE * sizeof(char));
        if (errcode != 0) /*error*/ return 0;
    FD_SET(udpClientSocket, &readfds);
    maxfd = max2(maxfd, udpClientSocket);   
    return 1;
}

void setupUDPServerSocket() {
    udpServerSocket = socket(AF_INET, SOCK_DGRAM, 0); //UDP socket
        if (udpServerSocket == -1)  { perror("udp server socket"); exit(1); }
    memset(&hints_us, 0, sizeof hints_us);
    hints_us.ai_family = AF_INET; //IPv4
    hints_us.ai_socktype = SOCK_DGRAM; //UDP socket
    hints_us.ai_flags = AI_PASSIVE;
    errcode = getaddrinfo(NULL, ASport, &hints_us, &res_us);
        if (errcode != 0) { perror("USS get addr info"); exit(1); }
    if (bind(udpServerSocket, res_us->ai_addr, res_us->ai_addrlen) < 0)  { perror("bind udp server socket"); exit(1); }
}

void setupTCPServerSocket() {
    tcpServerSocket = socket(AF_INET, SOCK_STREAM, 0);//TCP socket
        if (tcpServerSocket == -1) { perror("tcp server socket"); exit(1); }
    memset(&hints_ts, 0, sizeof hints_ts);
    hints_ts.ai_family = AF_INET;//IPv4
    hints_ts.ai_socktype = SOCK_STREAM;//TCP socket
    hints_ts.ai_flags = AI_PASSIVE;
    errcode = getaddrinfo(NULL, ASport, &hints_ts, &res_ts);
        if (errcode != 0) { perror("TSS get addr info"); exit(1); }
    if (bind(tcpServerSocket, res_ts->ai_addr, res_ts->ai_addrlen) < 0) { perror("bind tcp server socket"); exit(1); }
}

int checkRegisterInput(char buffer[SIZE]) {    
    char filename[SIZE], password[SIZE], uid[SIZE], pdip[SIZE], pdport[SIZE], driName[SIZE];

    sscanf(buffer, "REG %[0-9] %[0-9a-zA-Z] %[0-9.] %[0-9]\n", uid, password, pdip, pdport);
    if (strlen(uid) == 1 || strlen(uid) != 5) return 0;
    if (strlen(password) == 1 || strlen(password) != 8) return 0;
    if (strlen(pdip) == 1) return 0;
    if (strlen(pdport) == 1 || strlen(pdport) != 5) return 0;

    strcpy(dirName, "users/");
    strcat(dirName, uid);
    int error = mkdir(dirName, 0777);

    strcpy(filename, dirName);
    strcat(filename, "/password.txt");

    if (error == -1) { 
        /* If User has already been registered */
        string inFilePass;
        ifstream inFile;
        inFile.open(filename);
        getline(inFile, inFilePass);
        /* Check password */
        if (strcmp(inFilePass.c_str(), password) != 0) {
            /*password is incorrect*/ 
            return 0;
        }
        /* if user already exists, we will have to reset the pd info (pdport and pdip)*/
        memset(filename, '\0', SIZE * sizeof(char));
        strcpy(filename, dirName);
        strcat(filename, "/reg.txt");
        remove(filename);
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

    printf("PD: new user, UID=%s\n", uid);

    return 1;
}

int checkUnregisterInput(char buffer[SIZE]) {
    char filename[SIZE], uid[SIZE], password[SIZE], dirName[SIZE];

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
        memset(filename, '\0', SIZE * sizeof(char));
        strcpy(filename, dirName);
        strcat(filename, "/login.txt");   
        remove(filename); /* remove login.txt */
        rmdir(dirName);
        memset(filename, '\0', SIZE * sizeof(char));
        strcpy(filename, dirName);
        strcat(filename, "/fd.txt");   
        remove(filename); /* remove fd.txt */
        rmdir(dirName);
        // more files
        return 1;    
    }
    else { /* user directory does not exist */
        rmdir(dirName);
        return 0;
    }
}

int checkLoginInput(char buffer[SIZE]) {
    char filename[SIZE], uid[SIZE], password[SIZE], dirName[SIZE];

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

        printf("User: login ok, UID=%s\n", uid);

        return 1;    
    }
    else { /* user directory does not exist */
        rmdir(dirName);
        return 0;
    }
}

int treatRequestInput(char buffer[SIZE], int fdIndex) {
    char filename[SIZE], fname[SIZE], uid[SIZE], rid[SIZE], fop[2], dirName[SIZE], toSend[SIZE], op_name[SIZE];
    ofstream userTid, userFD; 
    sscanf(buffer, "REQ %[0-9] %[0-9] %s %s\n", uid, rid, fop, fname);

    // CONFIRMAR SE UID CORRECTO - EUSER

    if (strlen(rid) == 1) {
        /* if rid is empty, the user isn't logged in*/
        return ELOG;
    }

    strcpy(dirName, "users/");
    strcat(dirName, uid);

    int error = mkdir(dirName, 0777);

    if (error != -1)  {
        // perror("REQ - user does not exist");
        /* if there is no error in mkdir, the user directory does not exist*/
        remove(dirName);
        return EUSER;
    }

    char logFilename[SIZE];
    strcpy(logFilename, dirName);
    strcat(logFilename, "/login.txt");
    ifstream ifile(logFilename);
    /* if thre is no log.txt, the user isn't logged in*/
    if (!ifile) return ELOG;
    memset(logFilename, '\0', SIZE * sizeof(char));

   //CHECK FOP  L, R, U, D or X
   if (strcmp(fop, "U") != 0 && strcmp(fop, "D") != 0 && strcmp(fop, "L") != 0 && strcmp(fop, "R") != 0 && strcmp(fop, "X") != 0)
        return EFOP;

    strcpy(filename, dirName);
    strcat(filename, "/reg.txt");
    string inFileIP;
    string inFilePort;
    ifstream inFile;
    inFile.open(filename);
    getline(inFile, inFileIP);
    getline(inFile, inFilePort);
    inFile.close();

    strcpy(pdip, inFileIP.c_str());
    strcpy(pdport, inFilePort.c_str());
    if (setupUDPClientSocket(pdip, pdport) == 0)
        return EPD;

    int temp = rand() % 8000 + 1000;
    string s_vc = to_string(temp);
    char vc[6];
    strcpy(vc, s_vc.c_str());
    
    strcpy(toSend, "VLC ");
    strcat(toSend, uid);
    strcat(toSend, " ");
    strcat(toSend, vc);
    strcat(toSend, " ");
    strcat(toSend, fop);
    if (fname) {
        strcat(toSend, " ");
        strcat(toSend, fname);
    }
    strcat(toSend, "\n");

    int n = sendto(udpClientSocket, toSend, strlen(toSend), 0, res_uc->ai_addr, res_uc->ai_addrlen);
    if (n == -1)
        perror("VLC send");

    if (strcmp(fop, "U") == 0) {
        strcpy(op_name, "upload");
    } 
    else if (strcmp(fop, "R") == 0) {
        strcpy(op_name, "retrieve");
    } 
    else if (strcmp(fop, "D") == 0) {
        strcpy(op_name, "delete");
    } 
    else if (strcmp(fop, "L") == 0) {
        strcpy(op_name, "list");
    }  
    else if (strcmp(fop, "X") == 0) {
        strcpy(op_name, "remove");
    }

    /* create file for fop, filename and later tid */
    char tempfileName_2[SIZE];
    strcpy(tempfileName_2, dirName);
    strcat(tempfileName_2, "/tid.txt");
    userTid.open(tempfileName_2);
    userTid << fop;
    userTid << '\n';
    userTid << fname;
    userTid << '\n';
    userTid.close();

    /* create file with user fd */
    char tempfileName_1[SIZE];
    strcpy(tempfileName_1, dirName);
    strcat(tempfileName_1, "/fd.txt");
    printf("tempfilename: %s, fdindex: %s\n", tempfileName_1, to_string(fdIndex).c_str());
    userFD.open(tempfileName_1);
    userFD << to_string(fdIndex);
    userFD << '\n';
    userFD.close();

    printf("User: %s req, UID=%s, file: %s, RID=%s, VC=%s\n", op_name, uid, fname, rid, vc);
    return 1;
}

void treatRVCInput(char buffer[SIZE]) { //as received rvc from pd and he's going to send ok/nok to user
    char uid[SIZE], msg[SIZE], status[SIZE], dirName[SIZE] = "users/";
    int fdIndex;

    sscanf(buffer, "RVC %[0-9] %s\n", uid, status);

    strcat(dirName, uid);
    strcat(dirName, "/fd.txt");

    string index;
    ifstream indexFile;
    indexFile.open(dirName);
    getline(indexFile, index);
    indexFile.close();
    remove(dirName);
    // index
    fdIndex = atoi(index.c_str());

    if (strcmp(status, "OK\n") != 0) 
        strcpy(msg, "RRQ OK\n");
        
    else 
        strcpy(msg, "RRQ EPD\n");

    printf("index: %s, msg: %s",index.c_str(), msg);

    if (send(fdClients[fdIndex], msg, strlen(msg), 0) != strlen(msg))
        perror("RRQ send");
}

int checkAuthenticationInput(char buffer[SIZE]) {
    char uid[SIZE], rid[SIZE], vc[SIZE], dirName[SIZE];

    sscanf(buffer, "AUT %[0-9] %[0-9] %[0-9]\n", uid, rid, vc);

    return atoi(uid);
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
        /*error*/ perror("listen tcpserversocket");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(udpServerSocket, &readfds);
        FD_SET(tcpServerSocket, &readfds);
        FD_SET(udpClientSocket, &readfds);
        maxfd = tcpServerSocket;

        /*==========================
        Create child sockets
        ==========================*/
        for (int i = 0 ; i < maxUsers ; i++) {   
            //socket descriptor  
            fd = fdClients[i];   
                 
            //if valid socket descriptor then add to read list  
            if (fd > 0)   
                FD_SET(fd , &readfds);   
                 
            //highest file descriptor number, need it for the select function  
            if (fd > maxfd)   
                maxfd = fd;   
        }   

        /*==========================
        Pick the active file descriptor(s)
        ==========================*/
        maxfd = max2(udpServerSocket, maxfd);
        maxfd = max2(maxfd, udpClientSocket);
        retval = select(maxfd + 1, &readfds, NULL, NULL, NULL);
            if (retval <= 0) { perror("select retval"); exit(1); }
        
        
        for (; retval; retval--) {
            memset(command, '\0', SIZE * sizeof(char));
            memset(buffer, '\0', SIZE * sizeof(char));
            //-------------------------------------------------------------------------------------
            if (FD_ISSET(udpClientSocket, &readfds)) {
                /*============================
                        PD -> RVC status
                ============================*/
                addrlen_uc = sizeof(addr_uc);
                n = recvfrom(udpClientSocket, buffer, SIZE, 0, (struct sockaddr*) &addr_uc, &addrlen_uc);
                buffer[n] = '\0';

                /* get command code */
                strncpy(command, buffer, 3);

                if (strcmp(command, "RVC") == 0) {
                    // printf("RVC-test\n");
                    treatRVCInput(buffer);

                }
                memset(command, '\0', SIZE * sizeof(char));
                memset(buffer, '\0', SIZE * sizeof(char));
            }
            else if (FD_ISSET(udpServerSocket, &readfds)) {
                /*===========================================
                        PD -> REG UID pass PDIP PDport
                        PD -> UNR UID pass
                ===========================================*/
                addrlen_us = sizeof(addr_us);
                n = recvfrom(udpServerSocket, buffer, SIZE, 0, (struct sockaddr*) &addr_us, &addrlen_us);
                if (n == -1)/*error*/perror("recvfrom udpServerSocket");
                buffer[n] = '\0';

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
                        printf("run ok\n");
                        strcpy(buffer, "RUN OK\n");
                    }
                    else
                        strcpy(buffer, "RUN NOK\n");
                }
                
                /* sends ok or not ok to pd */
                n = sendto(udpServerSocket, buffer, strlen(buffer), 0, (struct sockaddr*) &addr_us, addrlen_us);
                if (n == -1) { perror("sendto udp server socket"); continue; }
            }
            else if (FD_ISSET(tcpServerSocket, &readfds)) {
                /*===========================================
                        USER -> LOG UID pass
                        USER -> REQ UID RID Fop [Fname]
                        USER -> AUT UID RID VC
                ===========================================*/
                addrlen_ts = sizeof(addr_ts);
                if ((newfd = accept(tcpServerSocket, (struct sockaddr*)&addr_ts, &addrlen_ts)) == -1) { perror("accept tcp server socket"); exit(1); }

                //add new socket to array of sockets  
                for (int i = 0; i < maxUsers; i++) {   
                    //if position is empty  
                    if (fdClients[i] == 0 ) {  
                        fdClients[i] = newfd;   
                        break;   
                    }   
                } 
            }
            for (int i = 0; i < maxUsers; i++) {   
                fd = fdClients[i];   
                if (FD_ISSET(fd, &readfds)) {  
                    char readBuffer[SIZE];
                    
                    int n = read(fd, readBuffer, SIZE);
                    while (n != 0) {
                        strcat(buffer, readBuffer);
                        memset(readBuffer, '\0', strlen(readBuffer) * sizeof(char));
                        n = read(fd, readBuffer, SIZE);
                    }

                    if (n == 0) {
                        getpeername(fd, (struct sockaddr*)&addr, (socklen_t*)&addrlen);

                        //Close the socket and mark as 0 in list for reuse
                        close(fd);
                        fdClients[i] = 0;
                    }
                    else if (n == -1) { perror("fdclients read"); exit(1); }
                    else {
                            /* get command code */
                        strncpy(command, buffer, 3);

                        if (strcmp(command, "LOG") == 0) {
                            if (checkLoginInput(buffer)) {
                                strcpy(buffer, "RLO OK\n");
                            }
                            else 
                                strcpy(buffer, "RLO NOK\n");

                            /* error in sending */

                            if (send(fd, buffer, strlen(buffer), 0) != strlen(buffer))
                                perror("RLO send");
                            fflush(stdout);
                        }
                        else if (strcmp(command, "REQ") == 0) {
                            
                            int reqResult = treatRequestInput(buffer, i);
                            //send error to user
                            strcpy(buffer, "RRQ ");
                            if (reqResult == ERR)
                                strcat(buffer, "ERR\n");
                            else if (reqResult == ELOG)
                                strcat(buffer, "ELOG\n");                                    
                            else if (reqResult == EUSER)
                                strcat(buffer, "EUSER\n");
                            else if (reqResult == EFOP)
                                strcat(buffer, "EFOP\n");
                            else if (reqResult == EPD)
                                strcat(buffer, "EPD\n");
                            else {
                                //strcat(buffer, "OK\n");
                                continue; //wait for pd confirmation
                            }
                            
                            if (send(fd, buffer, strlen(buffer), 0) != strlen(buffer))
                                perror("RRQ send"); 
                            
                        }
                        else if (strcmp(command, "AUT") == 0) {
                            int res = checkAuthenticationInput(buffer);
                            if (res != 0) {

                                int tid_temp = rand() % 8000 + 1000;

                                strcpy(dirName, "users/");
                                strcat(dirName, to_string(res).c_str());
                                strcat(dirName, "/tid.txt");
                                ofstream userTid; 
                                userTid.open(dirName, std::ios_base::app);
                                userTid << tid_temp;
                                userTid.close();
                                
                                string s_tid = to_string(tid_temp);
                                char tid[6];
                                strcpy(tid, s_tid.c_str());

                                string inFileFOP;
                                string inFileFNAME;
                                ifstream inFile;
                                inFile.open(dirName);
                                getline(inFile, inFileFOP);
                                getline(inFile, inFileFNAME);
                                inFile.close();
                                
                                strcpy(buffer, "RAU");
                                strcat(buffer, " ");
                                strcat(buffer, tid);
                                strcat(buffer, "\n");

                                //aceder ao file tid para recuperar fop e fname

                                printf("User: UID=%d, %s, %s, TID=%s\n", res, inFileFOP.c_str(), inFileFNAME.c_str(), tid);
                            }
                            else 
                                strcpy(buffer, "RAU 0\n");
                            /* error in sending */
                            if (send(fd, buffer, strlen(buffer), 0) != strlen(buffer))
                                perror("RAU send");
                        }
                    }
                    memset(buffer, '\0', strlen(buffer) * sizeof(char));
                }   
            } 
        }
    }
}