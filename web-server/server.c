#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "util.h"
#include <syscall.h>

#define MAX_THREADS 100
#define MAX_QUEUE_SIZE 100
#define MAX_REQUEST_LENGTH 64

//Structure for queue.
typedef struct request_queue
{
	int		m_socket;
	char*	m_szRequest;
} request_queue_t;

FILE *log_fd;

/* ~~~ Lock Declarations - START ~~~ */
pthread_mutex_t log_file = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t req_access = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t req_full = PTHREAD_COND_INITIALIZER; /* Dispatcher must not access an empty queue */
pthread_cond_t req_empty = PTHREAD_COND_INITIALIZER; /* Worker must not access a full queue */
/* ~~~ Lock Declarations - End ~~~ */

/* ~~~ Request queue declarations - START ~~~ */
request_queue_t requests[MAX_QUEUE_SIZE];
int req_start = 0; /* Where to remove a request to the queue */
int req_end = 0; /* Where to add a request from the queue */
int num_reqs = 0; /* Number of requests handled */
int qlen; /* The fixed bounded length of the request queue */
/* ~~~ Request queue declarations - END ~~~ */

void * dispatch(void * arg)
{	
	/* START- Declarations */
	int thread_id = (int)arg; /* An integer from 0 to num_dispatchers - 1 indicating dispatcher-thread ID of the handling thread. */
	int fd = 0; /* The file descriptor given to you by accept_connection() for this request */
	int read = 0; /* Number of bytes returned by a successful request, or the error string returned by return error if error occured */
	char disp_buff[1024]; /* The filename buffer filled in by the get request function */
	char* disp_req; /* Current dispatcher request */
	/* END - Declarations */

	
	// Each of the next two functions will be used in order, in a loop, in the dispatch threads:
	// int accept_connection(void);
	// int get_request(int fd, char *filename); 
	
	while(1){

		fd = accept_connection(); // Receieve a connection
		if(fd < 0) {
			perror("Error: Bad Connection");
			pthread_exit(NULL); //if the return value is negative, the dispatch thread calling accept_connection must exit by calling pthread_exit().
		}
		
		/* Receive a request */
		if (get_request(fd, disp_buff) < 0) {
			// You must recover from bad requests
			printf("Bad request in thread: %d\n", thread_id); 
			continue;
		}
		disp_req = (char*)malloc(strlen(disp_buff)+1);
		strcpy(disp_req, disp_buff);
		
		/* Add a request to the queue */
		pthread_mutex_lock(&req_access); /* Check the lock - ensure queue is not full*/
		while (num_reqs == qlen) {
			pthread_cond_wait(&req_full, &req_access);
		}
		/* Request write */
		requests[req_end].m_socket = fd;
		requests[req_end].m_szRequest = disp_req;
		num_reqs++; /* Increment request counter */
		req_end = (req_end + 1) % qlen; /* Update ring buffer starting index */
		pthread_cond_signal(&req_empty); /* Update locks; unlock */
		pthread_mutex_unlock(&req_access);
	}
	return NULL;
}

void * worker(void * arg)
{
	/* START - Declarations */	
	int thread_id = (int)arg; /* An integer from 0 to num_workers - 1 indicating worker-thread ID of the handling thread. */
	int fd = 0; /* Current file descriptor */
	int i = 0; /* Indexer */
	int fd_sz; /* Size of the file requested */
	int curr_req_num; /* Current number of handled requests by this thread */
	char *req; /* The filename buffer filled in by the get request function */
	char wrk_buf[1024]; /* Worker working buffer */
	void* chunk; /* Memory chunk for opening the requested file */
	int file_chk = 0; /* File checker */
	int req_len = 0; /* Length of request */
	char* fd_type;
	/* END - Declarations */
	
	while(1)
	{
		/* Get a request */
		pthread_mutex_lock(&req_access);
		while(num_reqs == 0) /* Empty queue */
		{
			pthread_cond_wait(&req_empty, &req_access);
		}
		fd = requests[req_start].m_socket;
		if (fd == -9999) {
			pthread_mutex_unlock(&req_access); /* Unlock and exit */
			return NULL;
		}
		strcpy(wrk_buf, requests[req_start].m_szRequest);
		free(requests[req_start].m_szRequest);

		num_reqs--;
		req_start = (req_start + 1) % qlen; /* Update ring buffer retrieval index */
		pthread_cond_signal(&req_full);
		pthread_mutex_unlock(&req_access);
		curr_req_num++;
		
		if((file_chk = open(wrk_buf+1, O_RDONLY)) == -1)
		{
			return_error(fd, "Invalid file in requested directory.");
			pthread_mutex_lock(&log_file);
			fprintf(log_fd, "[%d][%d][%d][%s][%s]\n", thread_id, curr_req_num, fd, wrk_buf, "Invalid file.");
			fflush(log_fd);
			pthread_mutex_unlock(&log_file);
			continue;
		}
			struct stat file_stats;
			fstat(file_chk, &file_stats);
			fd_sz = file_stats.st_size;
			chunk = malloc(fd_sz);
			read(file_chk, chunk, fd_sz);
			close(file_chk);

		pthread_mutex_lock(&log_file);
		fprintf(log_fd, "[%d][%d][%d][%s][%d]\n", thread_id, curr_req_num, fd, wrk_buf, fd_sz);
		fflush(log_fd);
		pthread_mutex_unlock(&log_file);

		req_len = strlen(wrk_buf);
		if (strcmp(wrk_buf + req_len - 5, ".html") == 0) {
			fd_type = "text/html";
		}
		else if (strcmp(wrk_buf + req_len - 4, ".jpg") == 0) {
			fd_type = "image/jpeg";
		}
		else if (strcmp(wrk_buf + req_len - 4, ".gif") == 0) {
			fd_type = "image/gif";
		}
		else
		{
			fd_type = "text/plain";
		}

		if(return_result(fd, fd_type, chunk, fd_sz) != 0)
		{
			printf("Error sending result in thread: %d\n", thread_id);
		}

		free(chunk);

	}
	
	return NULL;
}

