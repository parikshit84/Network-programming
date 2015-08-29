#include	"unp.h"
#define ODR_PATH "/tmp/odr_pb.dg"   // well known path for ODR Unix domain datagram 
#define SRV_PATH "/tmp/srv_pb.dg"   // well known path for server
void msg_send (int,char *,int,char *,int);
void msg_recv(int);
char recvbuff[MAXLINE];

struct incoming_msg_info
{
//int  sockfd;
char msgbody[100];
char src_ip[15];
//int * src_port;
int destport ;
};

//struct incoming_msg_info msg_recv(int);


int
main(int argc, char **argv)
{
	int					sockfd,dest_port=2000,flag;
  int * src_port;
	struct sockaddr_un	servaddr;
	char dest_ip[15],recvmsgbody[100],sendmsgbody[100],src_ip[15];
 
  struct hostent *hptr;
  struct in_addr **addr_list;
  struct incoming_msg_info incoming_msg;
  
  time_t curtime;
	

	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);  
 
  unlink (SRV_PATH);
	bzero(&servaddr, sizeof(servaddr));		/* bind an address for us */
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, SRV_PATH);
  printf("\n pathname is %s",servaddr.sun_path);

	Bind(sockfd, (SA *) &servaddr, sizeof(servaddr));
 printf("\n Waiting for incoming messages:"); 
 
for(;;)
{ 
//wait for incoming messages: 
 printf("\n Waiting for incoming messages:"); 
  msg_recv(sockfd);
 strcpy(dest_ip,recvbuff);
 
 if ( strcmp( (recvbuff+30),"time") == 0)
 {
 printf("\n Time service requested!");
 
 curtime= time(NULL); 
//generate current timestamp
strcpy(sendmsgbody,ctime(&curtime)); 
msg_send(sockfd,dest_ip,dest_port,sendmsgbody,flag);
}

}
unlink(servaddr.sun_path);
	exit(0);
}


//struct incoming_msg_info 
void msg_recv(rsockfd)
{
 
 struct incoming_msg_info rincoming_msg;
	struct sockaddr_un	cliaddr;
	socklen_t	clilen=sizeof(cliaddr);
 
 int n=Recvfrom(rsockfd, recvbuff, MAXLINE, 0, (SA *) &cliaddr, &clilen);
    printf("\n %d bytes received !Sender's addr is %s Msg is %s",n,cliaddr.sun_path,recvbuff);
    
    
//    return (rincoming_msg);
    
}


void 
msg_send(int fsockfd,char *fdest_ip,int fdest_port,char *fmsgbody,int fflag)
{
  socklen_t odrlen;
  struct sockaddr_un	odraddr;
  int i;
  char tempstr[100];
  
  bzero(&odraddr, sizeof(odraddr));	// fill in ODR's address 
  odraddr.sun_family = AF_LOCAL;
  strcpy(odraddr.sun_path, ODR_PATH);
  
  odrlen=sizeof(odraddr);       
 
  strcpy(tempstr,fdest_ip);
  memcpy (tempstr+15,&fdest_port,sizeof(int));
  memset(tempstr+20,2,1);   //Set byte 20 to 2 for msg type = 2
  strcpy(tempstr+21,fmsgbody);
  //strcpy(fmsgbody,tempstr);
  
  printf("\n Dest IP is %s",tempstr); 
  printf("\n Dest Port is %d",*( (int *) (tempstr+15) ) );
  printf("\n Msg type = %d",tempstr[20]);
  printf("\n Timestamp=%s",tempstr+21);
  
 
  Sendto(fsockfd, tempstr, 100, 0, (SA *) &odraddr, odrlen); 
  printf("\n Response sent to ODR service. Waiting for incoming messages...");
  
  
}            

