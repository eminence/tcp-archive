#include <assert.h>
#include <string.h>

#include "fancy_display.h"


static curses_out_t output;

void update_link_line(int line, int state) {
	
	mvwprintw(output.link_win, 2+line, 1, "%d: %s\n", line,state == 0?"Down":"Up");
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

int get_number(const char *msg) {
	
	FIELD *field[2];
	FORM  *my_form;
	int rows, cols,ch, num;
	int l = strlen(msg);
	WINDOW *my_form_win;
	PANEL *my_form_pan;
	char *buff = NULL;

	/* Initialize the fields */ /* height, width, toprow, leftcol, offscreen, buffs*/
	field[0] = new_field(1, 5, 1, l, 0, 0);
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
	my_form_win = newwin(rows + 4, cols + 2 + l, LINES/2 - (rows/2), COLS/2 - (cols/2));
	my_form_pan = new_panel(my_form_win);
	keypad(my_form_win, TRUE);

	/* Set main window and sub window */
	set_form_win(my_form, my_form_win);
	set_form_sub(my_form, derwin(my_form_win, rows, cols, 2, 2));

	/* Print a border around the main window and print a title */
	box(my_form_win, 0, 0);
	mvwprintw(my_form_win, 3,1,msg);
	//wprintw(my_form_win, 1, 0, cols + 4, "My Form", COLOR_PAIR(1));

	post_form(my_form);
	update_panels(); doupdate();

	/* Loop through to get user requests */
	while((ch = wgetch(my_form_win)) != 'a')
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
	printf(buff);
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

int init_display(int use_curses) {

	output.use_curses = use_curses;
	pthread_mutex_init(&output.lock,0);

	if (use_curses) {
		initscr();
		cbreak();
		noecho();
		curs_set(0);
		keypad(stdscr, TRUE);
											/*height, width, starty, startx*/
		output.log_win = newwin(LINES/2, COLS, LINES/2, 0);
		
		mvwhline(output.log_win,0,0,0,COLS); //box(output.log_win, 0, 0);
		output.log_pan = new_panel(output.log_win);
		scrollok(output.log_win,1);

		output.menu_win = newwin(1, COLS, 0,0);
		output.menu_pan = new_panel(output.menu_win);

		output.link_win = newwin(LINES/2, 12, 1, COLS-12);
		output.link_pan = new_panel(output.link_win);
		//mvwvline(output.link_win,0,0,0,LINES/2); /* y, x, ch, n */
		//mvwhline(output.link_win,LINES/2-1,0,0,10);
		wborder(output.link_win,0,' ',' ',0,ACS_VLINE,' ',ACS_BTEE,' ');
		mvwprintw(output.link_win, 0,1,"Link State\n");
		wmove(output.link_win,0,0);



		refresh();
		update_panels(); doupdate();

		nlog(MSG_LOG, "info","ncurses interface started up (%s)","test");

		if (has_colors() == FALSE) {
			nlog(MSG_LOG, "info","No color support on this terminal");
		} else {
			nlog(MSG_LOG, "info","Using colors");
			start_color();
			init_pair(MSG_LOG_COLOR, COLOR_CYAN, COLOR_BLACK);
			init_pair(MENU_COLOR, COLOR_WHITE, COLOR_BLUE);
			init_pair(MSG_ERROR_COLOR, COLOR_RED, COLOR_BLACK);
			init_pair(MSG_WARNING_COLOR, COLOR_YELLOW, COLOR_BLACK);
			wbkgd(output.menu_win, COLOR_PAIR(2));
			//wbkgd(output.link_win, COLOR_PAIR(2));
			wattron(output.menu_win, COLOR_PAIR(2));

		}

		wprintw(output.menu_win, "1:Send data   2:Receive Data   3:Toggle Link State   q:Quit");
		update_panels(); doupdate();

	} else {
			
	}

	return 0;
}

int get_key() {
	return wgetch(stdscr);
}

void nlog(msg_type msg, const char *slug, char *text, ...) {
	int c = output.use_curses;
	WINDOW *log = output.log_win;
	va_list args;
	va_start(args, text);
	pthread_mutex_lock(&output.lock);

	if (msg == MSG_LOG) {
		if (c) {
			assert(log);
			//wscrl(log, 1);
			wmove(log, (LINES/2)-1, 0);
		
			wattron(log, A_BOLD);
			wprintw(log,"[%s] ",slug);
			wattroff(log, A_BOLD);

			wattron(log,COLOR_PAIR(MSG_LOG_COLOR));
			vwprintw(log, text, args);
			wprintw(log,"\n");
			wattroff(log,COLOR_PAIR(MSG_LOG_COLOR));
			mvwhline(log,0,0,0,COLS);//box(log, 0, 0);
			update_panels(); doupdate();
		} else {
			printf("[%s] ", slug);
			vprintf(text, args);
			printf("\n");
			fflush(stdout);
		}

	} else if (msg == MSG_ERROR) {
		if (c) {
			assert(log);

			wscrl(log, 1);
			wmove(log, (LINES/2)-2, 0);
		
			wattron(log, A_BOLD);
			wprintw(log,"[%s] ",slug);
			wattroff(log, A_BOLD);

			wattron(log,COLOR_PAIR(MSG_ERROR_COLOR));
			vwprintw(log, text, args);
			wprintw(log,"\n");
			wattroff(log,COLOR_PAIR(MSG_ERROR_COLOR));
			mvwhline(log,0,0,0,COLS);//box(log, 0, 0);
			update_panels(); doupdate();
		} else {
			printf("ERROR! [%s] ", slug);
			vprintf(text, args);
			printf("\n");
			fflush(stdout);
		}
	} else if (msg == MSG_ERROR) {
		if (c) {
			assert(log);

			wscrl(log, 1);
			wmove(log, (LINES/2)-2, 0);

			wattron(log, A_BOLD);
			wprintw(log,"[%s] ",slug);
			wattroff(log, A_BOLD);

			wattron(log,COLOR_PAIR(MSG_WARNING_COLOR));
			vwprintw(log, text, args);
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
