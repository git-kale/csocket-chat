#include <stdio.h>      //Standred input outout library function
#include <stdlib.h>     //A general purpose library 
#include <sys/socket.h> //Contains defination of structures needed for soclet
#include <sys/types.h>  //Contains defination of data types used in syscalls
#include <netinet/in.h> //Contains structures needed for internet domain addresses
#include <unistd.h>     //Provides access to POSIX operating system API
#include <string.h>     //Contains function related to string manipulation

//Negative value of fd signifies error returned

#define TRUE 1
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
        printf("Usage : ./server <port>\n");
        exit(1);
    }
    
    int sockfd, newsockfd, portno, max_conn, max_streamlen;
    max_streamlen = 255;
    char stream[max_streamlen];

    //Sockaddr_in describes internet sock address defined in netinet/in.h
    struct sockaddr_in serv_addr , cli_addr;
    socklen_t clilen;
    
    //AF_INET : IPv4 connection
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("Socket creation failed");
    }

    //Shall place n zero-valued bytes in the area pointed to by s.
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

    // max_conn signifies maximum no. of connection queued before others are refused.  
    max_conn = 4;
    listen(sockfd, max_conn);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

    if (newsockfd < 0)
    {
        error("Socket acceptance failed");
    }

    while(TRUE)
    {
        bzero(stream, max_streamlen);
        int streamlen = read(newsockfd,stream, max_streamlen);

        if(streamlen < 0)
        {
            error("Socket reading failed");
        }
        printf("Client : %s\n",stream);
        bzero(stream,max_streamlen);
        fgets(stream, max_streamlen, stdin);

        streamlen = write(newsockfd,stream,strlen(stream));
        if (streamlen < 0) 
        {
            error("Socket writing failed");
        }
        int i = strncmp("Bye",stream, 3);
        if (i == 0)
        {
            break;
        }
    }
    close(newsockfd);
    close(sockfd);
    return 0;
}
