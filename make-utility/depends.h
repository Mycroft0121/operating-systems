#include "target.h"

typedef struct depend_t{
struct target_t* head;
int size;
}depend_t;

target_t *getHead(depend_t *);
target_t *getChild(depend_t*,int index);
target_t *search(depend_t *,char *);
target_t *searchAndPop(depend_t *,char *);
void push2(target_t**, target_t *);
void push(depend_t*, target_t *);
target_t *newNode(char* filename);
target_t *nullNode();
depend_t* createList(char* filename);
void printList(depend_t* lst);
int size(depend_t* lst);
int isLeaf(target_t* node);
target_t* treeSearch(target_t* node, char* filename);

