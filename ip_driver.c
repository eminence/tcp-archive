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

int main( int argc, char* argv[] ) {
	char netconf[256];
	int i = 0,nodenum, in;
	ip_node_t *node;
	char buf[ 256 ];
	int retval;
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
		int err;
		//printf( "\n0 : Read some data\n");
		//printf( "1 : Send some data\n");
		//printf( "2 : Get the status of a link\n");
		//printf( "3 : Toggle a link up or down\n");
		//printf( "Command: " );

		fflush(stdout);
		memset(buf,0,80);
		//err = read(STDIN_FILENO, buf, 80);
		//if( err <= 0 ) { printf("\n"); break; }
		buf[79] = 0;

		//sscanf(buf, "%d", &i);
		i = get_key();
		//printf("Got key: %d\n", i);
		switch( i ) {
			case 'r':
				show_route_table();
				break;
			case 't':
				show_tcp_table();	
				break;

			case '1':
				/* read */

				//retval = van_driver_recvfrom(node, data, 1000);
				//printf("Got data (%d): '%s'\n", retval, data);

				//printf( "van_driver_recvfrom returned: %d\n", retval );
				display_msg("Function Not Yet Implemented");
				break;

			case '2':
				/* write */
				in = get_number("Dest node:" );
				//scanf( "%d", &in );

				//printf( "Message: " );
				int len = get_text( "Message:", &buf, 256);
				//scanf( "%s", buf );
				retval = van_driver_sendto(node, buf, len, in);
				//printf("van_driver_sendto returned %d\n", retval);
				//display_msg("Function Not Yet Implemented");
				break;

			case '3':
				/* set link status */

				in = get_number("Interface:");
				if (get_if_state(node, in)) set_if_state(node, in, 0);
				else set_if_state(node, in, 1);
				break;

			case KEY_UP:
			case KEY_DOWN:
				scroll_logwin(i);
				break;
		}
	}

	free(data);

	van_driver_destory(node);

	return 0;
}

