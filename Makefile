# Makefile for IP 

CC				= /usr/bin/gcc
CS168_VAN_ROOT	= /course/cs168/van2
PROJECT			= ip
VAN_DRIVER		= van_driver
IP_DRIVER		= ip_driver
LIBRARY			= lib$(PROJECT).so

# Driver objects
VAN_OBJECTS		= van_driver.o rtable.o fancy_display.o state.o tcp.o
IP_OBJECTS		= ip_driver.o rtable.o fancy_display.o state.o tcp.o

# All other dependent objects
LIBOBJECTS		= van_driver.o # XXX Put all objects om which your IP driver depends here

# Compiler / Linker flags
DEBUG_FLAGS	= -g -Wall 
IFLAGS		= -I$(CS168_VAN_ROOT) -I/course/cs168/van2/util -I$(CS168_VAN_ROOT)/pub -I$(CS168_VAN_ROOT)/lib -I$(CS168_VAN_ROOT)/van2 
CFLAGS		= $(DEBUG_FLAGS) $(IFLAGS) -D_REENTRANT -D_XOPEN_SOURCE=500

LDIRS		= $(CS168_VAN_ROOT)/lib .
LDFLAGS		= $(LDIRS:%=-L%) $(LDIRS:%=-Wl,-R%) 
SOFLAGS		= -shared

VAN_DRIVER_LIBS	= -lvan2 -lform -lmenu -lpanel -lncurses
IP_DRIVER_LIBS	= -l$(PROJECT) -lutil
IP_LIBRARY_LIBS	= -lvan2 -lpthread -lutil -lrt -ldl -lm -lform -lmenu -lpanel -lncurses

all: $(LIBRARY) $(IP_DRIVER) $(VAN_DRIVER)

$(VAN_DRIVER): $(VAN_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(VAN_OBJECTS) $(VAN_DRIVER_LIBS)

$(IP_DRIVER): $(IP_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(IP_OBJECTS) $(IP_DRIVER_LIBS)

$(LIBRARY): $(LIBOBJECTS)
	$(CC) $(SOFLAGS) $(LDFLAGS) -o $@ $(LIBOBJECTS) $(IP_LIBRARY_LIBS) 

%.o : %.c
	@$(CC) $(CFLAGS) -M $< > $(@:.o=.d)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY : tags
tags:
	cscope -b -R
#	ctags --defines --typedefs-and-c++ --members --globals --no-warn *.[chCH]\
	*.cpp

.PHONY : clean
clean: tidy
	rm -f $(LIBRARY) $(IP_DRIVER) $(VAN_DRIVER) *~

.PHONY : tidy
tidy:
	rm -f $(VAN_OBJECTS) $(IP_OBJECTS) $(LIBOBJECTS) $(IP_OBJECTS:.o=.d) $(LIBOBJECTS:.o=.d) $(VAN_OBJECTS:.o=.d)

-include $(OBJECTS:.o=.d)
