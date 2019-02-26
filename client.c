#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netdb.h> //Definitions for network database operations
#include <netdb.h>

#define PORT 0
//Negative value of fd signifies error returned

#define TRUE 1
struct data_packet
{
    /* data */
    int version;
    int length;
    char payload[150];
};

void error(const char *message)
{
    //If error occures print error message & exit
    perror(message);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        // If no ports are specified terminate connection
        printf("Usage : ./client <-p> <port> <host>\n");
        exit(1);
    }

    int sockfd, portno, streamlen, max_streamlen;
    char *hostname;
    struct sockaddr_in serv_addr;
    max_streamlen = 150;

    if (strcmp(argv[1], "-p") == 0)
    {
        portno = atoi(argv[2]);
        strcpy(hostname, "192.168.2.16");
        printf("%s",hostname);
    }
    else
    {
        portno = atoi(argv[4]);
        strcpy(hostname, argv[2]);
    }

    //Description of data base entry for a single host

    char stream[max_streamlen];
    //AF_INET : IPv4 connection
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("Socket opening failed");
    }
    //Shall place n zero-valued bytes in the area pointed to by s.
    bzero((char *)&serv_addr, sizeof(serv_addr));

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = inet_addr(hostname); 

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Socket connection failed");
    }

    printf("Connecting to server... Connected!\n");
    printf("Connected to a friend! You send first.\n");

    while (TRUE)
    {
        struct data_packet aPacket;
        aPacket.version = htons(457);

        printf("Client : ");
        bzero(stream, max_streamlen);
        bzero(aPacket.payload, 150);
        fgets(stream, max_streamlen, stdin);

        if (strlen(stream) > 150)
        {
            printf("Error: Input too long.\n");
            continue;
        }

        aPacket.length = htons(strlen(stream));
        strcpy(aPacket.payload, stream);

        streamlen = write(sockfd, stream, strlen(stream));

        if (streamlen < 0)
        {
            error("Socket writing failed");
        }

        bzero(aPacket.payload, 150);
        //Shall place n zero-valued bytes in the area pointed to by s.
        bzero(stream, max_streamlen);
        streamlen = read(sockfd, &aPacket, sizeof(aPacket));

        if (streamlen < 0)
        {
            error("Socket reading failed");
        }
        printf("Server :%s", aPacket.payload);
        int i = strncmp("Bye", stream, 3);
        if (i == 0)
        {
            break;
        }
        // close(sockfd);
        return 0;
    }
}