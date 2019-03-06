#include <stdio.h>      //Standred input outout library function
#include <stdlib.h>     //A general purpose library
#include <sys/socket.h> //Contains defination of structures needed for soclet
#include <sys/types.h>  //Contains defination of data types used in syscalls
#include <netinet/in.h> //Contains structures needed for internet domain addresses
#include <unistd.h>     //Provides access to POSIX operating system API
#include <string.h>     //Contains function related to string manipulation
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/select.h>

#define PORT 0
#define CLIENT_LIMIT 5

//Negative value of fd signifies error returned

#define TRUE 1
typedef enum
{
    CONNECT,
    DISCONNECT,
    SUCCESS,
    GET_USERS,
    SET_USERNAME,
    ERROR,
    LIMIT_EXCEEDED
} status;

typedef struct message
{
    status flag;
    char username[36];
    char data[256];
} message;

struct data_packet
{
    /* data */
    int version;
    int length;
    char payload[150];
};
typedef struct connection_info
{
    /* data */
    int socket;
    struct sockaddr_in address;
    char username[36];
} connection_info;

int fd_maximum(fd_set *fd, connection_info *server_info, connection_info reciever[])
{
    FD_ZERO(fd);
    FD_SET(STDIN_FILENO, fd);
    FD_SET(server_info->socket, fd);

    int max_fd = server_info->socket;
    int i;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        if (reciever[i].socket > 0)
        {
            FD_SET(reciever[i].socket, fd);
            if (reciever[i].socket > max_fd)
            {
                max_fd = reciever[i].socket;
            }
        }
    }
    return max_fd;
}

void error(const char *message)
{
    //If error occures print error message & exit
    perror(message);
    exit(1);
}

void start_server(connection_info *server_info, int port)
{
    if ((server_info->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        error("Socket creation failed");
    }

    server_info->address.sin_family = AF_INET;
    server_info->address.sin_addr.s_addr = INADDR_ANY;
    server_info->address.sin_port = htons(port);

    if (bind(server_info->socket, (struct sockaddr *)&server_info->address, sizeof(server_info->address)) < 0)
    {
        error("Socket binding failed");
    }
    const int optVal = 1;
    const socklen_t optLen = sizeof(optVal);
    if (setsockopt(server_info->socket, SOL_SOCKET, SO_REUSEADDR, (void *)&optVal, optLen) < 0)
    {
        error("Socket option setting failed");
    }

    if (listen(server_info->socket, 3) < 0)
    {
        error("Socket listening failed");
    }
    printf("Waiting for incoming connections...\n");
}

void stop_server(connection_info server_info[])
{
    int i;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        close(server_info[i].socket);
    }
    exit(0);
}

void start_connection_message(connection_info *reciever, int sender)
{
    message msg;
    msg.flag = CONNECT;
    strncpy(msg.username, reciever[sender].username, 36);
    int i = 0;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        if (reciever[i].socket != 0)
        {
            if (send(reciever[i].socket, &msg, sizeof(msg), 0) < 0)
            {
                error("Sending failed");
            }
            else if (i == sender)
            {
                msg.flag = SUCCESS;
            }
        }
    }
}

void stop_connection_message(connection_info *reciever, char *username)
{
    message msg;
    msg.flag = DISCONNECT;
    strncpy(msg.username, username, 36);
    int i = 0;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        if (reciever[i].socket != 0)
        {
            if (send(reciever[i].socket, &msg, sizeof(msg), 0) < 0)
            {
                perror("Send failed");
                exit(1);
            }
        }
    }
}

void handle_new_connection(connection_info *server_info, connection_info reciever[])
{
    int new_socket;
    int address_length;
    new_socket = accept(server_info->socket, (struct sockaddr *)&server_info->address, (socklen_t *)&address_length);

    if (new_socket < 0)
    {
        error("Socket acceptance failed");
    }

    int i;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        if (reciever[i].socket == 0)
        {
            reciever[i].socket = new_socket;
            break;
        }
        else if (i == CLIENT_LIMIT - 1)
        {
            limit_exceeded(new_socket);
        }
    }
}

