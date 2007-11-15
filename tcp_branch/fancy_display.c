#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "fancy_display.h"

#include "tcp.h"
#include "van_driver.h"
#include "tcpstate.h"

static curses_out_t output;

void update_link_line(int line, int state) {

	pthread_mutex_lock(&output.lock);

	wmove(output.link_win, 2+line,1);
	wclrtoeol(output.link_win);

	if (state == 0) {
		wattron(output.link_win, COLOR_PAIR(LINK_DOWN_COLOR));
		mvwprintw(output.link_win, 2+line, 1, "%d: Down",line);
		wattron(output.link_win, COLOR_PAIR(LINK_DOWN_COLOR));
	} else {
		wattron(output.link_win, COLOR_PAIR(LINK_UP_COLOR));
		mvwprintw(output.link_win, 2+line, 1, "%d: Up", line);
		wattron(output.link_win, COLOR_PAIR(LINK_UP_COLOR));
	}
	update_panels(); doupdate();

	pthread_mutex_unlock(&output.lock);
}

void scroll_logwin(int i) {
	pthread_mutex_lock(&output.lock);
	i++;

	//	switch (i) {
	//		case KEY_NPAGE:
	//			if (output.logwin_scroll >= 0) {
	//				wscrl(output.log_win,-1);
	//				output.logwin_scroll += 1;
	//				update_panels(); doupdate();
	//			}
	//			break;
	//		case KEY_PPAGE:
	//			wscrl(output.log_win,1);
	//			output.logwin_scroll += -1;
	//			update_panels(); doupdate();
	//			break;
	//	}
	pthread_mutex_unlock(&output.lock);
}

void nlog_set_menu(const char *msg, ...) {
	pthread_mutex_lock(&output.lock);

	va_list args;
	va_start(args, msg);
	wmove(output.menu_win,0,0);
	vwprintw(output.menu_win, msg, args);
	update_panels(); doupdate();
	va_end(args);

	pthread_mutex_unlock(&output.lock);
}



void display_msg(char *msg, ...) {
	pthread_mutex_lock(&output.lock);

	va_list args;
	//int l = strlen(msg);
	WINDOW *my_form_win;
	PANEL *my_form_pan;

	int c=0;
	int n = 0;
	while (msg[c] != 0) {
		if (msg[++c] == '\n') n++;
	}

	char tmp[1024];

	va_start(args,msg);

	vsnprintf(tmp, 1024, msg, args);
	int l = strlen(tmp);

	/* Create the window to be associated with the form */
	my_form_win = newwin(3+n, 2 + l, LINES/2 - (3/2), COLS/2 - ((l+2)/2));
	leaveok(my_form_win, FALSE);
	my_form_pan = new_panel(my_form_win);
	keypad(my_form_win, TRUE);
	wbkgd(my_form_win, COLOR_PAIR(DEFAULT_COLOR));

	/* Print a border around the main window and print a title */
	wmove(my_form_win, 1, 1);
	vwprintw(my_form_win,msg,args);
	//wprintw(my_form_win, 1, 0, cols + 4, "My Form", COLOR_PAIR(1));
	
	move(0,COLS);
	wmove(my_form_win,0,0);
	update_panels(); doupdate();

	box(my_form_win, 0, 0);
	/* Loop through to get user requests */
	pthread_mutex_unlock(&output.lock); //xhax
	wgetch(my_form_win);
	pthread_mutex_lock(&output.lock); //xhax

	/* Un post form and free the memory */
	del_panel(my_form_pan);
	delwin(my_form_win);
	update_panels(); doupdate();
	va_end(args);

	pthread_mutex_unlock(&output.lock);
}



