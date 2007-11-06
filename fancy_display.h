#ifndef __FANCY_DISPLAY_
#define __FANCY_DISPLAY_

#include <curses.h>
#include <panel.h>
#include <menu.h>
#include <form.h>
#include <pthread.h>

typedef struct {

	int use_curses;
	pthread_mutex_t lock;

	WINDOW *rtable_win;
	PANEL	*rtable_pan;

	WINDOW *tcp_win;
	PANEL *tcp_pan;

	WINDOW *tab_win;
	PANEL *tab_pan;
	
	WINDOW *log_win;
	PANEL *log_pan;

	WINDOW *menu_win;
	PANEL *menu_pan;

	WINDOW *link_win;
	PANEL *link_pan;

	char *tabs[3];
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
#define DEFAULT_COLOR 13

#define LINK_DOWN_COLOR 14
#define LINK_UP_COLOR 15

#define USE_CURSES 1
#define NO_USE_CURSES 0

int init_display(int use_curses);
void nlog(msg_type msg, const char *slug, char *text, ...);
int get_key();
int get_number(char *);
void display_msg(char *msg);
void update_link_line(int, int);
void scroll_logwin(int);
void nlog_set_menu(const char *msg, ...);
void show_route_table();
void show_tcp_table();
void rtable_print( char *text, ...);
void clear_rtable_display();

#endif