void handle_client_message(connection_info reciever[], int sender)
{
    int read_size;
    message msg;

    if ((read_size = recv(reciever[sender].socket, &msg, sizeof(message), 0)) == 0)
    {
        printf("User disconnected: %s.\n", reciever[sender].username);
        close(reciever[sender].socket);
        reciever[sender].socket = 0;
        send_disconnect_message(reciever, reciever[sender].username);
    }
    else
    {

        switch (msg.flag)
        {
        case GET_USERS:
            send_user_list(reciever, sender);
            break;

        case SET_USERNAME:;
            int i;
            for (i = 0; i < CLIENT_LIMIT; i++)
            {
                if (reciever[i].socket != 0 && strcmp(reciever[i].username, msg.username) == 0)
                {
                    close(reciever[sender].socket);
                    reciever[sender].socket = 0;
                    return;
                }
            }

            strcpy(reciever[sender].username, msg.username);
            printf("User connected: %s\n", reciever[sender].username);
            send_connect_message(reciever, sender);
            break;

        default:
            fprintf(stderr, "Unknown message type received.\n");
            break;
        }
    }
}

void send_user_list(connection_info *clients, int receiver)
{
    message msg;
    msg.flag = GET_USERS;
    char *list = msg.data;

    int i;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        
        if (clients[i].socket != 0)
        {
            list = stpcpy(list, clients[i].username);
            list = stpcpy(list, "\n");
        }
    }

    if (send(clients[receiver].socket, &msg, sizeof(msg), 0) < 0)
    {
        error("Send failed");
    }
}

void limit_exceeded(int socket)
{
    message limit_exceeded;
    limit_exceeded.flag = LIMIT_EXCEEDED;

    if (send(socket, &limit_exceeded, sizeof(limit_exceeded), 0) < 0)
    {
        error("Send failed");
    }

    close(socket);
}

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        // If no ports are specified terminate connection
        printf("Usage : ./server <port>\n");
        exit(1);
    }

    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *addr;

    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            sa = (struct sockaddr_in *)ifa->ifa_addr;
            addr = inet_ntoa(sa->sin_addr);
            if (strcmp(ifa->ifa_name, "eth0") == 0)
            {
                break;
            }
        }
    }
    freeifaddrs(ifap);

    printf("Welcome to Chat!\n");

    int sockfd, newsockfd, portno, max_conn, max_streamlen;
    max_streamlen = 255;
    char stream[max_streamlen];

    //Sockaddr_in describes internet sock address defined in netinet/in.h
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clientLen;

    //AF_INET : IPv4 connection
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("Socket creation failed");
    }

    //Shall place n zero-valued bytes in the area pointed to by s.
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);

    //INADDR_ANY : Address to accept any incoming message
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Socket binding failed");
    }

    socklen_t serverLen = sizeof(serv_addr);

    if (getsockname(sockfd, (struct sockaddr *)&serv_addr, &serverLen) == -1)
    {
        error("ERROR on getsockname()\n");
    }
    else
    {
        printf("Waiting for connection on %s port %d\n", addr, ntohs(serv_addr.sin_port));
    }

    // max_conn signifies maximum no. of connection queued before others are refused.
    max_conn = 4;
    listen(sockfd, max_conn);
    clientLen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clientLen);

    if (newsockfd < 0)
    {
        error("Socket acceptance failed");
    }

    printf("You are server, be polite recieve first");

    while (TRUE)
    {
        struct data_packet aPacket;
        bzero(aPacket.payload, 150);
        bzero(stream, max_streamlen);

        int streamlen = read(newsockfd, stream, max_streamlen);

        if (streamlen < 0)
        {
            error("Socket reading failed");
        }
        printf("Client : %s%s", aPacket.payload, "PRINTED");

        while (TRUE)
        {
            printf("Server : ");
            bzero(stream, max_streamlen);
            bzero(aPacket.payload, 150);
            fgets(stream, max_streamlen, stdin);

            if (strlen(stream) > 150)
            {
                printf("Error: Input too long.\n");
                continue;
            }
            else
            {
                break;
            }
        }

        aPacket.version = htons(457);
        aPacket.length = htons(strlen(stream));

        strcpy(aPacket.payload, stream);

        streamlen = write(newsockfd, stream, strlen(stream));
        if (streamlen < 0)
        {
            error("Socket writing failed");
        }
        int i = strncmp("Bye", stream, 3);
        if (i == 0)
        {
            break;
        }
    }
    close(newsockfd);
    close(sockfd);
    return 0;
}
