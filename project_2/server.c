#include	"unpifiplus.h"

#define	RTT_RXTMIN      1000	/* min retransmit timeout value, in milliseconds */
#define	RTT_RXTMAX      3000	/* max retransmit timeout value, in milliseconds */
#define	RTT_MAXNREXMT 	12  	/* max # times to retransmit */
#define	RTT_RTOCALC(ptr) ((ptr)->rtt_srtt + (4.0 * (ptr)->rtt_rttvar))

extern struct ifi_info *Get_ifi_info_plus(int family, int doaliases);
extern        void      free_ifi_info_plus(struct ifi_info *ifihead);

struct rtt_info {
  int   rtt_rtt;	/* most recent measured RTT, in milli-seconds */
  int		rtt_srtt;	/* smoothed RTT estimator, in milli-seconds */
  int		rtt_rttvar;	/* smoothed mean deviation, in milli-seconds */
  int		rtt_rto;	/* current RTO to use, in milli-seconds */
  int		rtt_nrexmt;	/* # times retransmitted: 0, 1, 2, ... */
  uint64_t	rtt_base;	/* # milli-sec since 1/1/1970 at start */
};

struct serv_conn {
           int ssockfd;
           in_addr_t ip_addr_bound;
	         in_addr_t nw_mask;
           in_addr_t subnet_addr;
           };   //Structure with details of all IP addresses bound;
           
struct datagram {
           int seqnum;                 //bytes 1 thru 4
           char ack_flag;              //byte 5
           int next_seqnum;            //byte 6 thru 9
           short free_dg;              //bytes 10 thru 11
           char eof_flag;              //byte 12
           uint32_t send_ts;           //byte 13 thru 16 
           char data[496];             //bytes 17 thru 512
           };  // datagram structure
           
           
struct ackstr {
           char ack_flag;              //byte 1
           int next_seqnum;            //byte 2 thru 5
           short free_dg;              //bytes 6 thru 7
           };  // ACK structure
           
char sendbuf[512];
struct datagram dg[20];
uint32_t tsa[20];
int sockfd,win_size;          
 