int get_text(char *msg, char* return_, int len) {
	
	pthread_mutex_lock(&output.lock);
	
	FIELD *field[2];
	FORM  *my_form;
	int rows, cols,ch, num;
	int l = strlen(msg);
	WINDOW *my_form_win;
	PANEL *my_form_pan;
	char *buff = NULL;


	/* Initialize the fields */ /* height, width, toprow, leftcol, offscreen, buffs*/
	field[0] = new_field(6, 42, 1, 1, 0, 0);
	field[1] = NULL;

	/* Set field options */
	set_field_back(field[0], A_UNDERLINE);
	field_opts_off(field[0], O_AUTOSKIP); /* Don't go to next field when this */
	/* Field is filled up 		*/

	/* Create the form and post it */
	my_form = new_form(field);

	//set_field_type(field[0], TYPE_ALNUM,15);
	/* Calculate the area required for the form */
	scale_form(my_form, &rows, &cols);

	/* Create the window to be associated with the form */
	my_form_win = newwin(rows + 4, cols + 4, LINES/2 - (rows/2), COLS/2 - (cols/2));
	wbkgd(my_form_win, COLOR_PAIR(DEFAULT_COLOR));
	my_form_pan = new_panel(my_form_win);
	keypad(my_form_win, TRUE);

	/* Set main window and sub window */
	set_form_win(my_form, my_form_win);
	set_form_sub(my_form, derwin(my_form_win, rows, cols, 2, 2));

	/* Print a border around the main window and print a title */
	box(my_form_win, 0, 0);
	mvwprintw(my_form_win, 1,1,msg);
	//wprintw(my_form_win, 1, 0, cols + 4, "My Form", COLOR_PAIR(1));

	post_form(my_form);
	update_panels(); doupdate();

	/* Loop through to get user requests */


	pthread_mutex_unlock(&output.lock); //xhax
	ch = wgetch(my_form_win);
	pthread_mutex_lock(&output.lock); //xhax

	while(ch != '`')
	{	
		
		switch(ch)
		{
			case KEY_LEFT:
				form_driver(my_form, REQ_LEFT_CHAR);
				break;
			case KEY_RIGHT:
				form_driver(my_form, REQ_RIGHT_CHAR);
				break;
			case KEY_HOME:
				form_driver(my_form, REQ_BEG_LINE);
				break;
			case KEY_END:
				form_driver(my_form, REQ_END_LINE);
				break;
			case KEY_BACKSPACE:
				form_driver(my_form, REQ_DEL_PREV);
				//form_driver(my_form, REQ_VALIDATION);
				form_driver(my_form, REQ_END_LINE);
				break;
			default:
				/* If this is a normal character, it gets */
				/* Printed				  */	
				form_driver(my_form, ch);
				form_driver(my_form, REQ_VALIDATION);
				//form_driver(my_form, REQ_);
				break;
		}

		pthread_mutex_unlock(&output.lock); //xhax
		ch = wgetch(my_form_win);
		pthread_mutex_lock(&output.lock); //xhax

	}

	buff = field_buffer(field[0],0);
	assert(buff);
	//printf(buff);
	int i = 0;
	int start,end;
	for (i = 0; i < 252; i++) { /* strip leading space characters */
		if (buff[i] != ' ') { start = i; break; }	
	}
	for (i = 251; i >= 0; i--) { /* strip trailing space characters */
		if (buff[i] != ' '){ end = i+1; break; }	
	}
	memset(return_,0,len);
	memcpy(return_, buff+start, end-start);

	//nlog(MSG_LOG,"GETTEXT", "start=%d (buff[0]=%p=%p)  end=%d", start, buff[0],' ', end);

	/* Un post form and free the memory */
	unpost_form(my_form);
	free_form(my_form);
	free_field(field[0]);
	del_panel(my_form_pan);
	delwin(my_form_win);
	update_panels(); doupdate();

	pthread_mutex_unlock(&output.lock);
	return end-start;
}


