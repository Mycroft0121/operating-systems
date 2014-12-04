#include "wrapper.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>

#define MAX_TAB 100

/*
 * Name:		uri_entered_cb
 * Input arguments:'entry'-address bar where the url was entered
 *			 'data'-auxiliary data sent along with the event
 * Output arguments:void
 * Function:	When the user hits the enter after entering the url
 *			in the address bar, 'activate' event is generated
 *			for the Widget Entry, for which 'uri_entered_cb'
 *			callback is called. Controller-tab captures this event
 *			and sends the browsing request to the router(/parent)
 *			process.
 */
void uri_entered_cb(GtkWidget* entry, gpointer data)
{
	if(data == NULL) // Error Handling
	{	
		perror("Null data in uri_entered_cb function\n");		
		return;
	}

	browser_window* b_window = (browser_window*)data;
	comm_channel channel = b_window->channel;
	
	// Get the tab index where the URL is to be rendered
	int tab_index = query_tab_id_for_request(entry, data);

	if(tab_index < 0) // Error handling (Invalid tab)
	{
		perror("Invalid tab in uri_entered_cb function\n");
                return;
	}

	// Get the URL.
	char* uri = get_entered_uri(entry);
	if(uri == NULL || uri[0] != 'h' || uri[1] != 't' || uri[2] != 't' || uri[3] != 'p' || uri[4] != ':' || uri[5] != '/' || uri[6] != '/' ) // Error handling(Invalid URI)
	{
		perror("Invalid URI in uri_entered_cb function\n");
		return;
	}
	
	// Now you get the URI from the controller.
		
	// NEW_URI_ENTERED request declarations
	child_req_to_parent new_req;
	new_uri_req uri_req;
	child_request child_req;

	// NEW_URI_ENTERED request field assignment
	uri_req.render_in_tab = tab_index;
	strcpy(uri_req.uri, uri);
	child_req.uri_req = uri_req;
	new_req.type = NEW_URI_ENTERED;
	new_req.req = child_req;
	
	// Write req to controller end of pipe so router can read from pipe
	write(channel.child_to_parent_fd[1], &new_req, sizeof (child_req_to_parent));	
}

/*
 * Name:		new_tab_created_cb
 * Input arguments:	'button' - whose click generated this callback
 *			'data' - auxillary data passed along for handling
 *			this event.
 * Output arguments:    void
 * Function:		This is the callback function for the 'create_new_tab'
 *			event which is generated when the user clicks the '+'
 *			button in the controller-tab. The controller-tab
 *			redirects the request to the parent (/router) process
 *			which then creates a new child process for creating
 *			and managing this new tab.
 */ 
void new_tab_created_cb(GtkButton *button, gpointer data)
{
	if(data == NULL) // Error handling
	{
		perror("Null data in new_tab_created_cb function\n");
		return;
	}

 	int tab_index = ((browser_window*)data)->tab_index;

	if(tab_index < 0) // Error handling (Invalid tab)
	{
		perror("Invalid tab in new_tab_created_cb function\n");
                return;
	}
	
	//This channel has pipes to communicate with router. 
	comm_channel channel = ((browser_window*)data)->channel;

	// Users press + button on the control window. (Create new tab)

	// CREATE_TAB request declarations
	child_req_to_parent new_req;
	create_new_tab_req newtab;
	child_request child_req;

	// CREATE_TAB request field assignment
	newtab.tab_index = tab_index;	
	child_req.new_tab_req = newtab;
	new_req.type = CREATE_TAB;
	new_req.req = child_req;	
	
	//write req to controller end of pipe so router can read from pipe
	write(channel.child_to_parent_fd[1], &new_req, sizeof (child_req_to_parent));
}

/*
 * Name:                run_control
 * Input arguments:     'comm_channel': Includes pipes to communctaion with Router process
 * Output arguments:    void
 * Function:            This function will make a CONTROLLER window and be blocked until the program terminate.
 */