int
main(int argc, char **argv)
{
	struct ifi_info	*ifi, *ifihead;
	struct sockaddr	*sa;
	struct sockaddr_in	*sain, *cliaddr;
	u_char		*ptr;
	int		i, family, doaliases,servport,sockfd,maxfd;
 	const int			on = 1;
  fd_set		rset;
  char recvbuf[512],filename[20];
  socklen_t len;
  pid_t pid;
  void new_serv_child( struct serv_conn *, int,int ,struct sockaddr_in,char *);
  FILE *ppFile;
  
	
struct serv_conn conn[10];	
int ind,ind_used;

//servport=9870; //well known port number

len=sizeof(*cliaddr);

ppFile=fopen("server.in","r");

if(ppFile !=NULL)
		 {
		   fscanf(ppFile, "%ld %ld",&servport,&win_size);
		   printf("\nRead from server.in >>> Server's well known port number |%ld|",servport);
		   printf("\nRead from server.in >>> Max Sending Sliding window size |%ld|",win_size);
		   fclose(ppFile);
		 }
		 
		 else
		 {
   		 printf("\n Could not open the file server.in");
   		 exit(0);
     }

		family = AF_INET;

	doaliases = atoi(argv[2]);

	for (ifihead = ifi = Get_ifi_info_plus(family, doaliases),ind=0;
		 ifi != NULL; ifi = ifi->ifi_next,ind++) {
		printf("%s: ", ifi->ifi_name);
		if (ifi->ifi_index != 0)
			printf("(%d) ", ifi->ifi_index);
		printf("<");
/* *INDENT-OFF* */
		if (ifi->ifi_flags & IFF_UP)			printf("UP ");
		if (ifi->ifi_flags & IFF_BROADCAST)		printf("BCAST ");
		if (ifi->ifi_flags & IFF_MULTICAST)		printf("MCAST ");
		if (ifi->ifi_flags & IFF_LOOPBACK)		printf("LOOP ");
		if (ifi->ifi_flags & IFF_POINTOPOINT)	printf("P2P ");
		printf(">\n");
/* *INDENT-ON* */

		if ( (i = ifi->ifi_hlen) > 0) {
			ptr = ifi->ifi_haddr;
			do {
				printf("%s%x", (i == ifi->ifi_hlen) ? "  " : ":", *ptr++);
			} while (--i > 0);
			printf("\n");
		}
		if (ifi->ifi_mtu != 0)
			printf("  MTU: %d\n", ifi->ifi_mtu);

		if ( (sa = ifi->ifi_addr) != NULL)
		{
			printf("  IP addr: %s\n",
						Sock_ntop_host(sa, sizeof(*sa)));
                                             
			sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
	    Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		    sain = (struct sockaddr_in *) ifi->ifi_addr;
		    sain->sin_family = AF_INET;
		    sain->sin_port = htons(servport);
		    Bind(sockfd, (SA *) sain, sizeof(*sain));
		    
		    printf("bound %s\n", Sock_ntop((SA *) sain, sizeof(*sain)));
      conn[ind].ssockfd=sockfd;
			conn[ind].ip_addr_bound=(*sain).sin_addr.s_addr;
        }
           
/*=================== cse 533 Assignment 2 modifications ======================*/

		if ( (sa = ifi->ifi_ntmaddr) != NULL)
		{
			printf("  network mask: %s\n",
						Sock_ntop_host(sa, sizeof(*sa)));
                                             
      sain = (struct sockaddr_in *) ifi->ifi_ntmaddr;                                       
			conn[ind].nw_mask=(*sain).sin_addr.s_addr;
	  	conn[ind].subnet_addr =
                      conn[ind].ip_addr_bound & 
                      conn[ind].nw_mask;
            	
    }
           
                    

/*=============================================================================*/

		if ( (sa = ifi->ifi_brdaddr) != NULL)
			printf("  broadcast addr: %s\n",
						Sock_ntop_host(sa, sizeof(*sa)));
		if ( (sa = ifi->ifi_dstaddr) != NULL)
			printf("  destination addr: %s\n",
						Sock_ntop_host(sa, sizeof(*sa)));
	}
	free_ifi_info_plus(ifihead);
  ind_used=ind-1;
 
 
 for(;;)   //infinite loop starts here
 {
  FD_ZERO(&rset);
  maxfd=0;
 for (ind=0;ind<=ind_used;ind++) 
 {
     printf("\n Structure # %d\n",ind);
     printf("\n sockfd= %d",conn[ind].ssockfd);
     FD_SET(conn[ind].ssockfd, &rset);
     if  (conn[ind].ssockfd > maxfd)
         maxfd = conn[ind].ssockfd;
     
     printf("\n ip_addr_bound= %d ",conn[ind].ip_addr_bound);
     bzero(sain, sizeof(struct sockaddr_in));
     (*sain).sin_addr.s_addr=conn[ind].ip_addr_bound;
     sain->sin_family = AF_INET;
     printf("\n ip_addr_bound= %s ",Sock_ntop((SA *) sain, sizeof(*sain)));
     
     
     printf("\n Network mask=  %d",conn[ind].nw_mask);
     bzero(sain, sizeof(struct sockaddr_in));
     (*sain).sin_addr.s_addr=conn[ind].nw_mask;
     sain->sin_family = AF_INET;
     printf("\n Network mask= %s ",Sock_ntop((SA *) sain, sizeof(*sain)));
     
     printf("\n Subnet Address=  %d",conn[ind].subnet_addr);
     bzero(sain, sizeof(struct sockaddr_in));
     (*sain).sin_addr.s_addr=conn[ind].subnet_addr;
     sain->sin_family = AF_INET;
     printf("\n Subnet Address= %s ",Sock_ntop((SA *) sain, sizeof(*sain)));
     
     printf("\n-------------------------");
 }
 
 
 printf("\n  sizeof(int)= %d", sizeof(int));
 printf("\n sizeof(short) = %d",sizeof(short));
     
     printf("\n maxfd= %d ",maxfd);		
     printf("\nEntering main select...");
      Select(maxfd+1, &rset, NULL, NULL, NULL);  
  //blocks until there is an incoming client
  	printf("Exited main select");
 

 
 for (ind=0;ind<=ind_used;ind++) 
 {
 	   if (FD_ISSET(conn[ind].ssockfd, &rset))
    {
        printf("\n Incoming connection on socket # %d",conn[ind].ssockfd);
        bzero(cliaddr, sizeof(struct sockaddr_in));
        Recvfrom(conn[ind].ssockfd, recvbuf, sizeof(recvbuf), 0, (SA *)cliaddr, &len);
        printf("\n Message Content: %s",recvbuf);
        printf("\n Size of recv buf = %d",sizeof(recvbuf));
        printf("\n Client address= %s ",Sock_ntop((SA *) cliaddr, sizeof(*cliaddr)));
        strcpy(filename,recvbuf);
        
        //Add logic to check if we've seen this client before. Ignore if duplicate.
        
                  if ((pid = fork()) < 0)
            {
              perror("fork failed");
              exit(2);
            }
            
         if (pid ==0) //child process
            {
              printf("Created new server child");   
              new_serv_child(conn,ind,ind_used,*cliaddr,filename);
              printf("\n Child function ended normally.");
              printf("\n Waiting for new incoming clients...");
              _Exit(0);
             }         
    }
   
       
        
        
       //recvfrom or recvmsg
      //save from and to client IP address  + logic
      
      
      
//      if ( (pid = Fork()) == 0) {		/* child */
//		child(keysockfd, (SA *) &cliaddr, sizeof(cliaddr), (SA *) sa);
//		exit(0);		/* never executed */
//	}
     //fork off child child()

 }
 sleep(15);
 } //infinite loop ends


	exit(0);
}



