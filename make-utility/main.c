#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <unistd.h>
#include "depends.h"
#include "util.h"

/* Function declarations - START */
void printLeafs(target_t* node);
void foo();
void printNode(target_t *node);
int nFlag =0;
int modCheckTime(target_t *node,target_t *master);
/* Function declarations - END */

//This function will parse makefile input from user or default makeFile. 
int parse(char * lpszFileName,target_t* master)
{
	int nLine=0;
	char line[1024];
	FILE * fp = file_open(lpszFileName);
	int firstLine = 1;
	char **argbuff;
	target_t* lastTarget;
	int numToks;
	
	/* Invalid FILE */
	if(fp == NULL)
	{
		return -1;
	}
			/* Begin parsing */
			while(file_getline(line, fp) != NULL)
			{
					/* First line? */
					if(firstLine)
					{
						if(checkTarget(line) != 1)
						{
							printf("ERRROR syntax error\n");
							return;
						}
						firstLine =0;
					}
					/* Line is a command line? */
					if(checkCommand(line))
					{
						makeargv(line, "\t\n", &argbuff);
						if(argbuff[0] == NULL)
						{
							goto skip;
						}
						lastTarget->cmd_args = argbuff[0];
						printf("command %s\n" , lastTarget->cmd_args);
						goto skip;
					}
					/* Retrieve targets */
					else
					{
						numToks = makeargv(line, ": \n", &argbuff);
						printf("%d\n", numToks);
						if(argbuff[0] == NULL)
						{
							goto skip;
						}
					
					}
					
					
					/* Is target? */
					if(checkTarget(line) == 1)
					{
						/* Begin search */						
						lastTarget = treeSearch(master, argbuff[0]);
							if(lastTarget == NULL)
							{
								target_t* child = newNode(argbuff[0]);
								push(master->children,child);
								lastTarget = child;
							}
						nLine=1;
					}
					else
					{
						nLine=0;
					}
					/* Parsing */
					for(nLine; nLine < numToks; nLine++)
					{
						  printf("current iteration = %s\n" , argbuff[nLine]);
						/* Retrieve command */
						if(checkCommand(argbuff[nLine]) == 0)
						{
							target_t *search = treeSearch(master, argbuff[nLine]);
							if(search == NULL)
							{	
								target_t *child = newNode(argbuff[nLine]);
								push(lastTarget->children, child);
							}
							else
							{
								push(lastTarget->children,search);
							}
							
						}
						else
						{
							makeargv(line, "\t", &argbuff);					
							lastTarget->cmd_args = argbuff[0];
							
						}
					}
					printLeafs(master);
					skip:
						foo();
				}
			
	fclose(fp);

	return 0;
}

/* Exceptions */
void show_error_message(char * lpszFileName)
{
	fprintf(stderr, "Usage: %s [options] [target] : only single target is allowed.\n", lpszFileName);
	fprintf(stderr, "-f FILE\t\tRead FILE as a maumfile.\n");
	fprintf(stderr, "-h\t\tPrint this message and exit.\n");
	fprintf(stderr, "-n\t\tDon't actually execute commands, just print them.\n");
	fprintf(stderr, "-B\t\tDon't check files timestamps.\n");
	fprintf(stderr, "-m FILE\t\tRedirect the output to the file specified .\n");
	exit(0);
}

int main(int argc, char **argv) 

