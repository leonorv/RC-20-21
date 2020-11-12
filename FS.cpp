#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <sys/stat.h> 
#include <fstream>
#include <dirent.h> 
#include <time.h>
#include <ctype.h>
                      
using namespace std;

#define max(A,B)((A)>=(B)?(A):(B))
#define SIZE 128

extern int errno;

char ASIP[SIZE], ASport[SIZE] = "58030", FSIP[SIZE], FSport[SIZE] = "59030";
int verbose_flag = -1;
const int maxUsers = 5;
int udpClientSocket, tcpServerSocket;
fd_set readfds;
int maxfd, retval;
struct addrinfo hints_udp, hints_tcp, *res_udp, *res_tcp;
int fd, newfd, errcode;
struct sockaddr_in addr_udp, addr_tcp, addr;
socklen_t addrlen_udp, addrlen_tcp, addrlen;
ssize_t n, nread, nw;
int fdClients[maxUsers];
char *ptr, buffer[SIZE], check[3], command[4], password[8], uid[6]="", filename[50], vc[5], op_name[16], tid[5], fop[3], dirName[SIZE];

// void VerboseMode_SET(int test){
//     verbose_flag = test;
// }

void processInput(int argc, char* const argv[]) {
    if (argc%2 != 1) {
        printf("Parse error!\n");
        exit(1);
    }
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-q") == 0) {
            strcpy(FSport, argv[i + 1]);
            continue;
        }
        if (strcmp(argv[i], "-n") == 0) {
            strcpy(ASIP, argv[i + 1]);
            continue;
        }
        if (strcmp(argv[i], "-p") == 0) {
            strcpy(ASport, argv[i + 1]);
            continue;
        }
        else if (strcmp(argv[i], "-v") == 0) {
            //Ativar verbose mode
            // VerboseMode_SET(1);
            continue;
        }
    }
}

void Server_Client_Send(int udpClientSocket, char buffer[SIZE]){
    time_t start, end;
    double elapsed;

    time(&start);  /* start the timer */

    do {
        time(&end);

        elapsed = difftime(end, start);
        int n = sendto(udpClientSocket, buffer, strlen(buffer), 0, res_udp->ai_addr, res_udp->ai_addrlen);
          if (n == -1)
              perror("VLD send");
          else
              elapsed = 5;
    } while(elapsed < 5);  /* run for five seconds */
}

void treatRLS(int fd) {
    struct dirent *userDir;  // Pointer for directory entry 
    char dirName[SIZE];
    strcpy(dirName, "FS_files/");
    strcat(dirName, uid);
    DIR *dr = opendir(dirName); 
    int n_files = 0;
    int size;
  
    if (dr == NULL)  { 
        perror("Could not open current directory"); 
    } 

    string s = "";

    while ((userDir = readdir(dr)) != NULL) {
        if (!strcmp(userDir->d_name, ".") || !strcmp(userDir->d_name, "..") || !strcmp(userDir->d_name, "fd.txt"))
            continue;    /* skip self and parent */
        char filename[SIZE];
        strcpy(filename, dirName);
        strcat(filename, "/");
        strcat(filename, userDir->d_name);
        s += userDir->d_name;
        s += " ";
        struct stat st;
        stat(filename, &st);
        size = st.st_size;
        printf("size of %s: %d\n", filename, size);
        s += to_string(size);
        s += " ";
        n_files++;
    }

    closedir(dr); 
    if (n_files == 0) {
        // RLS EOF
        char buffer[9] = "RLS EOF\n";
        n = 0;
        while (n < 8) {
            n += send(fdClients[fd], &buffer[n], strlen(buffer)-n, 0);
            if (n < 0) {
                perror("error on send fs to user");
            break;
            }
        }
        return;
    }


    printf("s: %s\n", s.c_str());
    // RLS_NFILES_[s]
    int buffersize = 3 + 1 + strlen(to_string(n_files).c_str()) + 1 + strlen(s.c_str()) - 1;
    char buffer[buffersize];
    strcpy(buffer, "RLS ");
    strcat(buffer, to_string(n_files).c_str());
    strcat(buffer, " ");
    strcat(buffer, s.c_str());
    // strcat(buffer, "\0");
    buffer[buffersize] = '\0'; //'\n' sits on last space

    printf("buffer: %s\n", buffer);

    printf("buffersize: %d\n", buffersize);
    n = 0;
    while (n < buffersize) {
        n += send(fdClients[fd], &buffer[n], strlen(buffer)-n, 0);
        if (n < 0) {
            perror("error on send fs to user");
            break;
        }
        printf("n: %ld\n", n);
    }

}

 int checkFilename(char filename[SIZE]) { //returns 1 if filename is ok
     char buffer[SIZE];
    
    sscanf(filename, "%[a-zA-Z0-9._-]", buffer);
    
    if (strlen(buffer) != strlen(filename))
        return 0;
    if (strlen(buffer) < 6 || strlen(buffer) > 25)
        return 0;
    if (!(isalpha(buffer[strlen(buffer)-1]) && 
        isalpha(buffer[strlen(buffer)-2]) && 
        isalpha(buffer[strlen(buffer)-3]) && 
        buffer[strlen(buffer)-4]=='.'))
        return 0;
    // printf("buffer: %s\n", buffer);
    return 1;
 }

 void createFdFile(char uid[SIZE], int fd) {
    char path[SIZE];
    strcpy(path, dirName);
    strcat(path, uid);
    strcat(path, "/fd.txt");
    ofstream userFD;
    userFD.open(path);
    userFD << to_string(fd);
    userFD << '\n';
    userFD.close();
}

