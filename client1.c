#include "common.h"
#include "helper.h"
#include "tc_malloc.h"
#include <pthread.h>

#define MAX_SIZE 100

int server_port;
char *server_ip;
int nthreads;
int req_per_thread;
char * word;
int tot_len;

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

void *request_server(void *arg)
{
    char buf[BUFSIZE+1] = {0};
    struct sockaddr_in *server_addr = ((thread_args *)arg)->server_addr;

    void * threadcache;

    if ((threadcache = tc_thread_init()) == NULL)
    {
        fprintf(stderr, "thread local initialization failed\n");
        exit(EXIT_FAILURE);
    }

    int word_len = StrGetLength(word, 100);

    struct Hdr * header = create_pckt(word_len,REQUEST);
   


    for (int i = 0; i < req_per_thread; i++)
    {
        int clientfd,tmp,len;

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

        // send header
        send_to_host(clientfd,(const char *)header,HDR_LEN);

        // send the word
        send_to_host(clientfd,word,word_len);

        recv_from_host(clientfd, buf,HDR_LEN);

        struct Hdr *response_header = (struct Hdr *)buf;

        if (response_header->msg_type == htonl(RESPONSE))
        {
            len = ntohl(response_header->tot_len) - HDR_LEN;

            tmp = recv_from_host(clientfd,buf,len);

            buf[tmp] = '\0';
        }
        else if (response_header->msg_type == htonl(ERROR))
        {
            printf("no such word in the server\n");
            fflush(stdout);
        }
        else
        {
            printf("wrong command sent by server\n");
            fflush(stdout);
        }
        

        close(clientfd);
    }

    tc_free(header);
    return NULL;
}

int main(int argc, char *argv[])
{

    if (argc != 6)
    {
        fprintf(stderr, "usage: ./client1 <server_ip> <server_port> <numofthreads> <numofreq_perthread> <word>\n");
        exit(EXIT_FAILURE);
    }

    /* setup tc_malloc */
    void * pgcache,*threadcache;
    if((pgcache = tc_central_init()) == NULL)
    {
        fprintf(stderr,"failed to initialize tc_malloc\n");
        exit(EXIT_FAILURE);
    }

    if ((threadcache = tc_thread_init()) == NULL)
    {
        fprintf(stderr, "thread local initialization failed\n");
        exit(EXIT_FAILURE);
    }

    server_ip = argv[1];
    server_port = atoi(argv[2]);
    nthreads = atoi(argv[3]);
    req_per_thread = atoi(argv[4]);
    word = argv[5];

    pthread_t tid_buf[nthreads];

    thread_args *args = (thread_args *)tc_malloc(sizeof(thread_args));
    args->server_addr = get_serveraddress(server_ip,server_port);

    for(int i = 0; i < nthreads;i++)
    {
        if (pthread_create(&tid_buf[i], NULL, request_server,(void *)args))
        {
            fprintf(stderr, "pthread_create error\n");
            exit(EXIT_FAILURE);
        }
    }
   
    for(int i = 0; i < nthreads; i++)
    {
        if (pthread_join(tid_buf[i], NULL))
        {
            fprintf(stderr, "pthread_join error\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("number of served client requests: %d\n",nthreads * req_per_thread);
    fflush(stdout);
    tc_free(args);
    tc_free(args->server_addr);
    return 0;
}
