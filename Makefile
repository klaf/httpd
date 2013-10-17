CC=gcc
CFLAGS=-O2 -g -Wall -Werror -lpthread
OBJS= httpd.o config.o daemon.o log.o sock.o terminate.o web.o

all: ${OBJS} 
	${CC} ${CFLAGS} ${OBJS} -o httpd 
