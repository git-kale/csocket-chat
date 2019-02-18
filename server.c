#include <stdio.h>      //Standred input outout library function
#include <stdlib.h>     //A general purpose library 
#include <sys/socket.h> //Contains defination of structures needed for soclet
#include <sys/types.h>  //Contains defination of data types used in syscalls
#include <netinet/in.h> //Contains structures needed for internet domain addresses
#include <unistd.h>     //Provides access to POSIX operating system API
#include <string.h>     //Contains function related to string manipulation

//Negative value of fd signifies error returned


void error(const char *message)
{
    //If error occures print error message & exit 
    perror(message);
    exit(1);
}

int main(int argc, char const *argv[])
{
    if (argc < 2) 
    {
        // If no ports are specified terminate connection
        printf("No ports specified.\nConnection failed\n");
        exit(1);
    }
    
    int sockfd, newsockfd, portno, n;
    char stream[255];

    //Sockaddr_in describes internet sock address defined in netinet/in.h
    struct sockaddr_in serv_addr , cli_addr;
    socklen_t clilen;
    
    //AF_INET : IPv4 connection
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("Socket creation failed");
    }

    //Clear any data residing in serv_addr
    bzero((char*) &serv_addr , sizeof(serv_addr));
    portno = atoi(argv[1]);

    //INADDR_ANY : Address to accept any incoming message
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Socket binding failed");
    }
}
