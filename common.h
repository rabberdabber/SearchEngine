#ifndef __COMMON_H_
#define __COMMON_H_

// this may not be the best way look at other files
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/ioctl.h>


#define HDR_LEN 8
#define BACKLOG 1024
#define BUFSIZE 10240

enum MSG_TYPE
{
    REQUEST = 0x00000010,
    RESPONSE = 0x00000011,
    ERROR = 0x00000020
};

typedef struct Thread_args
{
    struct sockaddr_in * server_addr;
} thread_args;

struct Hdr
{
    uint32_t tot_len;
    uint32_t msg_type;
} __attribute__((packed));

int open_listenfd(int port)
{
    int listenfd, optval = 1;
    struct sockaddr_in serveraddr = {0};

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return -1;
    }

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval, sizeof(int)) < 0)
    {
        close(listenfd);
        perror("setsockopt");
        return -1;
    }

    if ((ioctl(listenfd, FIONBIO, (char *)&optval)) < 0)
    {
        perror("ioctl");
        close(listenfd);
        exit(-1);
    }

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *)&serveraddr,
             sizeof(serveraddr)) < 0)
    {
        close(listenfd);
        perror("bind");
        return -1;
    }

    if (listen(listenfd, BACKLOG) < 0)
    {
        close(listenfd);
        perror("listen");
        return -1;
    }

    return listenfd;
}

int open_clientfd(char *hostname, int port)
{
    int clientfd, result;
    struct hostent *hp;
    struct sockaddr_in serveraddr = {0};

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return -1;
    }

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);

    /* the given String is a valid ip address */
    if ((result = inet_pton(AF_INET, hostname, &(serveraddr.sin_addr)) == 1))
    {
    }

    else
    {
        if ((hp = gethostbyname(hostname)) == NULL)
        {
            close(clientfd);
            perror("gethostbyname");
            return -1;
        }

        bzero((char *)&serveraddr, sizeof(serveraddr));

        serveraddr.sin_family = AF_INET;

        bcopy((char *)hp->h_addr_list[0],
              (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
        serveraddr.sin_port = htons(port);
    }

    if (connect(clientfd, (struct sockaddr *)&serveraddr,
                sizeof(serveraddr)) < 0)
    {
        close(clientfd);
        perror("connect");
        return -1;
    }

    return clientfd;
}


int recv_from_host(int socket, char *buf, size_t len)
{
    ssize_t rd = 0, tmp = 0;
    size_t rem = len;

    while (rem > 0)
    {
        tmp = read(socket, buf + rd, rem);
        rem -= tmp;
        rd += tmp;
        if (tmp < 0)
        {
            perror("read");
            return tmp;
        }

        if (tmp == 0)
            return rd;

    }

    return rd;
}

int send_to_host(int socket, const char *buf, size_t len)
{
    ssize_t sent = 0, tmp = 0;
    size_t rem = len;

    while (rem > 0)
    {
        tmp = write(socket, buf + sent, rem);
        rem -= tmp;
        sent += tmp;

        if (tmp < 0)
        {
            perror("write");
            return tmp;
        }
    }

    return sent;
}


#endif