int main(int argc, char **argv)
{	
	/* START - Declarations */
	pthread_t p_worker[MAX_THREADS];
	pthread_t p_dispatcher[MAX_THREADS];
	int port = 0; /* Number can be specified ; 1025 - 65535 by default */
	char* path = argv[2]; /* The path to the web root location where files are served from */
	int num_dispatchers = 0; /* Number of dispatcher threads to start up */
	int num_workers = 0; /* Number of worker threads to start up */
	int cache_entries = 0; /* (OPTIONAL) The number of entries available in the cache */
	int i, j; /* Simple indexers */
	int thread_chk = 0; /* Thread error checker */
	log_fd = fopen("webserver_log", "w");
	/* END - Declarations */

	if(argc != 6 && argc != 7) /* Error check first. */
	{
		printf("usage: %s port path num_dispatchers num_workers queue_length [cache_size]\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Error checking & Field setting */
	for(i = 0; i < strlen(argv[1]); i++) /* Scan input value 1 to be valid port integer */
	{
		if(!isdigit(argv[1][i]))
		{
			perror("Error: Input 1 must be an integer");			
			return EXIT_FAILURE;
		}
	}
	port = atoi(argv[1]); /* Set the port field to be input entry 1 */
	
	if(port < 1025 || port > 65535) /* Valid port? */
	{
		perror("Error: Invalid port!");
		return EXIT_FAILURE;
	}

	for(i = 0; i < strlen(argv[3]); i++) /* Scan input value 3 to be valid dispatcher count */
	{
		if(!isdigit(argv[3][i]))
		{
			perror("Error: Input 3 must be an integer");			
			return EXIT_FAILURE;
		}
	}
	num_dispatchers = atoi(argv[3]); /* Set the num_dispatcher field */
	
	if(num_dispatchers < 0 || num_dispatchers > MAX_THREADS)
	{
		perror("Error: Dispatcher count must be a #: 1 - 100");
		return EXIT_FAILURE;
	}
	
	for(i = 0; i < strlen(argv[4]); i++) /* Scan input value 4 to be valid num_worker value */
	{
		if(!isdigit(argv[4][i]))
		{
			perror("Error: Input 4 must be an integer");			
			return EXIT_FAILURE;
		}
	}
	num_workers = atoi(argv[4]); /* Set the num_worker field */
	
	if(num_workers < 0 || num_workers > MAX_THREADS)
	{
		perror("Error: Worker count must be a #: 1 - 100");
		return EXIT_FAILURE;
	}
	 
	for(i = 0; i < strlen(argv[5]) - 1; i++); /* Scan input value 5 to be valid qlen value */
	{
		if(!isdigit(argv[5][i]))
		{
			perror("Error: Input 5 must be an integer");			
			return EXIT_FAILURE;
		}
	}
	qlen = atoi(argv[5]); /* Set the qlen field */
	
	if(qlen < 0 || qlen > MAX_QUEUE_SIZE)
	{
		perror("Error: Queue length must be a #: 1 - 100");
		return EXIT_FAILURE;
	}
	
	if (chdir(path)!=0) {
      perror("could not change directory to root");
      return EXIT_FAILURE;
    }

	init(port); // run ONCE in your main thread 

	printf("Server running on port %d with %d dispatchers and %d workers\n", port, num_dispatchers, num_workers);

	for(i = 0; i < num_workers; i++) /* Create num_workers threads */
	{
		thread_chk = pthread_create(&p_worker[i], NULL, worker, (void*)i); /* Create a worker thread */
		if(thread_chk == -1) /* Error checking - thread creation */
		{
			perror("Error: Worker thread creation error");			
			return EXIT_FAILURE;
		}
	}

	for(i = 0; i < num_dispatchers; i++) /* Create num_dispatchers threads */
	{
		thread_chk = pthread_create(&p_dispatcher[i], NULL, dispatch, (void*)i); /* Create a dispatcher thread */
		if(thread_chk == -1) /* Error checking - thread creation */
		{
			perror("Error: Dispatcher thread creation error");
			return EXIT_FAILURE;
		}

	}
	
	for(i = 0; i < num_dispatchers; i++)
	{
		pthread_join(p_dispatcher[i],NULL);
	}

	pthread_mutex_lock(&req_access);
	while(num_reqs == qlen) /* Full queue */
	{
		pthread_cond_wait(&req_full, &req_access);
	}
	requests[req_end].m_socket = -9999;
	requests[req_end].m_szRequest = NULL;
	num_reqs++;
	req_end = (req_end + 1) % qlen; /* Update ring buffer end index */
	pthread_cond_broadcast(&req_empty);
	pthread_mutex_unlock(&req_access);

	for(i = 0; i < num_workers; i++) /* Create num_workers threads */
	{
		pthread_join(p_worker[i], NULL);
	}
	
	fclose(log_fd);
	printf("Leaving main, all threads have exited.");

	return EXIT_SUCCESS;
}
