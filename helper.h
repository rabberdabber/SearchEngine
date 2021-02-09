#ifndef _HELPER_H_
#define _HELPER_H_

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define DYN_SIZE 10240
#define LIST_SIZE 10

enum ListType
{
    DYNLIST,
    LINKLIST
};

struct DynList
{
    size_t last_index;
    size_t phys_len;
    void **dyn_list;
};

struct LinkedList
{
    size_t val;
    size_t *list;
    size_t list_len;
    size_t last_index;
    struct LinkedList *next;
};

typedef struct DynList *DynList_T;
typedef struct LinkedList *List_T;

struct Word
{
    size_t hash;
    char *str;
    List_T list;
};

typedef struct Word *Word_T;

/********* LINKED LIST METHODS **************/
List_T create_link(size_t val);
void destroy_link(List_T link);
void insert_link(List_T *head, List_T node);
List_T lookup_link(List_T *head, size_t val);
void destroy(List_T head);

/* linkedlist wrapper */
Word_T create_word();
void destroy_word(Word_T word);

/************** DYNAMIC LIST METHODS ********/
DynList_T create_list(size_t init_size);
void destroy_list(DynList_T Dynlist);

int resize(void *buf, enum ListType type);

int insert(void *list, void *item_ptr, size_t val,
           size_t index, enum ListType type);
void *lookup(DynList_T dynlist, char *str, size_t index);

/*******STRING METHODS **********/

size_t StrGetLength(const char *pcSrc, size_t maxLen);
char *StrCopy(char *pcDest, const char *pcSrc, size_t max_cpy_size);
int StrCompare(const char *pcS1, const char *pcS2, size_t max_len);
char *StrSearch(const char *pcHaystack, const char *pcNeedle, size_t max_len);
char *StrConcat(char *pcDest, const char *pcSrc, size_t max_len);
char *StrTok(char *start, size_t max_len, int *index);
char *StrSplit(char *end, size_t max_len, int *index);
char *StrStrip(char *start, size_t max_len, int *index);
void flush_buf(void *ptr, size_t start, size_t end);

#endif
