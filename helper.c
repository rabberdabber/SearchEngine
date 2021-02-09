#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "helper.h"
#include "tc_malloc.h"

/****** LIST METHODS ********/
List_T create_link(size_t val)
{
    List_T node = (List_T)tc_calloc(1, sizeof(struct LinkedList));

    if (!node)
    {
        perror("calloc");
    }
    node->val = val;
    node->list = tc_calloc(1, sizeof(size_t) * LIST_SIZE);
    node->list_len = LIST_SIZE;

    if (!(node->list))
    {
        fprintf(stderr,"tc_calloc failed\n");
    }

    return node;
}

void destroy_link(List_T link)
{
    tc_free(link->list);
    tc_free(link);
}

void insert_link(List_T *head, List_T node)
{
    List_T ptr = *head;

    /* insert at the head of the list */
    node->next = ptr;
    *head = node;
    return;
}

List_T lookup_link(List_T *head, size_t val)
{
    List_T ptr = *head;

    for (; ptr != NULL; ptr = ptr->next)
    {
        if (ptr->val == val)
            return ptr;
    }

    return NULL;
}

void destroy(List_T head)
{
    List_T tmp;
    for (List_T ptr = head; ptr != NULL;)
    {
        tmp = ptr->next;
        destroy_link(ptr);
        ptr = tmp;
    }
}

/* wrapper for linked list with string */

Word_T create_word()
{
    Word_T word;

    word = (Word_T)tc_calloc(1, sizeof(struct Word));

    if (!word)
    {
        fprintf(stderr, "tc_calloc failed\n");
    }

    return word;
}

void destroy_word(Word_T word)
{
    if (!word)
        return;

    tc_free(word->str);

    if (word->list)
        destroy(word->list);

    tc_free(word);
}

DynList_T create_list(size_t init_size)
{
    DynList_T Dynlist = (DynList_T)tc_calloc(1, sizeof(struct DynList));

    if (!Dynlist)
        return Dynlist;

    Dynlist->phys_len = init_size;
    Dynlist->dyn_list = (void **)tc_calloc(init_size, sizeof(void *));
    if (!Dynlist->dyn_list)
    {
        fprintf(stderr, "tc_calloc failed\n");
        exit(EXIT_FAILURE);
    }
    return Dynlist;
}

void destroy_list(DynList_T Dynlist)
{
    void *tmp;

    for (size_t index = 0; index < Dynlist->last_index; index++)
    {
        if ((tmp = Dynlist->dyn_list[index]))
        {
            destroy_word((Word_T)tmp);
        }
    }

    tc_free(Dynlist->dyn_list);
    tc_free(Dynlist);
}

int resize(void *buf, enum ListType type)
{
    void *new_list;
    DynList_T list;
    List_T linklist;
    size_t phys_len, new_len;

    switch (type)
    {
    case LINKLIST:
        linklist = (List_T)buf;
        phys_len = linklist->list_len;

        new_len = phys_len + LIST_SIZE;

        new_list = tc_realloc(linklist->list,phys_len * sizeof(size_t),new_len * sizeof(size_t));

        if (!new_list)
        {
            perror("realloc");
            return -1;
        }

        linklist->list = new_list;
        linklist->list_len = new_len;
        break;
    case DYNLIST:

        list = (DynList_T)buf;
        phys_len = list->phys_len;

        new_len = phys_len * 2;

        new_list = tc_realloc(list->dyn_list,phys_len * sizeof(void *),new_len * sizeof(void *));
        
        if (!new_list)
        {
            perror("realloc");
            return -1;
        }

        list->dyn_list = (void **)new_list;
        list->phys_len = new_len;
        break;
    }

    return 1;
}

