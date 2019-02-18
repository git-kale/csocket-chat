#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>     
#include <netdb.h>          //Definitions for network database operations
#include <ctype.h>          

//Negative value of fd signifies error returned

#define TRUE 1

void error(const char* message)
{
    //If error occures print error message & exit
    perror(message);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, streamlen, max_streamlen;
    max_streamlen = 255;

    struct sockaddr_in serv_addr;
    
    //Description of data base entry for a single host
    struct hostent *server;

    char stream[max_streamlen];
    if (argc < 3)
    {
        // If no ports are specified terminate connection
        printf("Usage : ./server <host(default: 127.0.0.1)> <port>\n");
        exit(0);
    }

    portno = atoi(argv[2]);

    //AF_INET : IPv4 connection
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if (sockfd < 0)
    {
        error("Socket opening failed");
    }
    server = gethostbyname(argv[1]);
    
    if(server == NULL )
    {
        error("No such host");
    }
    //Shall place n zero-valued bytes in the area pointed to by s.
    bzero((char*) &serv_addr,sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr_list[0], (char *)& serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if(connect(sockfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
    {
        error("Socket connection failed");
    }
    while(TRUE)
    {
        bzero(stream, max_streamlen);
        fgets(stream,max_streamlen,stdin);
        streamlen = write(sockfd,stream,strlen(stream));

        if(streamlen < 0)
        {
            error("Socket writing failed");
        }

        //Shall place n zero-valued bytes in the area pointed to by s.
        bzero(stream, max_streamlen);
        streamlen = read(sockfd, stream, max_streamlen);

        if (streamlen < 0)
        {
            error("Socket reading failed");
        }
        printf("Server :%s", stream);
        int i = strncmp("Bye", stream, 3);
        if (i == 0)
        {
            break;
        }
    }
    close(sockfd);
    return 0;
}   