void test_tcp_menu_update() {
	pthread_mutex_lock(&output.lock);

	ITEM *curitem = current_item(output.tcp_menu);
	tcp_socket_t *sock = (tcp_socket_t*) item_userptr(curitem);

	unpost_menu(output.tcp_menu);

	char *new_text = malloc(128);
	sprintf(new_text, "State: %s lport:%d rport:%d peer:%d seqnum:%d acknum:%d",
			tcpm_strstate(tcpm_state(sock->machine)),
			sock->local_port,
			sock->remote_port,
			sock->remote_node,
			sock->seq_num,
			sock->ack_num
			);

	free((void*)curitem->description.str);
	curitem->description.str=new_text;
	curitem->description.length=strlen(new_text);

	post_menu(output.tcp_menu);

	redrawwin(output.menu_win);
	update_panels(); doupdate();

	pthread_mutex_unlock(&output.lock);

}

void tcp_table_new(ip_node_t *node, int fd) {

	pthread_mutex_lock(&output.lock);

	output.tcp_menu_num_items++;
	int i, retval;
	ITEM **new_items;

	// new memory for our new updated list of items
	unpost_menu(output.tcp_menu);
	new_items = (ITEM**)calloc(output.tcp_menu_num_items+1,sizeof(ITEM*));
	for (i = 0; i < output.tcp_menu_num_items - 1; ++i) {
		/* memcpy old items into new structure */
		new_items[i] = output.tcp_items[i];
		//memcpy(new_items[i], output.tcp_items[i], sizeof(ITEM*));
		//new_items[i] = new_item(item_name(output.tcp_items[i]), item_description(output.tcp_items[i]));
		//free_item(output.tcp_items[i]);
	}
	char *txt = malloc(8);
	memset(txt,0,8);
	sprintf(txt,"fd:%d",fd);
	char *desc = malloc(256); /* this will be free()'d when we update the socket desc */
	strcpy(desc,"This is a new socket                                                                                  ");
	new_items[output.tcp_menu_num_items-1] = new_item(txt, desc);
	set_item_userptr(new_items[output.tcp_menu_num_items - 1],(void*)node->socket_table[fd]);

	if ((retval = set_menu_items(output.tcp_menu, new_items)) != E_OK) {
			nlog(MSG_ERROR,"tcp_table_new", "Can't update the tcp table with this new socket");
	}
	//free (output.tcp_items);
	output.tcp_items = new_items;
	post_menu(output.tcp_menu);
	update_panels(); doupdate();

	pthread_mutex_unlock(&output.lock);
}

void update_tcp_table(tcp_socket_t *sock) {
	assert(sock);

	pthread_mutex_lock(&output.lock);


	// find the menu item associtated with this socket:
	int i = 0;
	while (output.tcp_items[i] != NULL) {
		if (item_userptr(output.tcp_items[i]) == (void*)sock) {
			// update!

			char *new_text = malloc(128);

			sprintf(new_text, "State: %s lport:%d rport:%d peer:%d seqnum:%d acknum:%d",
					tcpm_strstate(tcpm_state(sock->machine)),
					sock->local_port,
					sock->remote_port,
					sock->remote_node,
					sock->send_next,
					sock->recv_next
					);

			free((void*)output.tcp_items[i]->description.str);
			output.tcp_items[i]->description.str=new_text;
			output.tcp_items[i]->description.length=strlen(new_text);

			unpost_menu(output.tcp_menu);
			post_menu(output.tcp_menu);

			redrawwin(output.menu_win);
			update_panels(); doupdate();
			break;
		}
		i++;

	}

	pthread_mutex_unlock(&output.lock);

}