int run_control(comm_channel comm)
{
	browser_window * b_window = NULL;

	//Create controler process
	create_browser(CONTROLLER_TAB, 0, G_CALLBACK(new_tab_created_cb), G_CALLBACK(uri_entered_cb), &b_window, comm);

	//go into infinite loop.
	show_browser();
	return 0;
}

/*
* Name:                 run_url_browser
* Input arguments:      'nTabIndex': URL-RENDERING tab index
                        'comm_channel': Includes pipes to communctaion with Router process
* Output arguments:     void
* Function:             This function will make a URL-RENDRERING tab Note.
*                       You need to use below functions to handle tab event. 
*                       1. process_all_gtk_events();
*                       2. process_single_gtk_event();
*                       3. render_web_page_in_tab(uri, b_window);
*                       For more details please see Appendix B.
*/

int run_url_browser(int nTabIndex, comm_channel comm)
{
	browser_window * b_window = NULL;
	
	// Create controler window
	create_browser(URL_RENDERING_TAB, nTabIndex, G_CALLBACK(new_tab_created_cb), G_CALLBACK(uri_entered_cb), &b_window, comm);

	child_req_to_parent request;
	char uri_req[512];	
	int r;

	while (1) // Need to communicate with Router process here. Will receieve requests for processing.
	{
		process_single_gtk_event();
		usleep(1000);
		
		// Insert code here!!
		
		// 1. process_all_gtk_events();
		// 2. process_single_gtk_event();
        	// 3. render_web_page_in_tab(uri, b_window);

		// Read the request from the comm_channel

		r = read(comm.parent_to_child_fd[0], &request, sizeof(child_req_to_parent));
		
	    /*
	     * Handle each type of message (few mentioned below)
	     */
		
		if(r == -1)
		{
			//do nothing
		}
		else
		{
			if(request.type == NEW_URI_ENTERED) //  NEW_URI_ENTERED: render_web_page_in_tab(uri, b_window);
			{
				strcpy(uri_req, request.req.uri_req.uri);
				render_web_page_in_tab(uri_req, b_window);
			}
			//  TAB_KILLED: process all gtk events();
			if(request.type == TAB_KILLED)
			{
				process_all_gtk_events();
				break;
			}
		}
	}

	return 0;
}

