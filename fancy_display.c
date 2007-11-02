#include <assert.h>
#include "fancy_display.h"


static curses_out_t output;

int init_display(int use_curses) {

	output.use_curses = use_curses;

	if (use_curses) {
		initscr();
		cbreak();
		noecho();
		curs_set(0);
											/*height, width, starty, startx*/
		output.log_win = newwin(LINES/2, COLS, LINES/2, 0);
		
		mvwhline(output.log_win,0,0,0,COLS); //box(output.log_win, 0, 0);
		output.log_pan = new_panel(output.log_win);
		scrollok(output.log_win,1);

		output.menu_win = newwin(1, COLS, 0,0);
		output.menu_pan = new_panel(output.menu_win);
		keypad(stdscr, TRUE);

		refresh();
		update_panels(); doupdate();

		nlog(MSG_LOG, "info","ncurses interface started up (%s)","test");

		if (has_colors() == FALSE) {
			nlog(MSG_LOG, "WARNING","No color support on this terminal");
		} else {
			nlog(MSG_LOG, "info","Using colors");
			start_color();
			init_pair(MSG_LOG_COLOR, COLOR_CYAN, COLOR_BLACK);
			init_pair(MENU_COLOR, COLOR_WHITE, COLOR_BLUE);
			init_pair(MSG_ERROR_COLOR, COLOR_RED, COLOR_BLACK);
			wbkgd(output.menu_win, COLOR_PAIR(2));
			wattron(output.menu_win, COLOR_PAIR(2));

		}

		wprintw(output.menu_win, "1:Send data    2:Receive Data   3:Something");
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


	if (msg == MSG_LOG) {
		if (c) {
			assert(log);
			wscrl(log, 1);
			wmove(log, (LINES/2)-2, 0);
		
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
		}
	}


	va_end(args);

}