int get_number(char *msg) {
	
	pthread_mutex_lock(&output.lock);

	FIELD *field[2];
	FORM  *my_form;
	int rows, cols,ch, num;
	int l = strlen(msg);
	WINDOW *my_form_win;
	PANEL *my_form_pan;
	char *buff = NULL;

	/* Initialize the fields */ /* height, width, toprow, leftcol, offscreen, buffs*/
	field[0] = new_field(1, 5, 1, 1, 0, 0);
	field[1] = NULL;

	/* Set field options */
	set_field_back(field[0], A_UNDERLINE);
	field_opts_off(field[0], O_AUTOSKIP); /* Don't go to next field when this */
	/* Field is filled up 		*/

	/* Create the form and post it */
	my_form = new_form(field);

	set_field_type(field[0], TYPE_INTEGER);
	/* Calculate the area required for the form */
	scale_form(my_form, &rows, &cols);

	/* Create the window to be associated with the form */
	my_form_win = newwin(rows + 4, l + 2, LINES/2 - (rows/2), COLS/2 - (cols/2));
	wbkgd(my_form_win, COLOR_PAIR(DEFAULT_COLOR));
	my_form_pan = new_panel(my_form_win);
	keypad(my_form_win, TRUE);

	/* Set main window and sub window */
	set_form_win(my_form, my_form_win);
	set_form_sub(my_form, derwin(my_form_win, rows, cols, 2, 2));

	/* Print a border around the main window and print a title */
	box(my_form_win, 0, 0);
	mvwprintw(my_form_win, 1,1,msg);
	//wprintw(my_form_win, 1, 0, cols + 4, "My Form", COLOR_PAIR(1));

	post_form(my_form);
	update_panels(); doupdate();


	pthread_mutex_unlock(&output.lock); //xhax
	ch = wgetch(my_form_win);
	pthread_mutex_lock(&output.lock); //xhax

	/* Loop through to get user requests */
	while(ch != '`')
	{	switch(ch)
		{
			case KEY_LEFT:
				form_driver(my_form, REQ_LEFT_CHAR);
				break;
			case KEY_RIGHT:
				form_driver(my_form, REQ_RIGHT_CHAR);
				break;
			case KEY_HOME:
				form_driver(my_form, REQ_BEG_LINE);
				break;
			case KEY_END:
				form_driver(my_form, REQ_END_LINE);
				break;
			case KEY_BACKSPACE:
				form_driver(my_form, REQ_DEL_PREV);
				form_driver(my_form, REQ_VALIDATION);
				form_driver(my_form, REQ_END_LINE);
				break;
			default:
				/* If this is a normal character, it gets */
				/* Printed				  */	
				form_driver(my_form, ch);
				form_driver(my_form, REQ_VALIDATION);
				form_driver(my_form, REQ_END_LINE);
				break;
		}
		
		pthread_mutex_unlock(&output.lock); //xhax
		ch = wgetch(my_form_win);
		pthread_mutex_lock(&output.lock); //xhax
	}

	buff = field_buffer(field[0],0);
	assert(buff);
	//printf(buff);
	sscanf(buff,"%d", &num);

	/* Un post form and free the memory */
	unpost_form(my_form);
	free_form(my_form);
	free_field(field[0]);
	del_panel(my_form_pan);
	delwin(my_form_win);
	update_panels(); doupdate();

	pthread_mutex_unlock(&output.lock);

	return num;
}

void switch_to_tab(int t) {

	pthread_mutex_lock(&output.lock);

	unsigned int max_size = 0;
		int total_tabs = 0;
		while (output.tabs[total_tabs] != NULL) {
			if (strlen(output.tabs[total_tabs]) > max_size) {
				max_size = strlen(output.tabs[total_tabs]);
			}
			total_tabs++;
		}

		int tab_width = max_size + 2;

		//nlog(MSG_LOG,"tabs", "There are %d total tabs, the longest one is %d", total_tabs, max_size);

		// outline
		mvwvline(output.tab_win, 1,0,0,3);
		mvwhline(output.tab_win, 2,0,0,COLS-12);
		mvwaddch(output.tab_win, 2,0,ACS_LTEE);
		// stupid tab drawing crap
		mvwhline(output.tab_win, 0, 0, 0, 11);
		mvwaddch(output.tab_win, 0,0, ACS_ULCORNER);
		mvwaddch(output.tab_win, 0,5, ACS_TTEE);
		mvwaddch(output.tab_win, 2,5, ACS_BTEE);
		mvwaddch(output.tab_win, 0,11, ACS_URCORNER);
		mvwaddch(output.tab_win, 2,11, ACS_BTEE);

		mvwaddch(output.tab_win, 1, 5, ACS_VLINE);
		mvwaddch(output.tab_win, 1, 11, ACS_VLINE);

		
		/*wattron(output.tab_win, A_BOLD);
		mvwprintw(output.tab_win, 1,1," IP ");
		wattroff(output.tab_win, A_BOLD);
		mvwprintw(output.tab_win, 1,6," TCP ");*/
		int i;
		for (i = 0; i < total_tabs; i++) {
			
			if (i == t) wattron(output.tab_win, A_BOLD | A_UNDERLINE);
			mvwprintw(output.tab_win, 1,1+(i*tab_width)," %s ",output.tabs[i]);
			if (i == t) wattroff(output.tab_win, A_BOLD | A_UNDERLINE);

		}

		output.toptab = t;

	pthread_mutex_unlock(&output.lock);
}

