CC = gcc

LIBS = -lpthread\
	/home/courses/cse533/Stevens/unpv13e/libunp.a
	
FLAGS = -g -O2

CFLAGS = ${FLAGS} -I /home/courses/cse533/Stevens/unpv13e/lib/

all: parbhattacha_tour parbhattacha_arp

parbhattacha_tour: parbhattacha_tour.o get_hw_addrs.o
	${CC} -o parbhattacha_tour parbhattacha_tour.o get_hw_addrs.o ${LIBS}
 
parbhattacha_arp: parbhattacha_arp.o get_hw_addrs.o
	${CC} -o parbhattacha_arp parbhattacha_arp.o get_hw_addrs.o ${LIBS} 
 
get_hw_addrs.o: get_hw_addrs.c 
	${CC} ${CFLAGS} -c get_hw_addrs.c

parbhattacha_tour.o: parbhattacha_tour.c 
	${CC} ${CFLAGS} -c parbhattacha_tour.c
 
parbhattacha_arp.o: parbhattacha_arp.c 
	${CC} ${CFLAGS} -c parbhattacha_arp.c 