void new_serv_child( struct serv_conn *chld_conn, int chld_ind_curr,int chld_ind_used,struct sockaddr_in	chld_cliaddr,char *filename)
{
struct sockaddr_in	*chld_sain;
int i,j,seq,seq_start,win_start,bytes_read,diff,diff_start,diff_end;
const int			on = 1;
socklen_t len;
char recvbuf[512];
struct datagram dg_ack;
FILE* fd;
short eof_seq,prev_ack_seq,dupcount;
char filebuff[5001];
char temp[513];  //extra character to accomodate null character at end for displaying via printf
fd_set		fdset;
struct timeval tv;
struct ackstr ack;
int ceiling(int,int);
void retransmit(int);
static struct rtt_info rttinfo;

void	 rtt_init(struct rtt_info *);
void	 rtt_newpack(struct rtt_info *);
int		 rtt_start(struct rtt_info *);
void	 rtt_stop(struct rtt_info *, uint32_t);
int		 rtt_timeout(struct rtt_info *);
uint32_t rtt_ts(struct rtt_info *);
int rtt_minmax(int);
uint32_t recv_ts;

chld_sain=Malloc(sizeof(struct sockaddr_in));
len=sizeof(*chld_sain);



       printf("\n [Child] Entered new child");
       printf("\n [Child] Client address= %s ",Sock_ntop((SA *) &chld_cliaddr, sizeof(chld_cliaddr)));
       printf("\n [Child] Testing...%d",chld_conn[chld_ind_curr].ip_addr_bound);
 
     
//Close all sockets except the one the connection came in on
for (i=0;i<=chld_ind_used;i++)
{
    if (i != chld_ind_curr)
    {
    Close(chld_conn[i].ssockfd);
    printf("\nSocket # %d closed",chld_conn[i].ssockfd);
    }    
}     
     
//Create a new connection socket and bind it to the server IP that the client came in on, and set port number =  0  
         sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
	       Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
               
       bzero(chld_sain, sizeof(struct sockaddr_in));
       (*chld_sain).sin_addr.s_addr=chld_conn[chld_ind_curr].ip_addr_bound;
       chld_sain->sin_family = AF_INET;
       chld_sain->sin_port = htons(0);
       printf("\n [Child] Server IP from Struct element= %s ",Sock_ntop((SA *) chld_sain, sizeof(*chld_sain)));	    
		   Bind(sockfd, (SA *) chld_sain, sizeof(*chld_sain));
	     printf("bound new connection socket %d to IP  %s and port number %d\n", sockfd, Sock_ntop((SA *) chld_sain, sizeof(*chld_sain)), chld_sain->sin_port);
           
//Use getsockname function to determine the port number assigned to the socket
      bzero(chld_sain, sizeof(struct sockaddr_in));
      getsockname(sockfd,(SA *)chld_sain,&len);
      printf("\n[Child] New connection socket bound to IP address %s Port Number %d ",
      Sock_ntop((SA *) chld_sain, sizeof(*chld_sain)), chld_sain->sin_port);
//Move port number to send buffer
memset(sendbuf,0,sizeof(sendbuf));  //Flush buffer
memcpy(sendbuf, &(chld_sain->sin_port), sizeof(in_port_t)); 
//strcpy(sendbuf,(char *) &(chld_sain->sin_port));
printf("\nSendbuf = %s",sendbuf); 
      
      
//Connect new socket to client
Connect(sockfd, (SA *) &chld_cliaddr, sizeof(chld_cliaddr));
printf("\n Socket connected to client IP %s",Sock_ntop((SA *) &chld_cliaddr, sizeof(chld_cliaddr)));

rtt_init(&rttinfo);  

//Send datagram to client from listening socket
Sendto(chld_conn[chld_ind_curr].ssockfd, sendbuf, sizeof(sendbuf), 0, (SA *) &chld_cliaddr, sizeof(chld_cliaddr));  
printf("\n Handshake 2 of 3 sent from server to client with new ephemeral port number"); 

//Wait for Handshake 3 from client
FD_ZERO(&fdset);
 FD_SET(sockfd, &fdset);    //currently checking only new connection socket - need to also monitor listening socket
 tv.tv_sec=10;
 tv.tv_usec=0;
 printf("\nWaiting for Handshake 3 from client....");
 Select(sockfd+1, &fdset, NULL, NULL, &tv);    //wait for 10 seconds for ACK from server 
 if (FD_ISSET(sockfd, &fdset))
 {
 printf("\n Handshake 3 from client received on new connection socket!");
 memset(recvbuf,0,sizeof(recvbuf));  //Flush buffer
Recv(sockfd, recvbuf, sizeof(recvbuf), 0);  
memcpy(&ack, recvbuf, sizeof(struct ackstr));
printf("\n ack.ack_flag= %c",ack.ack_flag);
printf("\n ack.next_seqnum= %d",ack.next_seqnum); 
 }
 else
 printf("\n Timed out!");

printf("\n Filename: %s",filename);
//Begin file transfer
fd = fopen(filename,"r");   
if(fd == NULL)    //open file
 {
 printf("\n File open error");
 exit(1);
 }
else
    printf("\n File opened successfully");
    
eof_seq=0;
//memset(filebuff,0,sizeof(filebuff)); //flush file buffer    
//bytes_read=fread(filebuff,1,500*win_size,fd);   //read from file
//filebuff[500*win_size]=0;  //add null character at end
//if (bytes_read<500*diff)
//       eof=1;

seq=1;
diff=win_size;
win_start = 1;
dg_ack.next_seqnum=1;
dupcount=0;
prev_ack_seq=0;

for(;;)
{   

     if (diff>0)
    {
        memset(filebuff,0,sizeof(filebuff)); //flush file buffer    
        bytes_read=fread(filebuff,1,496*diff,fd);   //read from file
        printf("\n Read %d bytes from file",bytes_read);
        
         if (bytes_read < 496*diff)
         {
             eof_seq = (seq-1) + ceiling(bytes_read,496);
                     printf("\n EOF at seq %d",eof_seq);
         }
    }

//Format datagrams

    for(j=0;j<diff;j++)
    {
          
           //printf("\n Copying 496 bytes from filebuffer starting from position %d",i*500);
            i=(seq-1)%win_size;
            dg[i].seqnum = seq;
             printf("\n Sequence number = %d , internal buffer index = %d, loop index = %d  , Data:",seq,i,j);
            memcpy(&(dg[i].data), &(filebuff[j*496]), 496);  //copy 496 bytes   
             
            if (seq == eof_seq)
               dg[i].eof_flag=1;
            else
               dg[i].eof_flag=0;
                  
            memset(sendbuf,0,sizeof(sendbuf)); //flush file buffer
             
            memcpy(&(sendbuf[0]), &(dg[i].seqnum),sizeof(int));
      //      memcpy(&(sendbuf[4]), &(dg[i].ack_flag),sizeof(char));
      //      memcpy(&(sendbuf[5]), &(dg[i].next_seqnum),sizeof(int));
      //      memcpy(&(sendbuf[9]), &(dg[i].free_dg),sizeof(short));
            memcpy(&(sendbuf[11]),&(dg[i].eof_flag),sizeof(char));
            memcpy(&(sendbuf[16]),&(dg[i].data),496);
            
            
            memcpy(temp,&(dg[i].data),496);
            temp[496]=0;
            printf("\n--------------------------------------------------------");
            printf("\n %s ",temp); 
            printf("\n--------------------------------------------------------");
            
            
  //      printf("\n size=%d dg[%d].seqnum= %d",     sizeof(dg[i].seqnum),      i,dg[i].seqnum);
  //      printf("\n size=%d dg[%d].ack_flag= %c",   sizeof(dg[i].ack_flag),    i,dg[i].ack_flag);
  //      printf("\n size=%d dg[%d].next_seqnum= %d",sizeof(dg[i].next_seqnum), i,dg[i].next_seqnum);
  //      printf("\n size=%d dg[%d].free_dg= %d",    sizeof(dg[i].free_dg),     i,dg[i].free_dg);
  //      printf("\n size=%d dg[%d].eof_flag= %c",   sizeof(dg[i].eof_flag),    i,dg[i].eof_flag);
        
  //      printf("\n sendbuf[12]= %c, sendbuf[13]= %c , sendbuf[14]=%c",sendbuf[12],sendbuf[13],sendbuf[14]);
  //      printf("\n dg[i].data[12]=%c, dg[i].data[13]= %c , dg[i].data[14]=%c",dg[i].data[12],dg[i].data[13],dg[i].data[14]);
        
  //      printf("\n sendbuf[509]= %c, sendbuf[510]= %c , sendbuf[511]=%c",sendbuf[509],sendbuf[510],sendbuf[511]);
  //      printf("\n dg[i].data[509]= %c, dg[i].data[510]= %c , dg[i].data[511]=%c",dg[i].data[509],dg[i].data[510],dg[i].data[511]);    
            
            dg[i].send_ts=rtt_ts(&rttinfo);   //store send timestamp of segment 
            rtt_newpack(&rttinfo);            //initialize for this packet
            memcpy(&(sendbuf[12]),&(dg[i].send_ts),sizeof(uint32_t));
            Send(sockfd, sendbuf, sizeof(struct datagram), 0);  //send datagram to client
            printf("\n Datagram with seq no %d sent to client with timestamp %d",dg[i].seqnum,dg[i].send_ts);
            
            if (seq == eof_seq)
            {
               printf("\n End of file encountered!");
               seq++;
                break;
            }
            
            
           seq++;
    }
    
    
    
    //select statement to wait for ACKS
    //read next seqnum from incoming ACK
     FD_ZERO(&fdset);
     FD_SET(sockfd, &fdset);    
          
     tv.tv_sec = (int)(rttinfo.rtt_rto/1000);  //convert ms value to seconds
     tv.tv_usec= (rttinfo.rtt_rto%1000)*1000;   //convert remaining milliseconds to microseconds

     printf("\n Timeout set to %d seconds and %d microseconds",tv.tv_sec,tv.tv_usec);
     printf("\nWaiting for ACKs from client....");
     Select(sockfd+1, &fdset, NULL, NULL, &tv);   
     if (FD_ISSET(sockfd, &fdset))
     {
     recv_ts=rtt_ts(&rttinfo);   //store recv timestamp of segment in recv_ts
     printf("\n ACK from client received !");
     memset(recvbuf,0,sizeof(recvbuf));  //Flush buffer
   // Recv(sockfd, recvbuf, sizeof(struct ackstr), 0);  
    Recv(sockfd, recvbuf, sizeof(recvbuf), 0); 
    
        
    memset(&dg_ack,0,sizeof(struct datagram)); //flush temp dg 
       //copy incoming datagram to temp dg from recvbuf
        memcpy( &(dg_ack.seqnum),      &(recvbuf[0]),   sizeof(int));
        memcpy( &(dg_ack.ack_flag),    &(recvbuf[4]),   sizeof(char));
        memcpy( &(dg_ack.next_seqnum), &(recvbuf[5]),   sizeof(int));
        memcpy( &(dg_ack.free_dg),     &(recvbuf[9]),   sizeof(short));
        memcpy( &(dg_ack.eof_flag),    &(recvbuf[11]),  sizeof(char));
        memcpy( &(dg_ack.send_ts),    &(recvbuf[12]),  sizeof(uint32_t));
         
    
      printf("\n ack_flag= %c , Next Seq num=%d  , free space= %d segments , send_ts=%d",
           dg_ack.ack_flag,dg_ack.next_seqnum,dg_ack.free_dg,dg_ack.send_ts);
    
     if (dg_ack.next_seqnum == win_start + 1)  //in order ACK received
        rtt_stop(&rttinfo,recv_ts-dg_ack.send_ts);
    
    
           
     if(dg_ack.eof_flag==1)
     {
         printf("\n ACK received for last file segment");
         break;
      }         
           
       diff_start=dg_ack.next_seqnum-win_start;
       diff_end =   (dg_ack.next_seqnum+dg_ack.free_dg-1)-(seq-1);
       //(dg_ack.next_seqnum+dg_ack.free_dg-1)  --> max sequence number the client can currently accomodate
       // (seq-1)                               --> highest sequence number that was sent out
     // If diff_start and diff_end are positive, we can advance the window by the lower of the 2 values.
     if ((diff_start>0) && (diff_end >0) )
     {
        if (diff_start < diff_end)
            diff = diff_start;
        else  
            diff = diff_end;
     }
     else
         diff =0;
         
      if (eof_seq == seq-1)
          diff=0;  //If last segment has already been transmitted, don't advance the window!   
            
        printf("\ndiff_start=%d diff_end=%d diff=%d",diff_start,diff_end,diff);
        win_start = win_start + diff ;   //advancing the window by 'diff' segments    
        
     //detect duplicate ACKs
//     if (diff_start ==0)
//     {   
       if (dg_ack.next_seqnum == prev_ack_seq)
           dupcount++;
       else 
           dupcount = 1;
           
       if (dupcount ==3)
       {
           printf("\n 3 duplicate ACKs detected. Retransmitting next expected segment...");
           retransmit(dg_ack.next_seqnum);
        }   
//     }
     
     prev_ack_seq= dg_ack.next_seqnum;   
     printf("\n prev_ack_seq set to %d",prev_ack_seq); 
         
     }
     else
     {
        printf("\n Wait for ACKs timed out.");
        diff=0;
        if( rtt_timeout(&rttinfo) ==0)  //rtt_timeout increases value of rtt_rto
           {
               if (dg_ack.free_dg ==0)
                  continue;             //no free space on client - so do not retransmit
                                        //wait with new timeout value
               else
               {   
               printf("\n Retransmitting next expected segment..."); 
               //retransmit next expected segment
               retransmit(dg_ack.next_seqnum);
               }
            }
        else
            {
            printf("\n Max number of retransmissions exceeded!");
            break;  //out of infinite for loop
            }
     
     }
     
}
 

 Close(chld_conn[chld_ind_curr].ssockfd);
 Close(sockfd);
 printf("\n Child Sockets closed!");
 return;
}