int insert(void *list, void *item_ptr, size_t val,
           size_t index, enum ListType type)
{

    DynList_T Dynlist;
    List_T Linklist;
    void **list_ptr;
    int ret = 1;

    if (!list)
    {
        fprintf(stderr, "empty list argument\n");
        return -1;
    }

    if (type == DYNLIST)
    {
        Dynlist = (DynList_T)list;

        // TRY RESIZING
        if (Dynlist->last_index >= Dynlist->phys_len)
        {
            resize(Dynlist, DYNLIST);
            ret = 2;
        }

        if (index >= Dynlist->phys_len)
        {
            fprintf(stderr, "index out of bounds\n");
            return -1;
        }

        list_ptr = Dynlist->dyn_list;

        while (index < Dynlist->phys_len)
        {
            if (!list_ptr[index])
            {
                list_ptr[index] = item_ptr;
                //printf("inserted to hash_table %s\n",((Word_T) item_ptr)->str);
                break;
            }
            index++;
        }

        if (index > Dynlist->last_index)
        {
            Dynlist->last_index = index + 1;
        }
    }
    else
    {
        Linklist = (List_T)list;
        // TRY RESIZING
        if (Linklist->last_index >= Linklist->list_len)
        {
            resize(Linklist, LINKLIST);
        }

        Linklist->list[index] = val;
        Linklist->last_index++;
    }

    return ret;
}

void *lookup(DynList_T dynlist, char *str, size_t index)
{
    void *item;
    if ((item = dynlist->dyn_list[index]))
    {
        if (!StrCompare(((Word_T)item)->str, str, DYN_SIZE))
        {
            return item;
        }

        while (index < dynlist->phys_len &&
               (item = dynlist->dyn_list[++index]))
        {
            if (!StrCompare(((Word_T)item)->str, str, DYN_SIZE))
            {
                return item;
            }
        }
    }

    return NULL;
}

/* used for debugging purpose only */

void print_lists(List_T list)
{
    List_T ptr = list;

    for (; ptr != NULL; ptr = ptr->next)
    {
        printf("docId: %zu\n", ptr->val);
        for (int i = 0; i < ptr->last_index; i++)
        {
            printf("line num: %zu ", ptr->list[i]);
        }
    }
}

void print_words(DynList_T list)
{
    void **bufp = list->dyn_list;
    Word_T word;
    for (size_t i = 0; i < list->phys_len; i++)
    {
        if ((word = (Word_T)bufp[i]))
        {
            printf("\nIndex:%zu \n%s\n", i, word->str);
            print_lists(word->list);
        }
    }
}

/*********** STRING METHODS *********************/
/*------------------------------------------------------------------*/
/* StrGetLength: Takes a const char ptr and returns the number of   */
/*               Of chars pointed by the pointer.                   */
/*------------------------------------------------------------------*/
size_t StrGetLength(const char *pcSrc, size_t max_len)
{
    assert(pcSrc != NULL);

    const char *pcEnd;
    size_t curr_len = 0;
    pcEnd = pcSrc;

    while (curr_len < max_len && *pcEnd != '\0')
    {
        curr_len++;
        pcEnd++;
    }
    if (curr_len >= max_len)
    {
        fprintf(stderr, "buffer overflow\n");
        return 0;
    }

    return (size_t)(pcEnd - pcSrc);
}

/*------------------------------------------------------------------*/
/* StrCopy: Takes in destination and source pointers and copies the */
/*          Contents of source to the destination, assumes the      */
/*          Caller of this function should provide enough destin    */
/*          This function returns the pointer to destination.       */
/*------------------------------------------------------------------*/
char *StrCopy(char *pcDest, const char *pcSrc, size_t max_cpy_size)
{
    assert(pcDest != NULL && pcSrc != NULL);

    char *pD = pcDest;
    const char *pS = pcSrc;
    size_t cpy_size = 0;

    while (cpy_size < max_cpy_size && (*pD++ = *pS++) != '\0')
    {
        cpy_size++;
    }

    if (cpy_size >= max_cpy_size)
    {
        fprintf(stderr, "buffer overflow\n");
        return NULL;
    }

    return pcDest;
}

