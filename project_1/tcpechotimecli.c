#include	"unp.h"
# include <stdio.h>
#define SIZE 1024
int child_term=0;  //global variable

int
main(int argc, char **argv)
{
	int					sockfd, n , i, j,inp_type,strcmp_ret;
	char				recvline[MAXLINE + 1];
	struct sockaddr_in	servaddr;
	
 
 char			*ptr, **pptr;
 char			str[INET_ADDRSTRLEN];
 struct hostent	*hptr;
 char user_input[10];
 int option_selected;
 pid_t pid;
 int pfd[2];
  char buf[SIZE];
  int nread;
  void sig_chld(int);
  int port_echo,port_time,port_selected;
  char serv_ip_add[20];
  fd_set		rset;
 

  port_echo=9870;
  port_time=9880;


	if (argc != 2)
		err_quit("Please specify either an IP address or a domain name as a command line argument");
   
  /* check if user passed an IP address or a domain name by checking if argv[1] has any alphabets */
  inp_type = 0;
  for (i=0;argv[1][i]!='\0';i++)
  {   if ( (inp_type=isalpha(argv[1][i])) > 0)
      break;
  }
  
  if (inp_type == 0)
  {
   /* User entered an argument with no alphabets, possibly an IP address*/
          if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
       		err_quit("inet_pton error for %s", argv[1]);
            		if ( (hptr = gethostbyaddr(&servaddr.sin_addr, sizeof(servaddr.sin_addr),AF_INET)) == NULL) {
         			err_msg("gethostbyname error for host: %s: %s",	argv[1], hstrerror(h_errno));
         			 		}
         		printf("official hostname: %s\n", hptr->h_name);
    //        for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
    //  			printf("\talias: %s\n", *pptr);

			strcpy(serv_ip_add,argv[1]);
      
  }
  else if (inp_type > 0)
  /* User entered an argument which contains alphabets, possibly a domain name */
  {
       ptr = argv[1];
         		if ( (hptr = gethostbyname(ptr)) == NULL) {
         			err_msg("gethostbyname error for host: %s: %s",	ptr, hstrerror(h_errno));
         			 		}
         		printf("official hostname: %s\n", hptr->h_name);
      
   //    for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
   //   			printf("\talias: %s\n", *pptr);
      
      		switch (hptr->h_addrtype) {
      		case AF_INET:
      			pptr = hptr->h_addr_list;
      			for ( j=0; *pptr != NULL; pptr++,j++)
				{
      				printf("\nIP address: %s",Inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
      			    if (j==0)
					  strcpy(serv_ip_add,Inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
				}	
					break;
      
      		default:
      			err_ret("unknown address type");
      			break;
      		}
	}

start_main_inf_loop:
  for (;;) { //start infinite loop

  child_term=0;
  fputs("\n-------------------------",stdout);  
  fputs("\nPlease choose a service :\n",stdout);   
  fputs ("1. Enter 'echo' for echo service\n2. Enter 'time' for time service\n3. Enter 'quit' to quit\n",stdout);
  fgets(user_input, sizeof user_input, stdin);
  //printf ("text= %s", user_input);   
  
	  if(strcmp(user_input,"quit\n") == 0)
		{
	   //   printf("User wants to quit...");
		  exit(0);
		}
	  else if (strcmp(user_input,"echo\n") == 0)
	  {
		  port_selected = port_echo;
		  option_selected=1;
	  }
	  else if (strcmp(user_input,"time\n") == 0)
	  {
		  port_selected = port_time;
		  option_selected=2;
	  }
	  else 
	  { 
		  printf("\nYou have entered an invalid option. Please try again!");
		  goto start_main_inf_loop;
	  }



  


  signal(SIGCHLD,sig_chld); //Signal Handler for SIGCHLD

  /*Create a pipe for the parent to comunicate with the child process */
   if (pipe(pfd) == -1)
  {
    perror("pipe failed");
    exit(1);
  }
  if ((pid = fork()) < 0)
  {
    perror("fork failed");
    exit(2);
  }


     if (pid ==0) //child process
      {
         //printf("Entered child process");
         close(pfd[0]);    //Close child's read end of pipe
		 
		 /* include null terminator in write */
         write(pfd[1], buf,strlen(buf)+1);
		 dup2(pfd[1],9);  //The exec'd child process will use FD 9 to write to the pipe

        //*******************************************************************Connect to server
		if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			err_sys("socket error");

		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port   = htons(port_selected);	/* echo or time server */
		if (inet_pton(AF_INET, serv_ip_add, &servaddr.sin_addr) <= 0)
			err_quit("inet_pton error for %s", argv[1]);
	//	else
	//		printf("No inet error\n");

		if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
			err_sys("\nConnection to server failed");
		else
			printf("\nConnected to server\n");
	   //***********************************************************************
		 dup2(sockfd,10); //The exec'd child process will use FD 10 to connect to the server

	//	 printf("\nDup2 done!");
	//	 printf("\nOption selected= %d",option_selected);
		 
		 if (option_selected == 1)
			execlp("xterm","xterm","-e","./echo_cli","127.0.0.1",(char *) 0)   ;

		 if (option_selected == 2)
			execlp("xterm", "xterm", "-e", "./time_cli", "127.0.0.1", (char *) 0);

		
       }
      else  //parent process
      {
        // printf("This is the parent process");
		 close (pfd[1]);    //Close parent's write end of pipe

		 //Use Select to monitor both the pipe and the std input
		 
		printf("\nWaiting for child process to start...");
		
		 for(;;)
		 {
			 //Wait for child to send a msg thru the pipe confirming that it started successfully
		 if (read(pfd[0], buf, SIZE)<=0)
			 continue;
		 else
			 break;
		 }

		for(;;)
		 {
			 FD_ZERO(&rset);   //initialise the select
		     FD_SET(pfd[0], &rset);  //monitor pipe for messages from child process
			 FD_SET(fileno(stdin),&rset);  //monitor std input for user input
		
			//printf("\n Entered select in standby parent");
			if( select(max(pfd[0],fileno(stdin))+1, &rset, NULL, NULL, NULL) < 0)
			  {
				if (errno == EINTR)    //handle EINTR error when child is terminated manually
					goto start_main_inf_loop;
				else 
					err_quit("Select error!");
			  }

	//		printf("\nExited select in standby parent");

			if (FD_ISSET(pfd[0], &rset))
			{
				read(pfd[0], buf, SIZE);
				printf("\nMsg sent via pipe from child process --> %s", buf);
	//			FD_CLR(pfd[0], &rset);
			}

			if (FD_ISSET(fileno(stdin), &rset))
			{
				fgets(buf, sizeof buf, stdin);  //need to read input to reset readability state of stdin
				memset(buf,0,sizeof(buf));  //Flush buffer  
			    printf("\nPlease use the xterm child window to enter your input!");
			//	FD_CLR(fileno(stdin), &rset);
			}
		 }
	 }
		  
    
  }  //end infinite loop


  //printf("\nPARENT PROCESS EXITED INF LOOP!\n");

	
	
	exit(0);
}


void
sig_chld(int signo)
{
	pid_t	pid;
	int		stat, child_exit_status;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
	{
		printf("\nchild %d terminated\n", pid);
		child_term=1;  //global variable
	}

	if (WIFEXITED(stat)>0)
	{	
		child_exit_status = WEXITSTATUS(stat);
        printf("\nChild's exit status was %d \n",child_exit_status);
	}

//	goto start_main_inf_loop;
	return;
}