int ceiling(int n, int v){
    if(!n%v)               // If v evenly divides into n, n/v is the same as ceil(n, v), 
        return n/v;        
    else                   
        return (n/v) + 1;  
}



void retransmit(int rseq)
{
int k;

k=(rseq-1)%win_size;   
//k gives the index of the send window that contains the segment to be retransmitted.

memset(sendbuf,0,sizeof(sendbuf)); //flush file buffer
 
memcpy(&(sendbuf[0]), &(dg[k].seqnum),sizeof(int));
memcpy(&(sendbuf[11]),&(dg[k].eof_flag),sizeof(char));
memcpy(&(sendbuf[12]),&(dg[k].data),500);

Send(sockfd, sendbuf, sizeof(struct datagram), 0);  //send datagram to client
printf("\n Datagram with seq no %d retransmitted to client",dg[k].seqnum);
}



void
rtt_init(struct rtt_info *ptr)
{
	struct timeval	tv;
 unsigned long long temp1,temp2;

Gettimeofday(&tv, NULL);
 printf("\n [rtt_init] tv.tv_sec=%d  tv.tv_usec=%d ",tv.tv_sec,tv.tv_usec);
 temp1= (unsigned long long)(tv.tv_sec) * 1000;
 temp2= (unsigned long long)(tv.tv_usec) / 1000;
 ptr->rtt_base = temp1 + temp2;
 printf("\n [rtt_init]temp1=%llu  temp2=%llu  base=%llu",temp1,temp2,ptr->rtt_base);
 
 
 
 
//	ptr->rtt_base = ((uint64_t)tv.tv_sec * 1000) + ((uint64_t)tv.tv_usec / 1000);		/* # millisec since 1/1/1970 at start */
// printf("\n [rtt_init] rtt_base = %d ",ptr->rtt_base);

	ptr->rtt_rtt    = 0;
	ptr->rtt_srtt   = 0;
	ptr->rtt_rttvar = 750;  //750 millisec
	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
		/* first RTO at (srtt + (4 * rttvar)) = 3 seconds */
}


