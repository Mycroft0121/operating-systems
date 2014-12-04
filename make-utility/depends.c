#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "depends.h"


/* Retrieve child node */
target_t* getChild(depend_t* list , int index)
{
	target_t* current = list->head;
	int i =0;
	while(i < index)
	{
		current = current->next;
		i++;	
	}
	
	return current;
}

/* Search tree for file */
target_t* search(depend_t* list,char* filename)
{
	if(list == NULL)
	{
		return NULL;
	}
	target_t* current = list->head;
	
	while(current != NULL)
	{	
		if(strcmp(filename,current->filename) == 0)
		{
			return current;
		}
		current = current->next;
	}
	return NULL;
}
/* Search tree and return file */
target_t* searchAndPop(depend_t* list,char* filename)
{
	if(list == NULL)
	{
		return NULL;
	}
	target_t* current = list->head;
	target_t* previous = NULL;
	target_t* target_node = NULL;
	
	while(current != NULL)
	{
		if(strcmp(filename,current->filename) == 0)
		{
			if(previous == NULL)
			{
				target_node = current;
				list->head = current->next;
				return target_node;		
			}
			else
			{
				previous->next = current->next;
				target_node = current;
				current = previous->next;
				if(list->size != 0)
				{
					list->size--;
				}
				return target_node;
			}
		}
		previous = current;
        current = current->next;
	}
	return target_node; 
}

// pushing a node to a node.
void push2(target_t** head , target_t* target_node)
{
	target_node->next = *head;
	*head = target_node;
}

void push(depend_t* list , target_t* target_node)
{
	target_node->next = list->head;
	list->head = target_node;
	list->size++;
}
/* Node constructor */
target_t* newNode(char* filename)
{
	target_t* node = (target_t*)malloc(sizeof(target_t));
	node->children = (depend_t*)malloc(sizeof(depend_t));
	node->filename = filename;
	node->next = NULL;
	
	return node;
}
/* Null constructor */
target_t* nullNode()
{
	target_t* node = (target_t*)malloc(sizeof(target_t));
	node->children = (depend_t*)malloc(sizeof(depend_t));
	node->filename = NULL;
	node->next = NULL;
	return node;
}
/* Node list */
depend_t* createList(char* filename)
{
	depend_t* lst = (depend_t*)malloc(sizeof(depend_t));
	lst->head = newNode(filename);
	lst->size = 1;
	return lst;
}

/* Print list of nodes */
void printList(depend_t* list)
{
	if(list == NULL)
	{
		return;
	}
	target_t* current = list->head;
	while(current != NULL)
	{
		printf("Node: %s\n",current->filename);
		current = current->next;
	}
}
/* Is leaf? boolean */
int isLeaf(target_t* node)
{
	if(node == NULL)
	{
		printf("Null in isLeaf\n");
		return 1;
	}
	
	return node->children->size == 0;
}
/* Full tree search */
target_t* treeSearch(target_t* node, char* filename)
{
	target_t* match = NULL;

		if(strcmp(node->filename,filename) == 0)
		{
			return node;
		}
		else
		{
			int i;
			for (i=0; i< node->children->size; i++)
			{
				match = treeSearch(getChild(node->children,i),filename);
				if(match != NULL)
				{
					return match;
				}
			}
		}
		
		return match;
}


