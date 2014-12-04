
#include <time.h>
#include "packet.h"

static int pkt_cnt = 0;     	/* how many packets have been sent for current message */
static int pkt_total = 1;   	/* how many packets to send send for the message */
static int msqid = -1; 		/* id of the message queue */
static int receiver_pid; 	/* pid of the receiver */
static int which = -1;		/* which packet the current packet is */
static int how_many;		/* how many packets for current message */
static int num_of_packets_sent = 0; /* number of packets sent for current message */
static int is_packet_sent[MAX_PACKETS]; /* array of packet indices; has/hasn't ben sent */

/*
   Returns the packet for the current message. The packet is selected randomly.
   The number of packets for the current message are decided randomly.
   Each packet has a how_many field which denotes the number of packets in the current message.
   Each packet is string of 3 characters. All 3 characters for given packet are the same.
   For example, the message with 3 packets will be aaabbbccc. But these packets will be sent out order.
   So, message aaabbbccc can be sent as bbb -> aaa -> ccc
   */
static packet_t get_packet() {
  
  int i;

  packet_t pkt;

  if (num_of_packets_sent == 0) {
    how_many = rand() % MAX_PACKETS;
    if (how_many == 0) {
      how_many = 1;
    }
    printf("Number of packets in current message: %d\n", how_many);
    which = -1;
    for (i = 0; i < MAX_PACKETS; ++i) {
      is_packet_sent[i] = 0;
    }
  }
  which = rand() % how_many;
  if (is_packet_sent[which] == 1) {
    i = (which + 1) % how_many;
    while (i != which) {
      if (is_packet_sent[i] == 0) {
        which = i;
        break;
      }
      i = (i + 1) % how_many;
    } 

  }
  pkt.how_many = how_many;
  pkt.which = which;

  memset(pkt.data, 'a' + which, sizeof(data_t));

  is_packet_sent[which] = 1;
  num_of_packets_sent++;
  if (num_of_packets_sent == how_many) {
    num_of_packets_sent = 0;
  }

  return pkt;
}

static void packet_sender(int sig) {
  int sigCheck = 0; // Signaling error checker
  int read = 0; // BytesRead check for current packet
  char temp[PACKET_SIZE + 2]; // temp is just used for temporarily printing the packet.
  packet_t pkt; // Current packet to send
  pkt = get_packet(); // Set the current packet
  strcpy(temp, pkt.data); // Get current packet's data
  temp[3] = '\0';
  pkt_total = how_many;
  

  // Create a packet_queue_msg for the current packet.
  packet_queue_msg current_packet_msg;
  current_packet_msg.pkt = pkt;
  current_packet_msg.mtype = QUEUE_MSG_TYPE;

  // Send this packet_queue_msg to the receiver. 
  printf("Sending packet: %s\n", temp);
  read = msgsnd(msqid, &current_packet_msg, sizeof(packet_queue_msg), 0);
  if(msqid == -1) // Handle any error appropriately. msg send error
  {
	perror("Packet message sending error");
  }
  else
  {	
	sigCheck = kill(receiver_pid, SIGIO); // send SIGIO to the receiver if message sending was successful.

	if(sigCheck == -1) // Error handling - could not send SIGIO to receiver process
	{
		perror("Could not signal receiver");
	}
	pkt_cnt++;
  }

}

int main(int argc, char **argv) {

  if (argc != 2) {
    printf("Usage: packet_sender <num of messages to send>\n");
    exit(-1);
  } /* Input error checking */

  /* Declarations - START */
  int k = atoi(argv[1]); /* Number of messages  to send */
  int i; /* Packet indexer */
  int read = 0; /* Read checker */
  int timeCheck = 0; /* Alarm checker*/
  srand(time(NULL)); /* Seed for random number generator */
  pid_queue_msg rcv_pid_msg; /* Declare pid_queue msg for receiving pid from receiver */
  struct itimerval interval; /* Timer struct for alarm */
  struct sigaction act; /* Signal action for alarm */
  /* Declarations - END */         

  /* I. Set up a message queue for sending the packets to the receiver */
  msqid = msgget(key, IPC_CREAT|0666);
  printf("The message queue is: %d\n", msqid);
  if(msqid == -1) // Error Handling - Could not create the queue
  {
	perror("Failed to initialize the queue");
	return EXIT_FAILURE;
  }

  /* II. Read the receiver's pid from the queue*/
  printf("Waiting for receiver pid.\n");
  read = msgrcv(msqid, &rcv_pid_msg, sizeof(rcv_pid_msg), PID_MSG_TYPE, 0);
  if(read == -1) // Error Handling - Could not read from the queue
  {
	perror("Could not receive receiver's PID");
	return EXIT_FAILURE;
  }
  receiver_pid = rcv_pid_msg.pid;
  printf("Got pid : %d\n", receiver_pid);
 
  /* - set up alarm handler -- mask all signals within it */
  /* The alarm handler will get the packet and send the packet to the receiver. Check packet_sender();
   * Don't care about the old mask, and SIGALRM will be blocked for us anyway,
   * but we want to make sure act is properly initialized.
   */
   act.sa_handler = &packet_sender;
   sigaction(SIGALRM, &act, NULL);
   sigfillset(&act.sa_mask);

  /*  
   * - turn on alarm timer ...
   * use  INTERVAL and INTERVAL_USEC for sec and usec values
  */
  interval.it_value.tv_sec = INTERVAL;
  interval.it_value.tv_usec = INTERVAL_USEC; // Timer expires after 5 seconds
  interval.it_interval.tv_sec = INTERVAL;
  interval.it_interval.tv_usec = INTERVAL_USEC; // Timer expires every 5 seconds after that

  /* And the timer */
  timeCheck = setitimer(ITIMER_REAL, &interval, NULL);
  if(timeCheck == -1)
  {
	perror("Could not start timer!\n");
	return EXIT_FAILURE;
  }

  /* NOTE: the below code wont run now as you have not set the SIGALRM handler. Hence, 
     set up the SIGALRM handler and the timer first. */
  for (i = 1; i <= k; i++) {
    printf("==========================\n");
    printf("Sending Message: %d\n", i);
    while (pkt_cnt < pkt_total) {
      pause(); /* block until next packet is sent. SIGALRM will unblock and call the handler.*/
    }
    pkt_cnt = 0;
  }

  /* V. remove the queue */
  read = msgctl(msqid, IPC_RMID, NULL);
  if(read == -1) // Error handling - unable to disconnect queue
  {
	perror("Failed to disconnect msg queue");
	return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
