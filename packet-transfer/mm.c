#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mm.h"

/* Return usec */
double comp_time(struct timeval time_s, struct timeval time_e) {

  double elap = 0.0;

  if (time_e.tv_sec > time_s.tv_sec) {
    elap += (time_e.tv_sec - time_s.tv_sec - 1) * 1000000.0;
    elap += time_e.tv_usec + (1000000 - time_s.tv_usec);
  }
  else {
    elap = time_e.tv_usec - time_s.tv_usec;
  }
  return elap;
}

/* - Implement.  Return 0 for success, or -1 and set errno on fail. */
int mm_init(mm_t *mm, int hm, int sz) {
	// Initial declarations //
	void ** inp_data = calloc(hm, sz);
	int inp_used[hm];
	mm->m_numChunks = hm;
	mm->m_chunkSize = sz;
	mm->m_used = inp_used;
	mm->m_data = inp_data;
	int i = 0;

	// Set the data pointer array //
	for(i = 0; i < hm; i++)
	{		
		mm->m_data[i] = (void*)malloc(sz);
		mm->m_used[i] = 0;

		if(mm->m_data[i] <=0)
		{
			perror("Insufficient memory");
			return -1;
		}
	}
  	return 0;
}

void * mm_get(mm_t *mm) {

	int i = 0;
	int numCh; 
	numCh = mm->m_numChunks;
	
	// Scan the memory list for an unused block //
	for(i = 0; i < numCh; i++)
	{
		if(!mm->m_used[i]) // Chunk found?
		{
			mm->m_used[i] = 1; // Chunk is used			
			return mm->m_data[i];
		}
	}
	
    return NULL; // No available block //
}

/* - Implement */
void mm_put(mm_t *mm, void *chunk) {
	
	int j = 0;
	int numCh = 0;
	numCh = mm->m_numChunks;

	// Scan the memory list for the data to enable reuse of memory //
	for(j = 0; j < numCh; j++)
	{
		if(mm->m_data[j] == chunk) // Chunk found?
		{
			mm->m_used[j] = 0; // Chunk is now usable
		}
	}
}

/* - Implement */
void mm_release(mm_t *mm) {

	int k = 0;	
	int numCh = 0;
	numCh = mm->m_numChunks;	

	// Free all blocks in memory list // 
	for(k = 0; k < numCh; k++)
	{
		free(mm->m_data[k]);
	}	
}