uint32_t
rtt_ts(struct rtt_info *ptr)
{
	uint32_t		ts;
	struct timeval	tv;

	Gettimeofday(&tv, NULL);
 printf("\n[rtt_ts] tv.tv_sec=%d  tv.tv_usec=%d ",tv.tv_sec,tv.tv_usec);
  ts = (uint32_t)((uint64_t)tv.tv_sec * 1000) + ((uint64_t)tv.tv_usec / 1000) - (ptr->rtt_base);
  printf("\n [rtt_ts] ts = %d",ts);
//	ts = ((tv.tv_sec - ptr->rtt_base) * 1000) + (tv.tv_usec / 1000);
	return(ts);
}




int
rtt_minmax(int rto)
{
	if (rto < RTT_RXTMIN)
		rto = RTT_RXTMIN;
	else if (rto > RTT_RXTMAX)
		rto = RTT_RXTMAX;
	return(rto);
}


void
rtt_newpack(struct rtt_info *ptr)
{
	ptr->rtt_nrexmt = 0;
}


int
rtt_start(struct rtt_info *ptr)
{
  printf("\n [rtt_start] rto = %d",ptr->rtt_rto);
	return(ptr->rtt_rto);		//returns rto value in milliseconds
		/* 4return value can be used as: alarm(rtt_start(&foo)) */
}

void
rtt_stop(struct rtt_info *ptr, uint32_t ms)
{
	double		delta;
  printf("\n[rtt_stop]: Measured rtt = %d milliseconds",ms);
	ptr->rtt_rtt = ms ;		/* measured RTT in milli-seconds */

	/*
	 * Update our estimators of RTT and mean deviation of RTT.
	 * See Jacobson's SIGCOMM '88 paper, Appendix A, for the details.
	 * We use floating point here for simplicity.
	 */

	delta = ptr->rtt_rtt - ptr->rtt_srtt;
	ptr->rtt_srtt += delta / 8;		/* g = 1/8 */

	if (delta < 0.0)
		delta = -delta;				/* |delta| */

	ptr->rtt_rttvar += (delta - ptr->rtt_rttvar) / 4;	/* h = 1/4 */

	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
}
/* end rtt_stop */


int
rtt_timeout(struct rtt_info *ptr)
{
	ptr->rtt_rto *= 2;		/* next RTO */

	if (++ptr->rtt_nrexmt > RTT_MAXNREXMT)
		return(-1);			/* time to give up for this packet */
	return(0);
}
