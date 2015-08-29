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
  void	 str_cli2(FILE *, int);
  
 

  
/*********************************************************************/
  strcpy(buf, "Echo child process started normally...\n");
         /* include null terminator in write */
         write(9, buf,strlen(buf)+1);  //9 is the file descriptor for the pipe

 strcpy(buf, "\nWelcome to the Echo service..\n");
         /* include null terminator in write */
         write(1, buf,strlen(buf)+1);


str_cli2(stdin,10);   //10 is the FD for sockfd

	exit(0);
}

void str_cli2(FILE *fp, int sockfd)
{
	char	sendline[MAXLINE], recvline[MAXLINE];

	while (Fgets(sendline, MAXLINE, fp) != NULL) 
	{
		//printf("\nPassed to server ---> %s",sendline);
		Writen(sockfd, sendline, strlen(sendline));

		if (Readline(sockfd, recvline, MAXLINE) == 0)
			err_quit("str_cli: server terminated prematurely");

		printf ("\n Response from server --->");
		Fputs(recvline, stdout);
	}
}