/*------------------------------------------------------------------*/
/* StrCompare: Takes in two pointers , compares strings and returns */
/*             0 if they are the same, or positive num if the first */
/*             Different char has a larger ascii code  else returns */
/*             A negative num.                                      */
/*------------------------------------------------------------------*/
int StrCompare(const char *pcS1, const char *pcS2, size_t max_len)
{
    assert(pcS1 != NULL && pcS2 != NULL);

    const char *ptr1 = pcS1, *ptr2 = pcS2;
    size_t curr_len = 0;

    for (; curr_len < max_len && *ptr1 != '\0' && *ptr1 == *ptr2; ptr1++, ptr2++)
    {
        curr_len++;
    }
    if (curr_len >= max_len)
    {
        fprintf(stderr, "buffer overflow\n");
        return 0;
    }

    return *ptr1 - *ptr2;
}
/*------------------------------------------------------------------*/
/* StrSearch: Takes in pointers,pcHaystack and pcNeedle,and searches*/
/*            The string pcNeedle in the string pointed by          */
/*            PcHaystack and returns the address of the first char  */
/*            Substring pcNeedle in pcHaystack if  found            */
/*            Or else it returns NULL.                              */
/*------------------------------------------------------------------*/
char *StrSearch(const char *pcHaystack, const char *pcNeedle, size_t max_len)
{
    assert(pcHaystack != NULL && pcNeedle != NULL);

    const char *ptrHay = pcHaystack;
    size_t curr_len;

    if (*pcNeedle == '\0') //check for an empty string
        return (char *)ptrHay;

    for (; curr_len < max_len && *ptrHay != '\0'; ptrHay++)
    {

        if (*ptrHay == *pcNeedle)
        {
            /* This block checks if the substring pointed by */
            /* ptrHay matches the string in pcNeedle.        */
            const char *ptrH = ptrHay, *ptrN = pcNeedle;

            for (; curr_len < max_len && *ptrH != '\0' && *ptrN != '\0'; ptrH++, ptrN++)
            {
                curr_len++;
                if (*ptrH != *ptrN)
                    break;
            }
            if (*ptrN == '\0') //match
                return (char *)ptrHay;

            if (*ptrH == '\0') //non-match
                return NULL;
        }
    }
    if (curr_len >= max_len)
    {
        fprintf(stderr, "buffer overflow\n");
    }

    return NULL;
}
/*------------------------------------------------------------------*/
/* StrConcat: Takes in two pointers and concatenates second string  */
/*            With the first string but the caller has to           */
/*            Make sure the destination size is enough.             */
/*------------------------------------------------------------------*/
char *StrConcat(char *pcDest, const char *pcSrc, size_t max_len)
{
    assert(pcDest != NULL && pcSrc != NULL);
    char *ptrDest = pcDest;
    size_t curr_len = 0;

    while (curr_len < max_len && *ptrDest != '\0')
    {
        ptrDest++;
    }
    if (curr_len >= max_len)
    {
        fprintf(stderr, "buffer overflow\n");
        return NULL;
    }

    StrCopy(ptrDest, pcSrc, max_len - (ptrDest - pcDest));

    return pcDest;
}

char *StrTok(char *start, size_t max_len, int *index)
{
    size_t i = 0;
    for (; *index < max_len && start[i] && isalpha(start[i]); i++, (*index)++)
        ;

    return &start[i];
}

char *StrStrip(char *start, size_t max_len, int *index)
{
    size_t i = 0;
    for (; *index < max_len && start[i] && !isalpha(start[i]); i++, (*index)++)
        ;

    return &start[i];
}

char *StrSplit(char *end, size_t max_len, int *index)
{
    char *ptr = end;
   
    *end = '\0';
    (*index)++;

    if (*index > max_len)
        return end;

    ptr = StrStrip(end + 1, max_len - (*index), index);

    return ptr;
}