int init_display(int use_curses) {

	pthread_mutexattr_init(&output.lock_attr);
	pthread_mutexattr_settype(&output.lock_attr, PTHREAD_MUTEX_RECURSIVE_NP);

	pthread_mutex_init(&output.lock, &output.lock_attr);
	pthread_mutex_lock(&output.lock);

	output.use_curses = use_curses;
	output.tabs[0] = "IP";
	output.tabs[1] = "TCP";
	output.tabs[2] = NULL;


	if (use_curses) {
		initscr();
		cbreak();
		noecho();
		curs_set(0);
		leaveok(stdscr, FALSE);
		keypad(stdscr, TRUE);

		output.logwin_scroll = 0;

		output.log_win = newwin(2*LINES/3, COLS, LINES/3, 0);
		mvwhline(output.log_win,0,0,0,COLS); //box(output.log_win, 0, 0);
		mvwaddch(output.log_win,0,0,ACS_LLCORNER);
		output.log_pan = new_panel(output.log_win);
		scrollok(output.log_win,1);

		nlog(MSG_LOG, "info","ncurses interface started up (%s)","test");

		if (has_colors() == FALSE) {
			nlog(MSG_LOG, "info","No color support on this terminal");
		} else {
			nlog(MSG_LOG, "info","Using colors");
			start_color();
			init_pair(DEFAULT_COLOR, COLOR_WHITE, COLOR_BLACK);
			init_pair(MSG_LOG_COLOR, COLOR_CYAN, COLOR_BLACK);
			init_pair(MENU_COLOR, COLOR_WHITE, COLOR_BLUE);
			init_pair(MSG_ERROR_COLOR, COLOR_RED, COLOR_BLACK);
			init_pair(MSG_WARNING_COLOR, COLOR_YELLOW, COLOR_BLACK);
			init_pair(LINK_DOWN_COLOR, COLOR_RED, COLOR_BLACK);
			init_pair(LINK_UP_COLOR, COLOR_GREEN, COLOR_BLACK);
			init_pair(IMPORTANT_COLOR, COLOR_BLACK, COLOR_YELLOW);

		}

		wbkgd(stdscr,COLOR_PAIR(DEFAULT_COLOR));
		redrawwin(stdscr);
											/*height, width, starty, startx*/
		output.menu_win = newwin(1, COLS, 0,0);
		output.menu_pan = new_panel(output.menu_win);
		wbkgd(output.menu_win, COLOR_PAIR(MENU_COLOR));
		wattron(output.menu_win, COLOR_PAIR(MENU_COLOR));

		output.link_win = newwin(LINES/3, 12, 1, COLS-12);
		output.link_pan = new_panel(output.link_win);

		wbkgd(output.link_win, COLOR_PAIR(DEFAULT_COLOR));
		wbkgd(output.log_win, COLOR_PAIR(DEFAULT_COLOR));
		redrawwin(output.log_win);
		redrawwin(output.link_win);
		//mvwvline(output.link_win,0,0,0,LINES/2); /* y, x, ch, n */
		//mvwhline(output.link_win,LINES/2-1,0,0,10);
		wborder(output.link_win,0,' ',' ',0,ACS_VLINE,' ',ACS_BTEE,' ');
		mvwprintw(output.link_win, 0,1,"Link State\n");
		wmove(output.link_win,0,0);

		output.tab_win = newwin(LINES/3-1, COLS-12, 1,0);
		output.tab_pan = new_panel(output.tab_win);

		mvwvline(output.tab_win,0,0,0,LINES/3 -1);
		switch_to_tab(1);

											/*height, width, starty, start */
		output.rtable_win = newwin(LINES/3-1-3, COLS-12-1, 4,1);
		output.rtable_pan = new_panel(output.rtable_win);
		leaveok(output.rtable_win, FALSE);
		//mvwvline(output.rtable_win,0,0,0,LINES/2 -1-3);
		mvwprintw(output.rtable_win,0,1,"rtable window");

		output.tcp_win = newwin(LINES/3-1-3, COLS-12-1, 4,1);
		output.tcp_pan = new_panel(output.tcp_win);
		//mvwvline(output.tcp_win,0,0,0,LINES/2 -1-3);
		mvwprintw(output.tcp_win,0,1,"tcp window");


		// test menu stuff:

		char *choices[] = {
			"Choice 1",
			"Choice 2",
			"Choice 3",
			"Exit",
			(char*)NULL,
		};

		int i;

		output.tcp_menu_num_items=0;

		output.tcp_items = (ITEM**)calloc(output.tcp_menu_num_items+1,sizeof(ITEM*));
		for (i = 0; i < output.tcp_menu_num_items; ++i) {
			output.tcp_items[i] = new_item(choices[i], "TCP info here... ... ...");
		}

		output.tcp_menu = new_menu((ITEM **)output.tcp_items);

		/* Create the window to be associated with the menu */
		keypad(output.tcp_win ,TRUE);

		/* Set main window and sub window */
		set_menu_win(output.tcp_menu, output.tcp_win);
		//set_menu_sub(tcp_menu, derwin(tcp_win, 6, 38, 3, 1));
		set_menu_format(output.tcp_menu, 5, 1);

		/* Set menu mark to the string " * " */
		set_menu_mark(output.tcp_menu, " > ");

		post_menu(output.tcp_menu);

		refresh();
		update_panels(); doupdate();

	} else {
			
	}

	pthread_mutex_unlock(&output.lock);

	return 0;
}