int getUserFd(char uid[SIZE]) {
    char path[SIZE];
    strcpy(path, dirName);
    strcat(path, uid);
    strcat(path, "/fd.txt");

    string fd;
    ifstream fdFile;
    fdFile.open(dirName);
    getline(fdFile, fd);
    fdFile.close();
    remove(dirName);

    return atoi(fd.c_str());    
}

int main(int argc, char* argv[]) {
    if (gethostname(ASIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));
    
    if (gethostname(FSIP ,SIZE) == -1)
        fprintf(stderr,"error: %s\n",strerror(errno));

    strcpy(dirName, "FS_files/");
    int error = mkdir(dirName, 0777);

    /*==========================
    Setting up UDP Server Socket
    ==========================*/
    udpClientSocket = socket(AF_INET, SOCK_DGRAM, 0);//UDP socket
        if (udpClientSocket == -1)/*error*/exit(1);
    memset(&hints_udp, 0, sizeof hints_udp);
    hints_udp.ai_family = AF_INET;//IPv4
    hints_udp.ai_socktype = SOCK_DGRAM;//UDP socket
    errcode = getaddrinfo(ASIP, ASport, &hints_udp, &res_udp);
        if (errcode != 0)/*error*/exit(1);

    /*==========================
    Setting up TCP Server Socket
    ==========================*/
    tcpServerSocket = socket(AF_INET, SOCK_STREAM, 0);//TCP socket
        if (tcpServerSocket == -1)/*error*/exit(1);
    memset(&hints_tcp, 0, sizeof hints_tcp);
    hints_tcp.ai_family = AF_INET;//IPv4
    hints_tcp.ai_socktype = SOCK_STREAM;//TCP socket
    hints_tcp.ai_flags = AI_PASSIVE;
    errcode = getaddrinfo(NULL, FSport, &hints_tcp, &res_tcp);
        if (errcode != 0)/*error*/exit(1);
    if (bind(tcpServerSocket, res_tcp->ai_addr, res_tcp->ai_addrlen) < 0) exit(1);

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

    while (1){
        FD_ZERO(&readfds);
        FD_SET(udpClientSocket, &readfds);
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
        maxfd = max(maxfd, udpClientSocket);
        retval = select(maxfd + 1, &readfds, NULL, NULL, NULL);
            if (retval <= 0)/*error*/exit(1);
        
        for (; retval; retval--) {
            if (FD_ISSET(udpClientSocket, &readfds)) {
                char fname[25], command[4], uid[6], tid[5], fop[2];
                /*RECEIVING AS AS A CLIENT*/
                addrlen_udp = sizeof(addr_udp);
                n = recvfrom(udpClientSocket, buffer, SIZE, 0, (struct sockaddr*) &addr_udp, &addrlen_udp);
                if (n == -1)/*error*/exit(1);
                buffer[n] = '\0';

                printf("recebido do as: %s\n", buffer);

                /* CNF UID TID Fop [Fname] */
                sscanf(buffer, "%s %s %s %s %s\n", command, uid, tid, fop, fname);

                if (strcmp(fop, "L") == 0 || strcmp(fop, "X") == 0) {
                    fname[0] = '\0';
                }

                printf("command: %s\nuid: %s\nfop: %s\n", command, uid, fop);

                if (strcmp(command, "CNF") == 0) {


                    if (strcmp(fop, "U") == 0) {
                        strcpy(buffer,"RUP");
                    }
                    else if (strcmp(fop, "D") == 0) {
                        strcpy(buffer,"RDL");
                    }
                    else if (strcmp(fop, "R") == 0) {
                        strcpy(buffer,"RRT");
                    }
                    else if (strcmp(fop, "L") == 0) {
                        int fd = getUserFd(uid);
                        printf("vai fazer treat rls\n");
                        treatRLS(fd); //
                    }
                    else if (strcmp(fop, "X") == 0) {
                        strcpy(buffer,"RRM");
                    }
                    else if (strcmp(fop, "E") == 0) {
                        strcpy(buffer,"E");
                    }
                    else {
                        printf("ERR\n");
                    }  
                        
                    printf("operation validated\n");
                    //closing connection with user
                    close(fdClients[fd]);
                    fdClients[fd] = 0;

                    memset(buffer, '\0', SIZE * sizeof(char));
                }
                else {
                    perror("invalid command from AS");
                    continue;
                }
            }
            else if (FD_ISSET(tcpServerSocket, &readfds)) {
                addrlen_tcp = sizeof(addr_tcp);
                if ((newfd = accept(tcpServerSocket, (struct sockaddr*)&addr_tcp, &addrlen_tcp)) == -1) { perror("accept tcp server socket"); exit(1); }

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
                   

                    // char *token;
                    // char bufferTemp[SIZE];
                    memset(command, '\0', strlen(command)*sizeof(char));
                    n = read(fd, command, 4);
                    command[4] = '\0';

                    memset(uid, '\0', strlen(uid)*sizeof(char));
                    n = read(fd, uid, 6); // XXXXX_ reads UID
                    uid[5] = '\0'; 

                    char path[SIZE];
                    strcpy(path, dirName);
                    strcat(path, uid);
                    printf("command: %s, uid: %s\n", command, uid); fflush(stdout);

                    int error = mkdir(path, 0777);


                    if (n == 0) {
                        getpeername(fd, (struct sockaddr*)&addr, (socklen_t*)&addrlen);

                        //Close the socket and mark as 0 in list for reuse
                        close(fd);
                        fdClients[i] = 0;
                        // /* error */ exit(1);
                    }
                    else if (n == -1) { perror("fdclients read"); exit(1); }
                    else {


                        // printf("entrou no ")
                        

                        if (strcmp(command, "LST ") == 0) {

                            //already read command and uid                            
                            n = read(fd, tid, 5); // XXXX\n reads TID
                            if (tid[4] != '\n') {
                                // send RLS ERR to user
                                n = 0;
                                memset(buffer, '\0', strlen(buffer)*sizeof(char));
                                strcpy(buffer, "RLS ERR\n");
                                while (n != strlen(buffer)) {
                                    n += send(fd, &buffer[n], strlen(buffer)-n, 0);
                                }
                            }
                            tid[4] = '\0';

                            // send to AS ->> VLD UID TID
                            memset(buffer, '\0', strlen(buffer)*sizeof(char));
                            sprintf(buffer, "VLD %s %s\n", uid, tid);

                            printf("send to AS: %s", buffer);

                            Server_Client_Send(udpClientSocket, buffer);

                            createFdFile(uid, fd);
                            
                            continue;
                        }
                        else if (strcmp(command, "RTV ") == 0) {
                        }
                        else if (strcmp(command, "UPL ") == 0) {
                        }
                        else if (strcmp(command, "DEL ") == 0) {

                        }
                        else if (strcmp(command, "REM ") == 0) {

                        }
                        else {
                            printf("ERR\n");
                        }  
                        memset(buffer, '\0', SIZE * sizeof(char));    
                    }
                }
            }
        }
    }
}      