typedef struct target_t{
int status;
char* filename;
struct depend_t* children;
struct target_t* next;
char* cmd_args;
}target_t;