{
	// Declarations for getopt
	extern int optind;
	extern char * optarg;
	int ch;
	char * format = "f:hnBm:";
	
	// were going to push any target we havent found to this.
	target_t *master = nullNode();

	// Default makefile name will be Makefile
	char szMakefile[64] = "Makefile";
	char szTarget[64];
	char szLog[64];

	// Declare the target node and initialize
	while((ch = getopt(argc, argv, format)) != -1) 
	{
		switch(ch) 
		{
			case 'f':
				strcpy(szMakefile, strdup(optarg));
				break;
			case 'n':
				break;
			case 'B':
				break;
			case 'm':
				strcpy(szLog, strdup(optarg));
				break;
			case 'h':
			default:
				show_error_message(argv[0]);
				exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	// at this point, what is left in argv is the targets that were 
	// specified on the command line. argc has the number of them.

	int i = 0;

	// Only 1 allowable target by 'make'. If multiple targets remain after removing command tags, error out.
	if(argc > 1)
	{
		for(i = 0; i < argc; i++)
		{
			printf("%s\n", argv[i]);
		}
		printf("You have entered too many arguments, \nplease enter only one.\n");

		return EXIT_FAILURE;
	}


	//You may start your program by setting the target that make4061 should build.
	//if target is not set, set it to default (first target from makefile)

	char line[1024];
	char **argbuff;
	int numToks = 0;
	target_t *target = newNode(szMakefile);
	FILE * fp1 = file_open(target->filename);


	// This conditional specifies when a specific target is supplied on the command line (i.e. ./make4061 MyProg)
	if(argc == 1)
	{		 		
		if(strcmp(argv[0], "clean") == 0)
		{
			// set clean flag
		}
		else
		{						
			
			if(fp1 == NULL)
			{
				return EXIT_FAILURE;
			}						
			else
			{				
				parse(szMakefile,master);		
			}
		}
	}

	// Otherwise, the default target is the first line of the supplied makefile
	else
	{
		target = master;
		if(parse(szMakefile,master) == -1)
		{
			perror("FAILURE? \n");
			return EXIT_FAILURE;
		}
		if(master != NULL , master->children != NULL)
		{
			target = getTail(master->children);
		}
	}

	fclose(fp1);

		if(make4061(target,master) == -1)
		{
			return EXIT_FAILURE;
		}
	return EXIT_SUCCESS;
}

/* Print leaves of a supplied node */
void printLeafs(target_t* node)
{
    if(isLeaf(node))
    {
        printf("leaf = %s\n" , node->filename);
    }
    int i;
    for(i=0; i<node->children->size; i++)
    {
        printLeafs(getChild(node->children,i));
    }
}

/* Node Joining */
int make4061(target_t* node,target_t *master)
{
	int i;
	int cpid,wpid;
	int arg;
	
	/* Clean target? */
	if(strcmp(node->filename,"clean") == 0)
	{
			return TRUE;
	}
	
	/* Leaf node? */
	if(isLeaf(node))
	{
		if(node->cmd_args != NULL)
		{
			char** buffer;
			if(nFlag == 1)
			{
				printf("%s\n",node->cmd_args);
				return;
			}
			/* No dependencies */
			if(arg=makeargv(node->cmd_args, " ", &buffer) != 0)
			{
				execvp(*buffer,buffer);
			}
		}
		node->status = FINISHED;
		return TRUE;
	}
	pid_t cpids[node->children->size - 1]; 
	for(i=0; i<node->children->size; i++)
	{
		cpid = fork();

		if(childprocess(cpid))
		{				
			make4061(getChild(node->children,i),master);
			return TRUE;
		}
	}
	int status = INELIGIBLE;
	while ((wpid=wait(&status)) > INELIGIBLE)
	{
		// wait
	}

	char** buffer;
	if(node->cmd_args == NULL)
	{
		printf("node is null: %s\n" , node->filename);
	}
	if(node->cmd_args != NULL)
	{
		if(nFlag == TRUE)
		{
			printf("%s\n",node->cmd_args);
			return TRUE;
		}
		if(modCheckTime(node,master))
		{
			if(arg=makeargv(node->cmd_args, " ", &buffer) != 0)
			{
				execvp(*buffer,buffer);
			}
		}
	}
	return TRUE;
}

/* Boolean is child? */
int childprocess(int pid)
{
	return pid == 0;
}

/* Nothing */
void foo()
{
	
}
 /* Node printer */
void printNode(target_t *node)
{
	printf("your node %s \n",node->filename);
}

/* Display the modification time of a target */
int modCheckTime(target_t *node,target_t *master)
{
	depend_t *children = node->children;
	
	if(children != NULL)
	{
	  int i;
	  for(i=0; i<children->size; i++)
	  {
			if(treeSearch(master,getChild(children,i)->filename) == NULL)
			{
							printf("WE HAVE A BOGUS DEPENDENCY");
							return 0;
			}		
			return compare_modification_time(node->filename,getChild(children,i)->filename) != 0;		
	  }
	}
}