int get_fd_from_menu() {
	pthread_mutex_lock(&output.lock);
	tcp_socket_t *sock = (tcp_socket_t*) item_userptr(current_item(output.tcp_menu));
	pthread_mutex_unlock(&output.lock);
	if (sock == NULL) return -1; else return sock->fd;
}

void handle_tcp_menu_input(int c) {
		
	pthread_mutex_lock(&output.lock);

	if (output.toptab != 1) {
		pthread_mutex_unlock(&output.lock);
		return; /* ignore if we're not on the tcp tab */
	}

	switch (c) {
		case KEY_DOWN:
			menu_driver(output.tcp_menu, REQ_DOWN_ITEM);
			break;
		case KEY_UP:
			menu_driver(output.tcp_menu, REQ_UP_ITEM);
			break;
		case ' ':
			{
				tcp_socket_t *sock = (tcp_socket_t*) item_userptr(current_item(output.tcp_menu));
				if (sock != NULL) {
					display_msg("You just clicked sock %d", sock->fd);
				}
			}
			break;
	}
	update_panels(); doupdate();

	pthread_mutex_unlock(&output.lock);

	return;
}

void show_route_table() {
	pthread_mutex_lock(&output.lock);

	top_panel(output.rtable_pan);
	switch_to_tab(0);
	update_panels(); doupdate();

	pthread_mutex_unlock(&output.lock);
}