int main()
{
	comm_channel comm[MAX_TAB];	//Router <-> URL-RENDERING process communication channel array
	//Controller <-> Router communication channel is now comm[0]
	
	pid_t contPid, tabspid[MAX_TAB]; //for forking the CONTROLLER and URL_RENDERING processes
	
	int controllerDead = 1;
	int r,n;				//used for checking if the read returns anything useful
	int tab_count = 0;  //total # of tabs
	int tab_index = 0;  //current available index

	//pipes for bi-directions communication with the CONTROLLER process
	pipe(comm[0].parent_to_child_fd);
	pipe(comm[0].child_to_parent_fd);
	
	//make these pipes non-blocking
	int flags;
	flags = fcntl(comm[0].parent_to_child_fd[0], F_GETFL, 0);
	fcntl(comm[0].parent_to_child_fd[0] , F_SETFL, flags | O_NONBLOCK);
	flags = fcntl(comm[0].child_to_parent_fd[0], F_GETFL, 0);
	fcntl(comm[0].child_to_parent_fd[0] , F_SETFL, flags | O_NONBLOCK);
	
	//fork for the CONTROLLER
	contPid = fork();
	controllerDead = 0;
	
	if(contPid == 0)
	{

		run_control(comm[0]);

		//uses callback functions to respond to reodr.quests from the ROUTER
		process_all_gtk_events();

		//exits with the return status of 0 for success when the user closed the CONTROLLER window
		exit(0);
	}
	
	tab_count++;
	tab_index++; //tab 0 is the controller

	while(!controllerDead) // while the controller is still running
	{
		child_req_to_parent request;
		
		//do a non blocking read to check for messages from the children processes
		for(n = 0; n < tab_index; n++)
		{
			r = read(comm[n].child_to_parent_fd[0], &request, sizeof(child_req_to_parent));
			
			if(r == -1)
			{
				//just keep going, ignore the error message
			}
			else		//when the read returns some data
			{
		
				if(request.type == CREATE_TAB) // Create-tab req read
				{
					//creates non blocking pipes for Router <-> URL-RENDERING process communication
					if(tab_index >= MAX_TAB)
					{
						perror("Too many tabs");
						break;
					}
					// Set the pipe
					pipe(comm[tab_index].parent_to_child_fd);
					pipe(comm[tab_index].child_to_parent_fd);
			
					// Set flags for non-blocking pipe reads
					flags = fcntl(comm[tab_index].parent_to_child_fd[0], F_GETFL, 0);
					fcntl(comm[tab_index].parent_to_child_fd[0] , F_SETFL, flags | O_NONBLOCK);
					flags = fcntl(comm[tab_index].child_to_parent_fd[0], F_GETFL, 0);
					fcntl(comm[tab_index].child_to_parent_fd[0] , F_SETFL, flags | O_NONBLOCK);
				

					//the ROUTER forks() a URL_RENDERING process (this happens when the user clicks the 'new tab' button in the controller window)
					tabspid[tab_index] = fork();
					if(tabspid[tab_index] == -1)
					{
						perror("Fork failed :(");
						//fork failed
					}
					if(tabspid[tab_index] == 0) // Execute the browser
					{
						run_url_browser(tab_index, comm[tab_index]);
						exit(0);
					}
					// Increment the current tab and the max count
					tab_index++;
					tab_count++;
				}
				else if(request.type == NEW_URI_ENTERED) // URL rendering
				{
					int uri_index = request.req.uri_req.render_in_tab;
					
					// Request declarations
					child_req_to_parent new_uri_child;
					child_request c_req;
					new_uri_req new_uri;
					// Request field assignment
					new_uri.render_in_tab = uri_index;
					strcpy(new_uri.uri, request.req.uri_req.uri);
					c_req.uri_req = new_uri;
					new_uri_child.req = c_req;
					new_uri_child.type = NEW_URI_ENTERED;
				
					if(uri_index >= tab_index || uri_index < 0) //Error handling (is tab alive?)
					{
						perror("No such tab!");
					}
					else
					{
						write(comm[uri_index].parent_to_child_fd[1], &new_uri_child, sizeof(child_req_to_parent)); // Write request to URL proc
					}
				}
				else if(request.type == TAB_KILLED)
				{

					//ROUTER closes the file descriptors of the correspoding pipe that was between the ROUTER and the tab that is now closed
					int killed_index = request.req.killed_req.tab_index;

					if( killed_index == 0) // Controller is killed
					{
						// Request declaration
						child_req_to_parent new_req;
						tab_killed_req killed_req;
						child_request child_req;
						// Request field assignment
						child_req.killed_req = killed_req;
						new_req.type = TAB_KILLED;
						new_req.req = child_req;
						
						int j;
						for(j = 0;j < tab_index; j++ ) // Handle children
						{
							new_req.req.killed_req.tab_index = j;
							write(comm[j].parent_to_child_fd[1], &new_req, sizeof(child_req_to_parent)); // Send tab-kill req to all children
							kill(tabspid[j], SIGKILL); //Euthanize children
						}
						controllerDead = 1;
					}else{
						//
					}
					
					//Close the pipe & decrement tab count
					close(comm[killed_index].child_to_parent_fd[0]);
					close(comm[killed_index].parent_to_child_fd[0]);
					close(comm[killed_index].child_to_parent_fd[1]);
					close(comm[killed_index].parent_to_child_fd[1]);
					
					kill(tabspid[killed_index], SIGKILL);
					tab_count--;
					
					
					if( tab_count == 0 )
					{
						return 0;
					}
				}
			}
		}
		//use usleep between reads so we dont slow down the CPU
		usleep(10000); 

	}
	//ROUTER exits and returns 0 for success (when CONTROLLER and all URL_REDERING processes are finished
	kill(0, SIGKILL);
	exit(0);
}
