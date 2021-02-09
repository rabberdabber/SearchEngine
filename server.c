#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>
#include "helper.h"
#include "common.h"
#include "thread_pool.h"
#include "tc_malloc.h"

#define HASH_MULTIPLIER 65599
#define MAX_WORD_LEN 2048
#define MAX_PATH_LEN 4096
#define DOCSIZE 10
#define EXTEND 2



DynList_T Dynlist,file_list;


typedef struct Args {
    int fd;
}args;

struct Hdr *create_pckt(size_t len, uint32_t msg_type)
{

    struct Hdr *header = (struct Hdr *)tc_malloc(sizeof(struct Hdr));

    if (!header)
        return NULL;

    header->tot_len = htonl(len + HDR_LEN);
    header->msg_type = htonl(msg_type);

    return header;
}

static uint hash_function(const char *pcKey)
{
    int i;
    unsigned int uiHash = 0U;
    for (i = 0; pcKey[i] != '\0'; i++)
        uiHash = uiHash * (unsigned int)HASH_MULTIPLIER + (unsigned int)pcKey[i];
    return uiHash;
}

void disperse_keys(DynList_T dynlist)
{
    size_t curr_len = dynlist->phys_len;
    size_t prev_len = dynlist->phys_len / 2;
    size_t curr_index, tmp, last_index = dynlist->last_index;
    Word_T word;

    void **list_ptr = dynlist->dyn_list;

    if (!dynlist)
    {
        fprintf(stderr, "empty list argument\n");
    }

    for (size_t index = prev_len; index < curr_len; index++)
    {
        word = (Word_T)list_ptr[index];

        if (word && (tmp = word->hash) >= prev_len)
        {
            curr_index = tmp%curr_len;

            while (curr_index < curr_len)
            {
                if (!list_ptr[curr_index])
                {
                    list_ptr[curr_index] = word;

                    //store max index in last_index
                    dynlist->last_index = (curr_index > last_index) ? curr_index + 1 : last_index;
                    break;
                }

                curr_index++;
            }
        }
    }
}

static int parse(DynList_T dynlist, char *buf, size_t max_len,
                 size_t docId, size_t line_num)
{
    char *ptr = buf, *start = buf;
    size_t len, str_len, hash, tmp;
    int index = 0;
    void *item;
    List_T linklist, doc;
    Word_T word;
   
    int ret;

    if (!dynlist || !buf)
    {
        fprintf(stderr, "empty list argument\n");
        return -1;
    }

    while (index < max_len && *ptr)
    {
        start = ptr = StrStrip(ptr, max_len - index, &index);

        if (*ptr == '\0')
            return 1;

        len = max_len - index;
        ptr = StrTok(start,len, &index);

        if (*ptr != '\0')
        {
            ptr = StrSplit(ptr,len, &index);
        }

        str_len = StrGetLength(start, MAX_WORD_LEN);

        tmp = hash_function(start);

        hash = tmp % dynlist->phys_len;

        if ((item = lookup(dynlist, start, hash)))
        {

            if ((linklist = lookup_link(&(((Word_T)item)->list), docId)))
            {
                if (!insert(linklist, NULL, line_num, linklist->last_index, LINKLIST))
                {
                    return -1;
                }
            }
            else
            {
                doc = create_link(docId);

                if (!insert(doc, NULL, line_num, doc->last_index, LINKLIST))
                {
                    return -1;
                }

                insert_link(&(((Word_T)item)->list), doc);
            }
        }

        else
        {
            word = create_word();
            word->hash = tmp;
            word->str = tc_malloc((str_len + 1) * sizeof(char));

            StrCopy(word->str, start, MAX_WORD_LEN);
            doc = create_link(docId);
            insert(doc, NULL, line_num, doc->last_index, LINKLIST);
            insert_link(&(word->list), doc);
            ret = insert(dynlist, word, 0, hash, DYNLIST);

            if (ret == EXTEND)
            {
                disperse_keys(dynlist);
            }
        }
        /*---------------------------------*/
    }

    return 1;
}


static int tokenizer(DynList_T dynlist, char *filename, char *abs_path, size_t docId)
{
    FILE *fp;
    size_t name_len, path_len, file_len, line_num = 1;
    name_len = MAX_WORD_LEN;
    path_len = MAX_PATH_LEN;
    file_len = name_len + path_len + 1;
    char buf[DYN_SIZE + 1] = {0}, file_buf[MAX_WORD_LEN + MAX_PATH_LEN + 1] = {0};
    char *ret;

    name_len = StrGetLength(filename, MAX_WORD_LEN);
    path_len = StrGetLength(abs_path, MAX_PATH_LEN);

    StrCopy(file_buf, abs_path, file_len);
    StrConcat(file_buf, "/", file_len);
    StrConcat(file_buf, filename, file_len);

    fp = fopen(file_buf, "r");


    if (fp)
    {
        while (!feof(fp) && (ret = fgets(buf, DYN_SIZE, fp)))
        {
            parse(dynlist, buf, DYN_SIZE, docId, line_num++);
        }

        fclose(fp);
    }
    else
    {
        perror("fopen");
        return -1;
    }

    return 1;
}


