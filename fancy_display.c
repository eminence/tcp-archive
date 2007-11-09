#include <assert.h>
#include <string.h>

#include "fancy_display.h"


static curses_out_t output;

void update_link_line(int line, int state) {

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
}

void scroll_logwin(int i) {
	/*switch (i) {
		case KEY_UP:
			wscrl(output.log_win,-1);
			update_panels(); doupdate();
			break;
		case KEY_DOWN:
			wscrl(output.log_win,1);
			update_panels(); doupdate();
			break;

	}*/
}

void nlog_set_menu(const char *msg, ...) {
	va_list args;
	va_start(args, msg);
	pthread_mutex_lock(&output.lock);
	wmove(output.menu_win,0,0);
	vwprintw(output.menu_win, msg, args);
	update_panels(); doupdate();
	va_end(args);
	pthread_mutex_unlock(&output.lock);
}



void display_msg(char *msg, ...) {
	va_list args;
	int l = strlen(msg);
	WINDOW *my_form_win;
	PANEL *my_form_pan;

	va_start(args,msg);

	/* Create the window to be associated with the form */
	my_form_win = newwin(3, 2 + l, LINES/2 - (3/2), COLS/2 - ((l+2)/2));
	leaveok(my_form_win, FALSE);
	my_form_pan = new_panel(my_form_win);
	keypad(my_form_win, TRUE);
	wbkgd(my_form_win, COLOR_PAIR(DEFAULT_COLOR));

	/* Print a border around the main window and print a title */
	box(my_form_win, 0, 0);
	wmove(my_form_win, 1, 1);
	vwprintw(my_form_win,msg,args);
	//wprintw(my_form_win, 1, 0, cols + 4, "My Form", COLOR_PAIR(1));
	
	move(0,COLS);
	wmove(my_form_win,0,0);
	update_panels(); doupdate();

	/* Loop through to get user requests */
	wgetch(my_form_win);

	/* Un post form and free the memory */
	del_panel(my_form_pan);
	delwin(my_form_win);
	update_panels(); doupdate();
	va_end(args);
}



int get_text(char *msg, char* buf, int len) {
	
	FIELD *field[2];
	FORM  *my_form;
	int rows, cols,ch, num;
	int l = strlen(msg);
	WINDOW *my_form_win;
	PANEL *my_form_pan;
	char *buff = NULL;

	/* Initialize the fields */ /* height, width, toprow, leftcol, offscreen, buffs*/
	field[0] = new_field(2, 15, 1, 1, 0, 0);
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
	while((ch = wgetch(my_form_win)) != '`')
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
				//form_driver(my_form, REQ_VALIDATION);
				form_driver(my_form, REQ_END_LINE);
				break;
			default:
				/* If this is a normal character, it gets */
				/* Printed				  */	
				form_driver(my_form, ch);
				//form_driver(my_form, REQ_VALIDATION);
				//form_driver(my_form, REQ_);
				break;
		}
	}

	buff = field_buffer(field[0],0);
	assert(buff);
	//printf(buff);
	//sscanf(buff,"%d", &num);
	memcpy(buf, buff, 15);


	/* Un post form and free the memory */
	unpost_form(my_form);
	free_form(my_form);
	free_field(field[0]);
	del_panel(my_form_pan);
	delwin(my_form_win);
	update_panels(); doupdate();

	return 15;
}



int get_number(char *msg) {
	
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

	/* Loop through to get user requests */
	while((ch = wgetch(my_form_win)) != '`')
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

	return num;
}

void switch_to_tab(int t) {
		int max_size = 0;
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
}

int init_display(int use_curses) {

	output.use_curses = use_curses;
	pthread_mutex_init(&output.lock,0);
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

		output.log_win = newwin(LINES/2, COLS, LINES/2, 0);
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

		}

		wbkgd(stdscr,COLOR_PAIR(DEFAULT_COLOR));
		redrawwin(stdscr);
											/*height, width, starty, startx*/
		output.menu_win = newwin(1, COLS, 0,0);
		output.menu_pan = new_panel(output.menu_win);
		wbkgd(output.menu_win, COLOR_PAIR(MENU_COLOR));
		wattron(output.menu_win, COLOR_PAIR(MENU_COLOR));

		output.link_win = newwin(LINES/2, 12, 1, COLS-12);
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

		output.tab_win = newwin(LINES/2-1, COLS-12, 1,0);
		output.tab_pan = new_panel(output.tab_win);

		mvwvline(output.tab_win,0,0,0,LINES/2 -1);
		switch_to_tab(1);

											/*height, width, starty, start */
		output.rtable_win = newwin(LINES/2-1-3, COLS-12-1, 4,1);
		output.rtable_pan = new_panel(output.rtable_win);
		leaveok(output.rtable_win, FALSE);
		//mvwvline(output.rtable_win,0,0,0,LINES/2 -1-3);
		mvwprintw(output.rtable_win,0,1,"rtable window");

		output.tcp_win = newwin(LINES/2-1-3, COLS-12-1, 4,1);
		output.tcp_pan = new_panel(output.tcp_win);
		//mvwvline(output.tcp_win,0,0,0,LINES/2 -1-3);
		mvwprintw(output.tcp_win,0,1,"tcp window");

		refresh();
		update_panels(); doupdate();

	} else {
			
	}

	return 0;
}

