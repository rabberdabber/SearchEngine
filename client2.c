#include "common.h"
#include "helper.h"
#include "tc_malloc.h"
#include <pthread.h>

#define MAX_SIZE 100

int server_port;
char * server_ip;

struct sockaddr_in *get_serveraddress(char *hostname, int port)
{
    int result;
    struct hostent *hp;
    struct sockaddr_in *serveraddr = tc_malloc(sizeof(struct sockaddr_in));
    memset(serveraddr, 0, sizeof(struct sockaddr_in));

    /*if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return -1;
    }*/

    serveraddr->sin_family = AF_INET;
    serveraddr->sin_port = htons(port);

    /* the given String is a valid ip address */
    if ((result = inet_pton(AF_INET, hostname, &(serveraddr->sin_addr))) == 1)
    {
        return serveraddr;
    }

    else if ((hp = gethostbyname(hostname)) != NULL)
    {
        serveraddr->sin_addr = *((struct in_addr *)hp->h_addr);
        bzero(&(serveraddr->sin_zero), 8);
        return serveraddr;
    }

    perror("gethostbyname");
    return NULL;
}

struct Hdr *create_pckt(size_t len, uint32_t msg_type)
{

    struct Hdr *header = (struct Hdr *)tc_malloc(sizeof(struct Hdr));

    if (!header)
        return NULL;

    header->tot_len = htonl(len + HDR_LEN);
    header->msg_type = htonl(msg_type);

    return header;
}

void * request_server(void * arg)
{
    int clientfd,tmp;
    char buf[BUFSIZE] = {0};
    char cmd_buf[100] = {0};

    void *threadcache;
    struct sockaddr_in *server_addr = ((thread_args *)arg)->server_addr;

    if ((threadcache = tc_thread_init()) == NULL)
    {
        fprintf(stderr, "thread local initialization failed\n");
        return NULL;
    }


    int tot_len = MAX_SIZE + HDR_LEN;

    void *packet = tc_malloc(tot_len);
    struct Hdr * header = (struct Hdr *)packet;
    header->msg_type = htonl(REQUEST);
    

    printf("*** Starting CLI:\n");
    while(1)
    {
        printf("mini_spotlight> ");

        scanf("%s %s",cmd_buf,buf);

        if(StrCompare(cmd_buf,"search",MAX_SIZE))
        {
            continue;
        }

        int len = StrGetLength(buf,MAX_SIZE);
        
        header->tot_len = htonl(len + HDR_LEN);
    
        memcpy(packet + HDR_LEN,buf,len);

        if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("socket");
            return NULL;
        }

        if (connect(clientfd, (struct sockaddr *)server_addr,
                    sizeof(struct sockaddr)) < 0)
        {
            close(clientfd);
            perror("connect");
            return NULL;
        }
        send_to_host(clientfd,packet,len + HDR_LEN);

        recv_from_host(clientfd,buf,HDR_LEN);

        struct Hdr * response_header = (struct Hdr *) buf;

        if(response_header->msg_type == htonl(RESPONSE))
        {
            len = ntohl(response_header->tot_len) - HDR_LEN;
           
            tmp = recv_from_host(clientfd,buf,len);
            buf[tmp] = '\0';

            
            printf("%s",buf);
            fflush(stdout);
        }
        else
        {
            printf("no such word in the server\n");
            fflush(stdout);
        }
        
        close(clientfd);
    }

    tc_free(packet);
    return NULL;

}

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        fprintf(stderr, "usage: ./client2 <server_ip> <server_port>\n");
        exit(EXIT_FAILURE);
    }

    /* setup tc_malloc */
    void *pgcache,*threadcache;
    if ((pgcache = tc_central_init()) == NULL)
    {
        fprintf(stderr, "failed to initialize tc_malloc\n");
        exit(EXIT_FAILURE);
    }

    if ((threadcache = tc_thread_init()) == NULL)
    {
        fprintf(stderr, "thread local initialization failed\n");
        exit(EXIT_FAILURE);
    }

    server_ip = argv[1];
    server_port = atoi(argv[2]);

    thread_args *args = (thread_args *)tc_malloc(sizeof(thread_args));
    args->server_addr = get_serveraddress(server_ip,server_port);

    pthread_t tid;
    if(pthread_create(&tid,NULL,request_server,(void *)args))
    {
        fprintf(stderr,"pthread_create error\n");
        exit(EXIT_FAILURE);
    }

    if(pthread_join(tid,NULL))
    {
        fprintf(stderr,"pthread_join error\n");
        exit(EXIT_FAILURE);
    }

   
    return 0;
}
