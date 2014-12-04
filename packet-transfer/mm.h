#ifndef __MM_H
#define __MM_H

#include <sys/time.h>

#define INTERVAL 0
#define INTERVAL_USEC 500000
#define CHUNK_SIZE 64
#define NUM_CHUNKS 1000000

typedef struct {
	int m_chunkSize;
	int m_numChunks;
	int* m_used;
	void** m_data;
} mm_t;

double comp_time(struct timeval time_s, struct timeval time_e);
int mm_init(mm_t *mm, int num_chunks, int chunk_size);
void *mm_get(mm_t *mm);
void mm_put(mm_t *mm, void *chunk);
void mm_release(mm_t *mm);

#endif
