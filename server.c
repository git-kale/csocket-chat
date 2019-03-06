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
    LIMIT_EXCEEDED,
    PUBLIC_MESSAGE,
    PRIVATE_MESSAGE,
    USERNAME_ERROR
} status;

typedef struct message
{
    status flag;
    char username[36];
    char data[256];
} message;

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

void send_user_list(connection_info *reciever, int recv_no)
{
    message msg;
    msg.flag = GET_USERS;
    char *list = msg.data;

    int i;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {

        if (reciever[i].socket != 0)
        {
            list = stpcpy(list, reciever[i].username);
            list = stpcpy(list, "\n");
        }
    }

    if (send(reciever[recv_no].socket, &msg, sizeof(msg), 0) < 0)
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

void send_public_message(connection_info reciever[], int sender, char *text)
{
    message msg;
    msg.flag = PUBLIC_MESSAGE;
    strncpy(msg.username, reciever[sender].username, 36);
    strncpy(msg.data, text, 256);
    int i = 0;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        if (i != sender && reciever[i].socket != 0)
        {
            if (send(reciever[i].socket, &msg, sizeof(msg), 0) < 0)
            {
                error("Send failed");
            }
        }
    }
}

void send_private_message(connection_info reciever[], int sender, char *username, char *text)
{
    message msg;
    msg.flag = PRIVATE_MESSAGE;
    strncpy(msg.username, reciever[sender].username, 36);
    strncpy(msg.data, text, 256);

    int i;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        if (i != sender && reciever[i].socket != 0 && strcmp(reciever[i].username, username) == 0)
        {
            if (send(reciever[i].socket, &msg, sizeof(msg), 0) < 0)
            {
                error("Send failed");
            }
            return;
        }
    }

    msg.flag = USERNAME_ERROR;
    sprintf(msg.data, "Username \"%s\" does not exist or is not logged in.", username);

    if (send(reciever[sender].socket, &msg, sizeof(msg), 0) < 0)
    {
        error("Send failed");
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
        stop_connection_message(reciever, reciever[sender].username);
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
        case PUBLIC_MESSAGE:
            send_public_message(reciever, sender, msg.data);
            break;

        case PRIVATE_MESSAGE:
            send_private_message(reciever, sender, msg.username, msg.data);
            break;

            strcpy(reciever[sender].username, msg.username);
            printf("User connected: %s\n", reciever[sender].username);
            start_connection_message(reciever, sender);
            break;

        default:
            fprintf(stderr, "Unknown message type received.\n");
            break;
        }
    }
}

void trim_input(char *text)
{
    int len = strlen(text) - 1;
    if (text[len] == '\n')
    {
        text[len] = '\0';
    }
}

void handle_user_input(connection_info reciever[])
{
    char input[256];
    fgets(input, sizeof(input), stdin);
    trim_input(input);

    if (input[0] == 'q')
    {
        stop_server(reciever);
    }
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
    fd_set file_descriptors;

    connection_info server_info;
    connection_info reciever[CLIENT_LIMIT];

    int i;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        reciever[i].socket = 0;
    }

    start_server(&server_info, atoi(argv[1]));
    while (TRUE)
    {
        int max_fd = fd_maximum(&file_descriptors, &server_info, reciever);

        if (select(max_fd + 1, &file_descriptors, NULL, NULL, NULL) < 0)
        {
            perror("Select Failed");
            stop_server(reciever);
        }

        if (FD_ISSET(STDIN_FILENO, &file_descriptors))
        {
            handle_user_input(reciever);
        }

        if (FD_ISSET(server_info.socket, &file_descriptors))
        {
            handle_new_connection(&server_info, reciever);
        }

        for (i = 0; i < CLIENT_LIMIT; i++)
        {
            if (reciever[i].socket > 0 && FD_ISSET(reciever[i].socket, &file_descriptors))
            {
                handle_client_message(reciever, i);
            }
        }
    }
    return 0;
}