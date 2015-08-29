#include	"unp.h"
# include <stdio.h>
#define SIZE 1024

int
main(int argc, char **argv)
{
	int					sockfd, n , i, inp_type,strcmp_ret;
	char				recvline[MAXLINE + 1];
	struct sockaddr_in	servaddr;
	
 
 char			*ptr, **pptr;
	char			str[INET_ADDRSTRLEN];
	struct hostent	*hptr;
 char user_input[10];
 pid_t pid;
 int pfd[2];
  char buf[SIZE];
  int nread;
  void	 str_cli3(FILE *, int);
  
 



/*********************************************************************/
  strcpy(buf, "\nTime child process started normally...\n");
         /* include null terminator in write */
         write(9, buf,strlen(buf)+1);  // 9 is the file descriptor for the pipe

 strcpy(buf, "\nWelcome to the time service\n");
         /* include null terminator in write */
         write(1, buf,strlen(buf)+1);


str_cli3(stdin,10);   //10 is the FD for sockfd

	exit(0);
}

void str_cli3(FILE *fp, int sockfd)
{
	char	recvline[MAXLINE];

	for(;;)
	{
		if (Readline(sockfd, recvline, MAXLINE) == 0)
			err_quit("str_cli: server terminated prematurely");

		printf ("\n Response from server --->");
		Fputs(recvline, stdout);
	}
}






