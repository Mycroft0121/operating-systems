
#include "packet.h"

int msqid = -1;

static message_t message;   /* current message structure */
static mm_t mm;             /* memory manager will allocate memory for packets */
static int pkt_cnt = 0;     /* how many packets have arrived for current message */
static int pkt_total = 1;   /* how many packets to be received for the message */
struct sigaction rcv_act; // Signal action struct for the receiver //

/*
   Handles the incoming packet. 
   Store the packet in a chunk from memory manager.
   The packets for given message will come out of order. 
   Hence you need to take care to store and assemble it correctly.
   Example, message "aaabbb" can come as bbb -> aaa, hence, you need to assemble it
   as aaabbb.
   Hint: "which" field in the packet will be useful.
 */
static void packet_handler(int sig) {

  /* Declarations - START */
  packet_t pkt; /* Current packet to be handled*/ 
  void *chunk; /* Current memory chunk retrieved from memory */
  packet_queue_msg nxt_pkt_msg; /* Current queue message to be read */ 
  int read = 0; /* Message queue read checker */
  char temp[PACKET_SIZE + 2]; /* Used for printing the packet, temporarily */
  packet_t tst_pkt;
  /* Declarations - END */
  
  read = msgrcv(msqid, &nxt_pkt_msg, sizeof(nxt_pkt_msg), QUEUE_MSG_TYPE, 0); // get the "packet_queue_msg" from the queue.
  if(read == -1) // Error handling - could not handle packet message
  {
	perror("Could not handle packet message");
  }
  pkt = nxt_pkt_msg.pkt; // Retrieve packet from message received
  pkt_total = pkt.how_many; // Update the packet total
  strcpy(temp, pkt.data); // Copy to temp
  printf("Packet is: %s\n", temp);
  
  // store pkt in the memory from memory manager
  chunk = mm_get(&mm); // Get chunk from memory manager
  chunk = memcpy(chunk, &pkt, sizeof(packet_t)); // Copy packet to chunk; "stores" the packet in memory
  if(chunk == NULL) /* Error checking - cant copy packet to chunk */
  {
	perror("Could not copy packet to memory");
  }
  
  tst_pkt = *((packet_t*)chunk); // Checking
  strcpy(temp, tst_pkt.data);
  printf("Chunk in memory is: %s\n", temp);
  
  message.data[pkt_cnt] = memcpy(chunk, &pkt, sizeof(packet_t));
  message.num_packets++;
  if(message.data[pkt_cnt] == NULL) /* Error checking - cant copy packet to chunk */
  {
	perror("Could not copy packet to memory");
  }
 
  pkt_cnt++;
}

/*
 * Create message from packets.
 * Return a pointer to the message on success, or NULL
 */
static char *assemble_message() {
	char *msg, *temp;
	int i,j;
	packet_t tmp_pkt;
	int msg_len = message.num_packets * sizeof(data_t);

	/* Allocate msg and assemble packets into it */
	temp = malloc(4*sizeof(char));
	msg = (char*)malloc(msg_len);
	
  	j = 0;
	while( j < pkt_total){
		for(i = 0; i < message.num_packets; i++){
			tmp_pkt = *((packet_t*)message.data[i]);
			if(tmp_pkt.which == j){
			  	strcpy(temp,tmp_pkt.data);
			  	strncat(msg,temp,3);
			  	j++;
			}
		}
	}

	/* reset these for next message */
	pkt_total = 1;
	pkt_cnt = 0;
	message.num_packets = 0;

	return msg;
}

int main(int argc, char **argv) {

  if (argc != 2) {
    printf("Usage: packet_receiver <num of messages to receive>\n");
    exit(-1);
  } // Error checking - input

  /* Declarations - START*/
  int k = atoi(argv[1]); /* no of messages you will get from the sender */
  int i; /* Packet indexer */
  int read = 0; /* Bytes read int */
  char *msg; /* Message buffer */
  pid_t rcv_pid; /* Receiver pid */ 
  rcv_pid = getpid(); /* Get PID */
  pid_queue_msg rcv_pid_msg; /* Declare new pid_queue_msg */
  rcv_pid_msg.pid = rcv_pid; /* Set PID of pid_queue_msg */
  rcv_pid_msg.mtype = PID_MSG_TYPE; /* Set the pid_queue_msg type */
  message.num_packets = 0; /* Number of packets in current message */
  /* Declarations - END */

  /* - init memory manager for NUM_CHUNKS chunks of size CHUNK_SIZE each */
  mm_init(&mm, NUM_CHUNKS, CHUNK_SIZE);

  /* I. Get the message queue set up by the sender. Use the same key as used by the sender */
  msqid = msgget(key, 0666);
  if(msqid == -1) /* Error checking - Unable to initialize queue */
  {
	perror("Could not initialize the msg queue");
	return EXIT_FAILURE;
  }

  /* II. Send the pid to sender via the queue */
  read = msgsnd(msqid, &rcv_pid_msg, sizeof(pid_queue_msg), 0);
  printf("Sending pid: %d\n", rcv_pid);
  if(read == -1) /* Error Handling - unable to send receiver pid */
  {
	perror("Failed to send receiver pid");
	return EXIT_FAILURE;
  }

  /* III. Set up SIGIO handler that reads an incoming packet from the queue. */
  sigfillset(&rcv_act.sa_mask); // Block other signals when the handler starts executing as reading may be interrupted
  rcv_act.sa_handler = packet_handler; // Set the signal handler
  sigaction(SIGIO, &rcv_act, NULL); // Handler operates on SIGIO signal

  for (i = 1; i <= k; i++) {
    while (pkt_cnt < pkt_total) {
      pause(); /* block until next packet */
    } /* While there are still packets for the current message ... */
  
    msg = assemble_message(); /* Assemble the current message string, only after all packets have arrived */
    if (msg == NULL) {
      perror("Failed to assemble message");
    }
    else {
      fprintf(stderr, "GOT IT: message=%s\n", msg);
      free(msg);
    }
  }

  /* V. Release memory manage and remove the queue */
  mm_release(&mm);
  read = msgctl(msqid, IPC_RMID, NULL);
  if(read == -1) // Error handling - unable to disconnect queue
  {
	perror("Failed to disconnect msg queue");
	return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}
