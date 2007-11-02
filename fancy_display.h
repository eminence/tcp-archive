#ifndef __FANCY_DISPLAY_
#define __FANCY_DISPLAY_

#include <curses.h>
#include <panel.h>
#include <menu.h>

typedef struct {

	int use_curses;

	WINDOW *rtable_win;
	PANEL	*rtable_pan;
	
	WINDOW *log_win;
	PANEL *log_pan;

	WINDOW *menu_win;
	PANEL *menu_pan;
} curses_out_t;

typedef enum {	
	MSG_LOG,
	MSG_WARNING,
	MSG_ERROR,
} msg_type;

#define MENU_COLOR 2

#define MSG_LOG_COLOR 10
#define MSG_WARNING_COLOR 11
#define MSG_ERROR_COLOR 12

#define USE_CURSES 1
#define NO_USE_CURSES 0

int init_display(int use_curses);
void nlog(msg_type msg, const char *slug, char *text, ...);
int get_key();

#endif
