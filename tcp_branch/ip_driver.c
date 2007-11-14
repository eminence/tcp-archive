/*
 * Driver program for the IP program.
 * 
 * Name:
 * Acct:
 * Date:
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "van_driver.h"
#include "fancy_display.h"
#include "tcp.h"

int main( int argc, char* argv[] ) {
	char netconf[256];
	int i = 0,nodenum, in;
	ip_node_t *node;
	char buf[ 256 ];
	char *data = malloc(1000);

	if( argc != 3 ) {
		printf("usage: %s node_num netconfig\n",argv[0]);
		return -1;
	}
	nodenum = atoi(argv[1]);

	strncpy(netconf,argv[2],256);

	printf("Starting up...\n");
	printf("Init display driver...\n");

	init_display(USE_CURSES);

	nlog(MSG_LOG, "init", "Display driver started");
	nlog(MSG_LOG, "init", "Starting van driver...");
	node = van_driver_init(netconf, nodenum);


	while( i != 'q' ) {
		int retval;
		//printf( "\n0 : Read some data\n");
		//printf( "1 : Send some data\n");
		//printf( "2 : Get the status of a link\n");
		//printf( "3 : Toggle a link up or down\n");
		//printf( "Command: " );

		fflush(stdout);
		memset(buf,0,80);
		//err = read(STDIN_FILENO, buf, 80);
		//if( err <= 0 ) { printf("\n"); break; }
		buf[256] = 0;

		//sscanf(buf, "%d", &i);
		i = get_key();
		//printf("Got key: %d\n", i);
		switch( i ) {

			case 'h':
				display_msg("alpha\nbeta");
				break;

			case 'r':
				show_route_table();
				break;
			case 't':
				show_tcp_table();	
				break;

			case 's': /* new socket */
				retval = v_socket();

				display_msg("v_socket() returned %d", retval);
				break;

			case KEY_NPAGE:
			case KEY_PPAGE:
				scroll_logwin(i);
				break;

			case KEY_DOWN:
			case KEY_UP:
			case ' ':
				handle_tcp_menu_input(i);
				break;

			case '-':
				test_tcp_menu_update();
				break;

			case 'b':
				{
					int socket;
					uint16_t port;
					socket = get_fd_from_menu();
					if (socket == -1) {
						display_msg("Please select/create a socket first!"); break;
					} else {
						
						//node = get_number("enter this node number");
						port = get_number("Local port to bind to");
						nlog(MSG_LOG, "socket", "binding socket %d to local port %d", socket, port);
						retval = v_bind(socket, nodenum, port);
						display_msg("v_bind() returned %d", retval);
					}
				}
				break;
			case 'l':
				{
					int socket = get_fd_from_menu();
					if (socket == -1) {
						display_msg("Please select/create a socket first!"); break;
					} else {
						retval = v_listen(socket, 0);
						display_msg("v_listen() returned %d", retval);
					}

				}
				break;

			case 'c': /* connect! */
				{
					int socket, node;
					uint16_t port;
					socket = get_fd_from_menu();
					if (socket == -1) {
						display_msg("Please select/create a socket first!"); break;
					} else {
						node = get_number("node to connect to");
						port = get_number("port to connect to");
						nlog(MSG_LOG,"socket","connecting to %d on port %d with socket %d...", node, port, socket);
						retval = v_connect(socket, node, port);
						display_msg("v_connect() returned %d", retval);
					}
				}
				break;
      case 'x': /* close */
        {
					int socket;
					socket = get_fd_from_menu();
					if (socket == -1) {
						display_msg("Please select/create a socket first!"); break;
					} else {
						nlog(MSG_LOG,"socket","closing %d...", socket);
						retval = v_close(socket);
						display_msg("v_close() returned %d", retval);
          }
        }
        break;
			case 'a': /*accept*/
				{
					int socket = get_fd_from_menu();
					if (socket == -1) {
						display_msg("Please select/create a socket first!"); break;
					} else {
						display_msg("about to call v_accept().  Note: this will block\n Press any key to continue");
						retval = v_accept(socket);
						display_msg("v_accept() returned %d", retval);
					}

				}

				break;
			case '2':
				/* read */

				//retval = van_driver_recvfrom(node, data, 1000);
				//printf("Got data (%d): '%s'\n", retval, data);

				//printf( "van_driver_recvfrom returned: %d\n", retval );
				{
					int socket = get_fd_from_menu();
					if (socket == -1) {
						display_msg("Please select/create a socket first!"); break;
					} else {
						int len = get_number("Number of bytes to read:");
						char *data = malloc(len+1);
						retval = v_read(socket, data, len);

						data[retval] = '\0';

						display_msg("v_read() returned %d", retval);
						display_msg("The %d bytes returned from read are: %s", retval, data);
					}

				}


				display_msg("Function Not Yet Implemented");
				break;

			case '1':
				/* write */
				{
					int socket = get_fd_from_menu();
					if (socket == -1) {
						display_msg("Please select/create a socket first!"); break;
					} else {
						int len = get_text( "Message:", buf, 256);
						display_msg("You entered %d(%d) characters: %s", strlen(buf),len, buf);

						retval = v_write(socket,buf, len);
						display_msg("v_write() returned %d", retval);
					}

				}
				break;

				//retval = van_driver_sendto(node, buf, len, in, PROTO_DATA);
				//printf("van_driver_sendto returned %d\n", retval);
				//display_msg("Function Not Yet Implemented");

			case '3':
				/* set link status */

				in = get_number("Interface:");
				if (get_if_state(node, in)) set_if_state(node, in, 0);
				else set_if_state(node, in, 1);
				break;
		}
	}

	free(data);

	van_driver_destory(node);

	return 0;
}