void show_route_table() {
	top_panel(output.rtable_pan);
	switch_to_tab(0);
	update_panels(); doupdate();
}

void show_tcp_table() {
	top_panel(output.tcp_pan);
	switch_to_tab(1);
	update_panels(); doupdate();
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
	werase(output.rtable_win);
	wmove(output.rtable_win,0 ,1);

	update_panels(); doupdate();
}

void rtable_print( char *text, ...) {
	va_list args;
	va_start(args, text);
	vwprintw(output.rtable_win, text, args);
	wprintw(output.rtable_win,"\n");
	update_panels(); doupdate();
	va_end(args);
}
int get_key() {
	return wgetch(stdscr);
}


void nlog_s(const char *wfile, int wline,msg_type msg, const char *slug, char *text, ...) {
	int c = output.use_curses;
	WINDOW *log = output.log_win;
	va_list args;
	va_start(args, text);
	pthread_mutex_lock(&output.lock);

	if (msg == MSG_LOG) {
		if (c) {
			char lineno[5];
			int linenol,y,x;
			assert(log);
			//wscrl(log, 1);
			wmove(log, (LINES/2)-1, 0);
		
			wattron(log, A_BOLD);
			wprintw(log,"[%s] ",slug);
			wattroff(log, A_BOLD);

			wattron(log,COLOR_PAIR(MSG_LOG_COLOR));
			vwprintw(log, text, args);

			getyx(log,y,x);
			sprintf(lineno,"%d",wline);
			linenol = strlen(wfile) + strlen(lineno)+2; 
			mvwprintw(log,y,COLS-linenol,"%s:%d",wfile, wline);
			wmove(log,y,COLS-1);

			wprintw(log,"\n");

			wattroff(log,COLOR_PAIR(MSG_LOG_COLOR));
			mvwhline(log,0,0,0,COLS);//box(log, 0, 0);
			mvwaddch(output.log_win,0,0,ACS_LLCORNER);
			update_panels(); doupdate();
		} else {
			printf("[%s] ", slug);
			vprintf(text, args);
			printf("\n");
			fflush(stdout);
		}

	} else if (msg == MSG_ERROR) {
		if (c) {
			char lineno[5];
			int linenol,y,x;
			assert(log);

			wscrl(log, 1);
			wmove(log, (LINES/2)-2, 0);
		
			wattron(log, A_BOLD);
			wprintw(log,"[%s] ",slug);
			wattroff(log, A_BOLD);

			wattron(log,COLOR_PAIR(MSG_ERROR_COLOR));
			vwprintw(log, text, args);

			getyx(log,y,x);
			sprintf(lineno,"%d",wline);
			linenol = strlen(wfile) + strlen(lineno)+2; 
			mvwprintw(log,y,COLS-linenol,"%s:%d",wfile, wline);
			wmove(log,y,COLS-1);
			
			wprintw(log,"\n");
			wattroff(log,COLOR_PAIR(MSG_ERROR_COLOR));
			mvwhline(log,0,0,0,COLS);//box(log, 0, 0);
			mvwaddch(output.log_win,0,0,ACS_LLCORNER);
			update_panels(); doupdate();
		} else {
			printf("ERROR! [%s] ", slug);
			vprintf(text, args);
			printf("\n");
			fflush(stdout);
		}
	} else if (msg == MSG_WARNING) {
		if (c) {
			char lineno[5];
			int linenol,y,x;
			assert(log);

			wscrl(log, 1);
			wmove(log, (LINES/2)-2, 0);

			wattron(log, A_BOLD);
			wprintw(log,"[%s] ",slug);
			wattroff(log, A_BOLD);

			wattron(log,COLOR_PAIR(MSG_WARNING_COLOR));
			vwprintw(log, text, args);

			getyx(log,y,x);
			sprintf(lineno,"%d",wline);
			linenol = strlen(wfile) + strlen(lineno)+2; 
			mvwprintw(log,y,COLS-linenol,"%s:%d",wfile, wline);
			wmove(log,y,COLS-1);

			wprintw(log,"\n");
			wattroff(log,COLOR_PAIR(MSG_WARNING_COLOR));
			mvwhline(log,0,0,0,COLS);//box(log, 0, 0);
			update_panels(); doupdate();
		} else {
			printf("WARNING [%s] ", slug);
			vprintf(text, args);
			printf("\n");
			fflush(stdout);
		}
	}


	va_end(args);
	pthread_mutex_unlock(&output.lock);

}
