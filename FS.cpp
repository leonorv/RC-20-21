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
char *ptr, buffer[SIZE], check[3], command[4], password[8], uid[6]="", vc[5], op_name[16], tid[5], fop[3], dirName[SIZE];

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

void treatRLS(int fd, char uid[6]) {
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
                perror("error on send fs to user 2");
            break;
            }
        }
        return;
    }


    printf("s: %s\n", s.c_str());
    // RLS_NFILES_[s]
    int buffersize = 3 + 1 + strlen(to_string(n_files).c_str()) + 1 + strlen(s.c_str());
    char buffer[buffersize];
    strcpy(buffer, "RLS ");
    strcat(buffer, to_string(n_files).c_str());
    strcat(buffer, " ");
    strcat(buffer, s.c_str());
    buffer[buffersize] = '\0'; //'\n' sits on last space

    n = 0;
    while (n < buffersize) {
        n += send(fdClients[fd], &buffer[n], strlen(buffer)-n, 0);
        if (n < 0) {
            perror("error on send fs to user 3");
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

 void treatRRT(int fd, char uid[6]) {  
    struct dirent *userDir;  // Pointer for directory entry 
    char path[SIZE];
    DIR *dr; 
    char filename[SIZE];

    char path_to_filename[SIZE];
    strcpy(path_to_filename, "FS_files/");
    strcat(path_to_filename, uid);
    strcat(path_to_filename, "/filename.txt");

    printf("path to filename: %s\n", path_to_filename);
    
    string fname;
    ifstream fstream;
    fstream.open(path_to_filename);
    getline(fstream, fname);
    fstream.close();
    remove(path_to_filename);

    strcpy(filename, fname.c_str());

    printf("treat RRT: uid: %s, filename: %s\n", uid, filename);

    strcpy(path, "FS_files/");
    strcat(path, uid);
    strcat(path, "\0");

    dr = opendir(path); 
    int n_files = 0;
    if (dr != NULL) {
        while ((userDir = readdir(dr)) != NULL) {
            if (!strcmp(userDir->d_name, ".") || !strcmp(userDir->d_name, "..") || !strcmp(userDir->d_name, "fd.txt"))
                continue;    /* skip self and parent and fd*/
            n_files++;
        }
    }

    printf("nfiles: %d\n", n_files);
    if (n_files == 0) {
        char bufferError[SIZE] = "RRT NOK\n";
        int n = 0;        
        do {
            n += write(fdClients[fd], &bufferError[n], strlen(bufferError) - n);
        } while (strlen(bufferError) > n);
        return;
    }
    
    strcat(path, "/");
    strcat(path, filename);

    printf("filename rrt: %s\n", filename);
    
    struct stat st;
    stat(path, &st);
    int fsize = st.st_size;

    if (access(path, F_OK) == -1 || strcmp(filename, "fd.txt") == 0) {
        /* file not available */
        char bufferError[SIZE] = "RRT EOF\n";
        int n = 0;        
        do {
            n += write(fdClients[fd], &bufferError[n], strlen(bufferError) - n);
        } while (strlen(bufferError) > n);
        return;
    }


    FILE *f;
    f = fopen(path, "rb");

    int buffersize = 3 + 1 + strlen(to_string(fsize).c_str());
    char firstBuffer[buffersize];

    strcpy(firstBuffer, "RRT ");
    strcat(firstBuffer, to_string(fsize).c_str());
    strcat(firstBuffer, " ");
    firstBuffer[buffersize] = '\0'; //'\n' sits on last space

    printf("firstbuffer: %s\n", firstBuffer);


    int sum = 0;
    n = 0;
    while (sum < buffersize) {
        n = send(fdClients[fd], &firstBuffer[sum], strlen(firstBuffer)-sum, 0);
        sum += n;
        if (n < 0) {
            perror("error on send fs to user 1");
            break;
        }
    }

    //just like user upload
    sum = 0;
    n = 0;
    do {
        char bufferTemp[1];
        memset(bufferTemp, '\0', sizeof(char));
        fread(bufferTemp, 1, 1, f);
        //printf("buffertemp: %s\n", bufferTemp);
        n = write(fdClients[fd], bufferTemp, 1);
        sum += n;
        if (n < 0) {
            perror("write fs to user");
            break;
        }
    } while (fsize > sum);
 } 

 void createFdFile(char uid[SIZE], int i) {
    char path[SIZE];
    strcpy(path, dirName);
    strcat(path, uid);
    strcat(path, "/fd.txt");
    ofstream userFD;
    userFD.open(path);
    userFD << to_string(i);
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

                    printf("recebeu cnf, fop: %s\n", fop);

                    int index = getUserFd(uid);

                    if (strcmp(fop, "U") == 0) {
                    }
                    else if (strcmp(fop, "D") == 0) {
                    }
                    else if (strcmp(fop, "R") == 0) {
                        printf("vai fazer treat RRT\n");
                        treatRRT(index, uid); ///
                    }
                    else if (strcmp(fop, "L") == 0) {
                        treatRLS(index, uid); ///
                    }
                    else if (strcmp(fop, "X") == 0) {
                    }
                    else if (strcmp(fop, "E") == 0) {
                        // strcpy(buffer,"E"); // enviar par ao user RRT INV ou 
                    }
                    else {
                        printf("ERR\n");
                    }  
                        
                    printf("operation validated\n");
                    //closing connection with user
                    close(fdClients[index]);
                    fdClients[index] = 0;

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
                    //char buff[SIZE];
                    //int n = 0;
                    //n += read(fd, buff, SIZE); 

                   // if (n == 0) {
                   //     //getpeername(fd, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
                   //     close(fd);
                   //     fdClients[i] = 0;
                   //     continue;
                   // }
                    //else if (n == -1) { perror("fdclients read"); exit(1); }
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

                            createFdFile(uid, i);
                            
                            //continue;
                            close(fd);
                            fdClients[i] = 0;
                        }
                        else if (strcmp(command, "RTV ") == 0) {
                            //RTV UID TID Fname
                            //already read command and uid    
                            // printf("RTV command: %s. uid: %s\n");                      
                            n = read(fd, tid, 5); // XXXX\n reads TID
                            if (tid[4] != '\n') {
                                // send RTV ERR to user
                                n = 0;
                                memset(buffer, '\0', strlen(buffer)*sizeof(char));
                                strcpy(buffer, "RTV ERR\n");
                                while (n != strlen(buffer)) {
                                    n += send(fd, &buffer[n], strlen(buffer)-n, 0);
                                }
                            }
                            tid[4] = '\0';

                            char filename[SIZE];
                            int sum = 0;
                            n = 0;
                            do {
                                n = read(fd, &filename[sum], SIZE-sum);
                                sum += n;
                            } while (n > 0);
                            filename[sum] = '\0';
                            printf("filename: %s\n", filename);
                            
                            char path[SIZE];
                            strcpy(path, "FS_files/");
                            strcat(path, uid);
                            strcat(path, "/filename.txt");
                            printf("path: %s\n", path);

                            ofstream fname(path);         
                            fname << filename;
                            fname << '\n';
                            fname.close();
    
                                                        // send to AS ->> VLD UID TID
                            memset(buffer, '\0', strlen(buffer)*sizeof(char));
                            sprintf(buffer, "VLD %s %s\n", uid, tid);

                            printf("send to AS: %s", buffer);

                            Server_Client_Send(udpClientSocket, buffer);

                            createFdFile(uid, i);
                            
                            //continue;
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