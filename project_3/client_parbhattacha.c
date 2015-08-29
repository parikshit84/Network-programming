#include	"unp.h"
#include	"hw_addrs.h"
#define ODR_PATH "/tmp/odr_pb.dg"   // well known path for Unix domain datagram 
void msg_send (int,char *,int,char *,int);
void msg_recv(int);
void lookup_ip(void);
char recvbuff[MAXLINE];
char * usernode;
/////////////
struct  sockaddr  *my_eth0_ip_addr_n;	/* eth0 IP address in network byte format */
char my_eth0_ip_addr_p[15];  // eth0 IP address in presentation format 
char my_host_name[50];

/////////////

int
main(int argc, char **argv)
{
	int					sockfd,dest_port=1000,flag,ret;
	struct sockaddr_un	cliaddr;
	char dest_ip[15],msgbody[100];
 char * usernode;
 usernode= (char *) malloc (5);
 
 struct hostent *hptr;
struct in_addr **addr_list;
	

	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

	bzero(&cliaddr, sizeof(cliaddr));		/* bind an address for us */
	cliaddr.sun_family = AF_LOCAL;
	strcpy(cliaddr.sun_path, tmpnam(NULL));
 printf("\n Temp pathname is %s",cliaddr.sun_path);

	Bind(sockfd, (SA *) &cliaddr, sizeof(cliaddr));
 
 lookup_ip();

again: 
for(;;)
{ 
 printf("\n Please enter a server node value from vm1 thru vm10:  ");
 ret=scanf("%s",usernode);
 
 printf("\n You entered node # %d", atoi(usernode+2));
 if( (usernode[0] != 'v') || (usernode[1] != 'm') || (atoi(usernode+2)<1) || (atoi(usernode+2) > 10)   )
  {
   printf("\n Invalid node entered. Please choose a value from vm1 thru vm10 (case sensitive)");
   goto again;
   }
 else   
 {
    printf("\n Input accepted");
    
    hptr = gethostbyname(usernode);
    addr_list = (struct in_addr **)hptr->h_addr_list;
    printf("\nIP address of server is %s ", inet_ntoa(*addr_list[0]));
    strcpy(dest_ip,inet_ntoa(*addr_list[0])); 
    strcpy(msgbody,"time");   
  }   
  printf("\n---------------------------------------------------------");
  printf("\n Client at node %s sending request to server at node %s",my_host_name,usernode);
  printf("\n---------------------------------------------------------");

	msg_send(sockfd,dest_ip,dest_port,msgbody,flag);
  printf("\n Waiting for response from server:"); 
  msg_recv(sockfd);
  
}  

unlink(cliaddr.sun_path);
	exit(0);
}


void 
msg_send(int fsockfd,char *fdest_ip,int fdest_port,char *fmsgbody,int fflag)
{
  socklen_t odrlen;
  struct sockaddr_un	odraddr;
  int i;
  char tempstr[100],fdest_port_str[10];
  
  bzero(&odraddr, sizeof(odraddr));	/* fill in ODR's address */
  odraddr.sun_family = AF_LOCAL;
  strcpy(odraddr.sun_path, ODR_PATH);
  
  odrlen=sizeof(odraddr);       
 
  strcpy(tempstr,fdest_ip);
//  itoa(fdest_port,fdest_port_str,10);
//  strcat(tempstr,fdest_port_str);
  memcpy (tempstr+15,&fdest_port,sizeof(int));
  memset(tempstr+20,0,1);   //Set byte 20 to 0 for msg type = 0
  strcpy(tempstr+21,fmsgbody);
  
  printf("\n Dest IP is %s",tempstr); 
  printf("\n Dest Port is %d",*( (int *) (tempstr+15) ) );
  printf("\n Msg type = %d",tempstr[20]);
  printf("\n Timestamp=%s",tempstr+21);
  
  
  
 
 Sendto(fsockfd, tempstr, 100, 0, (SA *) &odraddr, odrlen); 
  printf("\n Msg sent to ODR service");
  
  
}    


//struct incoming_msg_info 
void msg_recv(rsockfd)
{
 

	struct sockaddr_un	cliaddr;
	socklen_t	clilen=sizeof(cliaddr);
 
 int n=Recvfrom(rsockfd, recvbuff, MAXLINE, 0, (SA *) &cliaddr, &clilen);
    printf("\n %d bytes received from server %s. Response from server is %s",n,recvbuff,recvbuff+30);
    
  printf("\n---------------------------------------------------------");
  printf("\n Client at node %s received from server at node %s:",my_host_name,recvbuff);
  printf("\n %s",recvbuff+30);
  printf("\n---------------------------------------------------------");
    
    
    
    
    
    
//    return (rincoming_msg);
    
}        

void lookup_ip(void)
{

struct hostent *hptr;
struct in_addr sinaddr;
struct hwa_info	*hwa, *hwahead;
struct sockaddr	*sa;
int ind;

	for (hwahead = hwa = Get_hw_addrs(),ind=0; hwa != NULL; hwa = hwa->hwa_next) 
  {
     if(  strcmp(hwa->if_name,"eth0") == 0)
     {
        my_eth0_ip_addr_n= hwa->ip_addr;
        break;     
      }
   }   
        
strcpy(my_eth0_ip_addr_p,Sock_ntop( my_eth0_ip_addr_n, sizeof(*my_eth0_ip_addr_n)));
printf("\n My eth0 IP address=%s",my_eth0_ip_addr_p);


inet_pton(AF_INET, my_eth0_ip_addr_p, &sinaddr);
hptr = gethostbyaddr(&sinaddr, sizeof(sinaddr), AF_INET);
strcpy(my_host_name,hptr->h_name);
printf("My Host name: %s\n", my_host_name);
        
}      

