CC = gcc

LIBS = -lpthread\
	/home/courses/cse533/Stevens/unpv13e/libunp.a
	
FLAGS = -g -O2

CFLAGS = ${FLAGS} -I /home/courses/cse533/Stevens/unpv13e/lib/

all: prhwaddrs ODR_parbhattacha client_parbhattacha server_parbhattacha

prhwaddrs: prhwaddrs.o get_hw_addrs.o
	${CC} -o prhwaddrs prhwaddrs.o get_hw_addrs.o ${LIBS} 

ODR_parbhattacha: ODR_parbhattacha.o get_hw_addrs.o
	${CC} -o ODR_parbhattacha ODR_parbhattacha.o get_hw_addrs.o ${LIBS}
 
client_parbhattacha: client_parbhattacha.o get_hw_addrs.o
	${CC} -o client_parbhattacha client_parbhattacha.o get_hw_addrs.o  ${LIBS}
 
server_parbhattacha: server_parbhattacha.o get_hw_addrs.o
	${CC} -o server_parbhattacha server_parbhattacha.o get_hw_addrs.o  ${LIBS}
 

get_hw_addrs.o: get_hw_addrs.c 
	${CC} ${CFLAGS} -c get_hw_addrs.c


prhwaddrs.o: prhwaddrs.c 
	${CC} ${CFLAGS} -c prhwaddrs.c
 
ODR_parbhattacha.o: ODR_parbhattacha.c 
	${CC} ${CFLAGS} -c ODR_parbhattacha.c
 
client_parbhattacha.o: client_parbhattacha.c 
	${CC} ${CFLAGS} -c client_parbhattacha.c
 
server_parbhattacha.o: server_parbhattacha.c 
	${CC} ${CFLAGS} -c server_parbhattacha.c