DIR *read_dir(DynList_T Dynlist, DynList_T file_list, char *abs_path)
{
    DIR *folder;
    struct dirent *entry;

    folder = opendir(abs_path);
    size_t docId = 0;

    if (folder)
    {
        while ((entry = readdir(folder)))
        {
            if (entry->d_type != DT_REG || !StrCompare(entry->d_name, ".", MAX_WORD_LEN) || !StrCompare(entry->d_name, "..", MAX_WORD_LEN))
                continue;

            insert(file_list, entry->d_name, 0, file_list->last_index, DYNLIST);

            /* for each file parse the words 
               and insert to hash table */
            tokenizer(Dynlist, entry->d_name, abs_path, docId++);
        }
    }
    return folder;
}

void thread_job(void *arg)
{
    Word_T word;
    size_t hash;
    List_T docs;
    int len,tot_len = 0;


    char * packet = (char *)tc_malloc(DYN_SIZE + 1);


    int fd = ((args *)arg)->fd;

    recv_from_host(fd,packet,HDR_LEN);
    
    struct Hdr * header = (struct Hdr *)packet;


    if(header->msg_type == htonl(REQUEST))
    {
        len = ntohl(header->tot_len) - HDR_LEN;

        int tmp = recv_from_host(fd, packet,len);
        packet[tmp] = '\0';

        hash = hash_function(packet) % Dynlist->phys_len;

        word = (Word_T)lookup(Dynlist,packet, hash);

        if (word)
        {
            for (docs = word->list; docs != NULL; docs = docs->next)
            {
                for (int i = 0; i < docs->last_index; i++)
                {
                    tot_len += sprintf(packet+tot_len,"%s: line #%zu\n",
                            ((char *)file_list->dyn_list[docs->val]), docs->list[i]);

                }
            }
        

            struct Hdr * sent_header = create_pckt(tot_len,RESPONSE);

            // send the header
            send_to_host(fd,(const char *)sent_header,HDR_LEN);
            // send the payload
            send_to_host(fd,packet,tot_len);

            tc_free(sent_header);
        }

        else
        {
            struct Hdr * error_header = create_pckt(0,ERROR);
            send_to_host(fd,(const char *)error_header,HDR_LEN);
            tc_free(error_header);
        }
        

    }

    tc_free(packet);
    close(fd);
}

int main(int argc, char *argv[])
{
    int listenfd;
    DIR * folder;

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

    Dynlist = create_list(DYN_SIZE);
    file_list = create_list(DOCSIZE);


    if (argc != 3)
    {
        fprintf(stderr, "Usage: ./[server_name] [abs_path] [server_port]");
        exit(EXIT_FAILURE);
    }

    char *abs_path = argv[1];

    /********** BOOTSTRAPPING ******************/
    if((folder = read_dir(Dynlist,file_list,abs_path)) == NULL)
    {
        fprintf(stderr, "Oops could not open the directory!\n");
        exit(EXIT_FAILURE);
    }

    printf("bootstrapping done\n");

    // register maximum socket
    int maxfd,newfd;
    fd_set currentfds,readyfds;

    char * server_port = argv[2];

    int rc,desc_ready = 0;
    listenfd = open_listenfd(atoi(server_port));
    maxfd = listenfd;

    FD_ZERO(&currentfds);
    FD_SET(listenfd,&currentfds);


    if(!init_pool())
    {
        fprintf(stderr,"couldn't create thread pool\n");
        exit(EXIT_FAILURE);
    }
    
    do
    {
        memcpy(&readyfds,&currentfds,sizeof(currentfds));
        rc = select(maxfd+1,&readyfds,NULL,NULL,NULL);


        if(rc <= 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        desc_ready = rc;
        for(int i = 0;i <= maxfd && desc_ready > 0;i++)
        {
            // active socket
            if(FD_ISSET(i,&readyfds))
            {
                desc_ready -= 1;

                if(i == listenfd)
                {
                
                    while ((newfd = accept(listenfd, NULL, NULL)))
                    {
                        if (newfd < 0)
                        {
                            if (errno != EWOULDBLOCK)
                                perror("accept");

                            break;
                        }

                        FD_SET(newfd, &currentfds);

                        if (newfd > maxfd)
                        {
                            maxfd = newfd;
                        }
                    }
                }
               
                else
                {
                        args * arg = (args *)tc_malloc(sizeof(args));
                        arg->fd = i;
                        job_t * job = (job_t *) tc_calloc(1,sizeof(job_t));
                        job->exec_func = thread_job;
                        job->func_data = arg;

                        /* added the job to the pool */
                        add_job_to_pool(job);

                        FD_CLR(i,&currentfds);
                }
            


            }
        }


    }while(1);

   

    close(listenfd);
    closedir(folder);
    destroy_list(Dynlist);
    tc_free(file_list->dyn_list);
    tc_free(file_list);
    return 0;
}


