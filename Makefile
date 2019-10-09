#
# In order to execute this "Makefile" just type "make"
#

OBJS 	= main.o functions.o md5.o
SOURCE	= main.c functions.c md5.c
HEADER  = functions.h md5.h
OUT  	= project3
CC	= gcc
LDFLAGS = -g3 -pthread
FLAGS   = -c -g -lm
flags=$(shell gpgme-config --libs --cflags --ldflags)
#-g -c -pedantic -ansi  -Wall
# -g option enables debugging mode
# -c flag generates object code for separate files

$(OUT): $(OBJS)
	$(CC) -g -pthread $(OBJS) -o $@



functions.o: functions.c
	$(CC) $(FLAGS) functions.c


main.o: main.c
	$(CC) $(FLAGS) main.c

md5.o: md5.c
	$(CC) $(FLAGS) md5.c

# clean house
clean:
	rm -f $(OBJS) $(OUT) 