void show_tcp_table() {
	pthread_mutex_lock(&output.lock);

	top_panel(output.tcp_pan);
	switch_to_tab(1);
	update_panels(); doupdate();

	pthread_mutex_unlock(&output.lock);
}

void clear_rtable_display() {
	/*
	int x = 1;
	int y = 3;
	for (y = 3; y <LINES/2-1; y++) {
		for (x = 1; x < COLS-13; x++) {
			mvwaddch(output.rtable_win, y, x, ' ');
		}
	}
	*/
	pthread_mutex_lock(&output.lock);

	werase(output.rtable_win);
	wmove(output.rtable_win,0 ,1);

	update_panels(); doupdate();

	pthread_mutex_unlock(&output.lock);
}

void rtable_print( char *text, ...) {
	pthread_mutex_lock(&output.lock);

	va_list args;
	va_start(args, text);
	vwprintw(output.rtable_win, text, args);
	wprintw(output.rtable_win,"\n");
	update_panels(); doupdate();
	va_end(args);

	pthread_mutex_unlock(&output.lock);
}

int get_key() {
	int c;

	//pthread_mutex_lock(&output.lock); //xhax
	c = wgetch(stdscr);
	//pthread_mutex_unlock(&output.lock); //xhax

	return c;
}


void nlog_s(const char *wfile, int wline,msg_type msg, const char *slug, char *text, ...) {

	pthread_mutex_lock(&output.lock);
	
	int c = output.use_curses;
	WINDOW *log = output.log_win;
	va_list args;
	va_start(args, text);




	if (c) {
		int color;
		if (msg == MSG_LOG) color = COLOR_PAIR(MSG_LOG_COLOR);
		if (msg == MSG_WARNING) color = COLOR_PAIR(MSG_WARNING_COLOR);
		if (msg == MSG_ERROR) color = COLOR_PAIR(MSG_ERROR_COLOR);
		if (msg == MSG_XXX) color = COLOR_PAIR(IMPORTANT_COLOR);

		wscrl(output.log_win,output.logwin_scroll);

		char lineno[5];
		int linenol,y,x;
		assert(log);
		//wscrl(log, 1);
		wmove(log, (2*LINES/3)-1, 0);

		wattron(log, A_BOLD);
		wprintw(log,"[%s] ",slug);
		wattroff(log, A_BOLD);

		wattron(log,color);
		vwprintw(log, text, args);

		getyx(log,y,x);
		sprintf(lineno,"%d",wline);
		linenol = strlen(wfile) + strlen(lineno)+2; 
		wattr_on(log,WA_DIM,NULL);
		mvwprintw(log,y,COLS-linenol,"%s:%d",wfile, wline);
		wattr_off(log,WA_DIM,NULL);
		wmove(log,y,COLS-1);

		wprintw(log,"\n");

		wattroff(log,color);
		mvwhline(log,0,0,0,COLS);//box(log, 0, 0);
		mvwaddch(output.log_win,0,0,ACS_LLCORNER);

		wscrl(output.log_win, -output.logwin_scroll);

		update_panels(); doupdate();

	} else {
		if (msg == MSG_LOG) {
			printf("[%s] ", slug);
			vprintf(text, args);
			printf("\n");
			fflush(stdout);

		} else if (msg == MSG_ERROR) {
			printf("ERROR! [%s] ", slug);
			vprintf(text, args);
			printf("\n");
			fflush(stdout);
		} else if (msg == MSG_WARNING) {
			printf("WARNING [%s] ", slug);
			vprintf(text, args);
			printf("\n");
			fflush(stdout);
		}

	}

	char fname[255] = {0};
	sprintf(fname, "logs/tcp%d.log", getpid());
	
	FILE* log_file = fopen(fname, "a");
	
	fprintf(log_file, "[%s] ", slug);
	vfprintf(log_file, text, args);
	fprintf(log_file, "\n");

	fclose(log_file);

	va_end(args);
	pthread_mutex_unlock(&output.lock);
}
