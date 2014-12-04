/********************
 * util.h
 *
 * You may put your utility function definitions here
 * also your structs, if you create any
 *********************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

// the following ifdef/def pair prevents us from having problems if 
// we've included util.h in multiple places
#ifndef _UTIL_H_
#define _UTIL_H_

//You can change any value in this file as you want. 
#define INELIGIBLE 0
#define READY 1
#define RUNNING 2
#define FINISHED 3

#define MAX_LENGTH 1024
#define MAX_DEPENDENCIES 10
#define MAX_TARGETS 10

// This stuff is for easy file reading
FILE * file_open(char*);
char * file_getline(char*, FILE*);
int is_file_exist(char *);
int get_file_modification_time(char *);
int compare_modification_time(char *, char *);
int makeargv(const char *s, const char *delimiters, char ***argvp);
void freemakeargv(char **argv);

// Additional utility functions //
int checkBlankOrComment(char* line);
char* line_trim(char* line);
int checkCommand(char* line);
int CheckTarget(char* line);
int is_empty(char *line);

#endif
