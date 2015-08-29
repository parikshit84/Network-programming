#include	"unpthread.h"
static void	*doecho(void *);		
static void	*dotime(void *);
int
main(int argc, char **argv)
{
	int				listenfd_e,listenfd_t, *iptr,*jptr;
	char port_echo[6],port_time[6];
	pthread_t		tid;
	socklen_t		addrlen, len;
	struct sockaddr	*cliaddr;
	int			maxfdp1;
	fd_set		rset;
	
	strcpy(port_echo,"9870");  //assumed to be the well-known port number for echo service
	strcpy(port_time,"9880");  //assumed to be the well-known port number for time service

    listenfd_e = Tcp_listen(NULL, port_echo, &addrlen); //set up listening socket for echo service
	listenfd_t= Tcp_listen(NULL, port_time, NULL);  //set up listening socket for time service
	
	cliaddr = Malloc(addrlen);
	FD_ZERO(&rset);

	for ( ; ; ) {
		len = addrlen;
		iptr = Malloc(sizeof(int));
		jptr = Malloc(sizeof(int));

		FD_SET(listenfd_e, &rset);
		FD_SET(listenfd_t, &rset);
		maxfdp1 = max(listenfd_e,listenfd_t) + 1;
	//	printf("Entering main select...");
		Select(maxfdp1, &rset, NULL, NULL, NULL);
	//	printf("Exited main select");

		if (FD_ISSET(listenfd_e, &rset)) 
		{	// Listening socket for echo service is readable 
          	 *iptr = Accept(listenfd_e, cliaddr, &len);
		     Pthread_create(&tid, NULL, &doecho, iptr);
		}

		
		if (FD_ISSET(listenfd_t, &rset)) 
		{	// Listening socket for time service is readable
	          	*jptr = Accept(listenfd_t, cliaddr, &len);
				Pthread_create(&tid, NULL, &dotime, jptr);
		}


	}
}

static void *
doecho(void *arg)
{
	int		connfd;
	void str_echo2(int);

	connfd = *((int *) arg);
	free(arg);

	Pthread_detach(pthread_self());
	printf("\nConnected to echo client");
	str_echo2(connfd);		/* same function as before */
	Close(connfd);			/* done with connected socket */
	return(NULL);
}


void str_echo2(int sockfd)
{
	ssize_t		n;
	char		buf[MAXLINE];

again:
	while ( (n = read(sockfd, buf, MAXLINE)) > 0)
	{
		printf("\n Echoed back to client ---> %s",buf);
		Writen(sockfd, buf, n);
		memset(buf,0,sizeof(buf));  //Flush buffer
	}



	if (n < 0 && errno == EINTR)
		goto again;
	else if (n < 0)
		err_sys("str_echo: read error");
}

static void *
dotime(void *arg)
{
	int		connfd;
	void sendtime(int);

	connfd = *((int *) arg);
	free(arg);

	Pthread_detach(pthread_self());

	printf("\nConnected to time client");
	
	sendtime(connfd);
	
	Close(connfd);			/* done with connected socket */
	return(NULL);
}

void sendtime(int sockfd)
{
	ssize_t		n;
	char		buf[MAXLINE];
	time_t				ticks;
	fd_set		rset_t;
	struct timeval tv;


	FD_ZERO(&rset_t);
	


	for(;;)
	{
	//	printf("\nEntering Inf loop for time service");

		FD_SET(sockfd, &rset_t);
		tv.tv_sec=5;
		tv.tv_usec=0;

		Select(sockfd+1, &rset_t, NULL, NULL, &tv);  //blocking until timeout value is reached
	//	printf("\nExited select for time service");

		if (FD_ISSET(sockfd, &rset_t))
		{
			printf("\nTime client disconnected!");
			break;
		}

		else //timeout value reached
		{	
			printf("\nClient is still active, sending timestamp...");
 			ticks = time(NULL);
			snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks));
			Writen(sockfd, buf, strlen(buf));
			memset(buf,0,sizeof(buf));  //Flush buffer    
		}
		
	}
	
	
}

