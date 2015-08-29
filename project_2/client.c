#include	"unpifiplus.h"
#include <math.h>

extern struct ifi_info *Get_ifi_info_plus(int family, int doaliases);
extern        void      free_ifi_info_plus(struct ifi_info *ifihead);

static void	*readthr(void *);

char servip[20], filename[20];
		 long servport,win_size;
		 int seed;
		 float drop_prob;
		 int mu;
		 float sleeptime;

struct serv_conn {
           int ssockfd;
           in_addr_t ip_addr_bound;
	         in_addr_t nw_mask;
           in_addr_t subnet_addr;
           };   //Structure with details of all IP addresses bound;
           
struct ackstr {
           char ack_flag;              //byte 1
           int next_seqnum;            //byte 2 thru 5
           short free_dg;              //bytes 6 thru 7
           };  // ACK structure
           
struct datagram {
int seqnum;                 //bytes 1 thru 4
char ack_flag;              //byte 5
int next_seqnum;            //byte 6 thru 9
short free_dg;              //bytes 10 thru 11
char eof_flag;              //byte 12
uint32_t send_ts;           //byte 13 thru 16 
char data[496];             //bytes 17 thru 512
};  // datagram structure


pthread_mutex_t lock;
struct datagram dg[10],dg_t;
//int win_size=8;
int last_read_seq_num,running_ind,next_exp_seq_num,eof_seq,sel;
short free_segs,final_read=0;
char sendbuf[512];
struct serv_conn conn[10];
int ret_drop;


int
main(int argc, char **argv)
{
	struct ifi_info	*ifi, *ifihead;
	struct sockaddr	*sa;
	struct sockaddr_in	*sain, servaddr;
	u_char		*ptr;
	int		i,j,buf_pos, family, doaliases,servport,sockfd,maxfd;
 int last_in_order;
 
 	const int			on = 1;
  fd_set		rset;
  char recvbuf[512],temp[513],mainbuf[5120];
//  char servip[]="130.245.1.184";
  socklen_t len;
  fd_set		fdset;
  struct timeval tv;
  struct ackstr ack;
  int segdrop(float);

 
  ssize_t bytes_read;
  pthread_t		tid;
  
  FILE *ppFile;
	
	
	
int ind,ind_used,in_order;


len=sizeof(struct sockaddr_in);

int cliport=9890; 

ppFile=fopen("client.in","r");

		 if(ppFile !=NULL)
		 {
		 
		 fscanf(ppFile, "%s %ld %s %ld %d %f %ld",servip,&servport,filename, &win_size, &seed, &drop_prob, &mu);
		   
		   printf("\nRead from client.in >>> Server IP address |%s|",servip);
		   printf("\nRead from client.in >>> Server's well known port number |%ld|",servport);
		   printf("\nRead from client.in >>> Filename to be transferred |%s|",filename);
		   printf("\nRead from client.in >>> Receiving Sliding window size |%ld|",win_size);
		   printf("\nRead from client.in >>> Random Generator Seed value |%d|",seed);
		   printf("\nRead from client.in >>> Probability of datagram loss|%f|",drop_prob);
		   printf("\nRead from client.in >>> mu in milliseconds|%ld|\n",mu);

		 fclose(ppFile);
		 }
		 
		 else
		 {
		 printf("\n Could not open the file client.in");
		 exit(0);
     }

if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
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
		    sain->sin_port = htons(cliport);
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
 
 //Convert Server IP address from presentation to network format
 
     bzero(&servaddr, sizeof(struct sockaddr_in));
     inet_pton(AF_INET, servip, &servaddr.sin_addr);
     servaddr.sin_family = AF_INET;
     servaddr.sin_port=servport;
     printf("\n Server address= %s ",Sock_ntop((SA *) &servaddr, sizeof(servaddr)));
 
 
  FD_ZERO(&rset);
  maxfd=0;
  sel = -1;
 for (ind=0;ind<=ind_used;ind++) 
 {
     printf("\n Structure # %d\n",ind);
     printf("\n sockfd= %d",conn[ind].ssockfd);
 //    FD_SET(conn[ind].ssockfd, &rset);
 //    if  (conn[ind].ssockfd > maxfd)
 //        maxfd = conn[ind].ssockfd;
     
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
     
     //check if the current interface and the server belong to the same subnet by XORing 
     //the subnet address of server with subnet address of client
     
     if ( ( (servaddr.sin_addr.s_addr & conn[ind].nw_mask) ^ conn[ind].subnet_addr ) ==0)
     {
        printf("\n Server address is local to the current client IP interface");
        printf("\n (servaddr.sin_addr.s_addr & conn[ind].nw_mask) = %d",servaddr.sin_addr.s_addr & conn[ind].nw_mask);
                
        sel = ind;
        setsockopt(conn[sel].ssockfd, SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on));
      }  
      
 }
 
    if (sel == -1)
    {
       printf("\n Server address is not local to any of the client IP addresses");
       sel = ind_used;
     }  
       
       
     
     printf("\n maxfd= %d ",maxfd);	
     
    	
     printf("\nEntering main select...");
     
 //    Select(maxfd+1, &rset, NULL, NULL, NULL);  
  //blocks until there is an incoming client
	//	printf("Exited main select");
 
 //    sock_pton(servip,(SA *)servaddr);
  
     
 strcpy(sendbuf,filename); 
     
 sendagain:
 Sendto(conn[sel].ssockfd, sendbuf, sizeof(sendbuf), 0, (SA *) &servaddr, sizeof(servaddr));
 printf("\n Handshake 1 of 3 sent from client to server");
 
 FD_ZERO(&fdset);
 FD_SET(conn[sel].ssockfd, &fdset);
 tv.tv_sec=5;
 tv.tv_usec=0;
 printf("\nWaiting for Handshake 2 from server....");
 Select(conn[sel].ssockfd+1, &fdset, NULL, NULL, &tv);    //wait for 5 seconds for ACK from server 
 if (FD_ISSET(conn[sel].ssockfd, &fdset))
 {
      printf("\n Handshake 2 of 3 from server received!");
      //get port number of server's connection socket
      Recvfrom(conn[sel].ssockfd, recvbuf, sizeof(recvbuf), 0, (SA *) &servaddr, &len);
      memcpy(&(servaddr.sin_port), recvbuf, sizeof(in_port_t));
      printf("\n Server's new ephemeral port number=%d", servaddr.sin_port);
      
      //Connect to new socket on server
     Connect(conn[sel].ssockfd, (SA *) &servaddr, sizeof(servaddr));
     printf("\n Socket connected to server IP %s",Sock_ntop((SA *) &servaddr, sizeof(servaddr)));
     //send 3rd handshake
     ack.ack_flag='A';
     ack.next_seqnum=0;
     ack.free_dg=win_size;
     memset(sendbuf,0,sizeof(sendbuf)); //flush buffer    
     memcpy(sendbuf, &ack, sizeof(struct ackstr));
     //Sendto(conn[sel].ssockfd, sendbuf, sizeof(sendbuf), 0, (SA *) &servaddr, sizeof(servaddr));
     Send(conn[sel].ssockfd, sendbuf, sizeof(sendbuf), 0);
     printf("\n Sent Handshake 3 from client to server");
  }
 else
  {
     printf("\n Timed out!");
     goto sendagain;
   }
 
 
Pthread_create(&tid, NULL, &readthr, NULL); 
 
free_segs=win_size;
memset(dg,0,win_size*sizeof(struct datagram)); //flush dg array 
last_read_seq_num=0;   //to be updated by read thread
next_exp_seq_num=1;
running_ind=0;         //to be updated by read thread
eof_seq=-1;
//when read thread can consume the buffer only partially due to a gap, need to move the unread
//portion to the left and reset running_ind 
srand48(seed);
for(;;)
{ 
  //Wait for file segments to  come  in 
   FD_ZERO(&fdset);
   FD_SET(conn[sel].ssockfd, &fdset);
   printf("\n Waiting for file segments from server...");
   tv.tv_sec=30;
   tv.tv_usec=0;
   Select(conn[sel].ssockfd+1, &fdset, NULL, NULL, &tv);
   if (FD_ISSET(conn[sel].ssockfd, &fdset))
   {
        ret_drop=segdrop(drop_prob);
        if ( ret_drop ==0)
       {
            printf("\n Incoming segment dropped based on random number generator");
            Recv(conn[sel].ssockfd, recvbuf, sizeof(recvbuf), 0);
            memset(recvbuf,0,sizeof(recvbuf)); //flush buffer 
        } 

       else
       { 
     
            //printf("\n File segments from server received!");
            memset(recvbuf,0,sizeof(recvbuf)); //flush buffer 
            bytes_read=Recv(conn[sel].ssockfd, recvbuf, sizeof(recvbuf), 0);
            //printf("\n Bytes Read =%d",bytes_read);
            
            memset(&dg_t,0,sizeof(struct datagram)); //flush temp dg 
            
            //copy incoming datagram to temp dg from recvbuf
            memcpy( &(dg_t.seqnum),      &(recvbuf[0]),   sizeof(int));
           // memcpy( &(dg_t.ack_flag),    &(recvbuf[4]),   sizeof(char));
           // memcpy( &(dg_t.next_seqnum), &(recvbuf[5]),   sizeof(int));
           // memcpy( &(dg_t.free_dg),     &(recvbuf[9]),   sizeof(short));
            memcpy( &(dg_t.eof_flag),    &(recvbuf[11]),  sizeof(char));
            memcpy( &(dg_t.send_ts),    &(recvbuf[12]),  sizeof(uint32_t));
            memcpy( &(dg_t.data),        &(recvbuf[16]),   496);
            
            //printf("\n Seq num of incoming segment=%d",dg_t.seqnum);
            
            //printf("\n Main thread - Attempting to set lock");
            pthread_mutex_lock(&lock);
            //printf("\n Lock set by main thread");
            
            buf_pos= running_ind + (dg_t.seqnum - next_exp_seq_num);
            
            if (buf_pos > win_size - 1)
               printf("\n Out of buffer space - segment discarded!");
            else
            //if ( (dg_t.seqnum >= next_exp_seq_num + free_segs) || (dg_t.seqnum < next_exp_seq_num)  )
            //   printf("\n Out of buffer space - segment discarded!");
            //else   
            {
                        free_segs--;
                        //free_segs=win_size-(buf_pos+1);  //space left for new segments, excluding gaps
                                     
                     //copy incoming datagram to correct position on main buffer array   
                     memcpy( &(dg[buf_pos].seqnum),      &(recvbuf[0]),   sizeof(int));
                     //   memcpy( &(dg[buf_pos].ack_flag),    &(recvbuf[4]),   sizeof(char));
                     //   memcpy( &(dg[buf_pos].next_seqnum), &(recvbuf[5]),   sizeof(int));
                     //   memcpy( &(dg[buf_pos].free_dg),     &(recvbuf[9]),   sizeof(short));
                     memcpy( &(dg[buf_pos].eof_flag),    &(recvbuf[11]),  sizeof(char));
                     memcpy( &(dg[buf_pos].send_ts),    &(recvbuf[12]),  sizeof(uint32_t));
                     memcpy( &(dg[buf_pos].data),        &(recvbuf[16]),   496);
                      
                    printf("\n buf_pos=%d  running_ind=%d",buf_pos,running_ind);   
                  
                     if(dg_t.seqnum == next_exp_seq_num)
                     {
                  //     running_ind++;
                       in_order=1;    //in_order flag set
                     } 
                     else
                       in_order=0;
                       
                     if (dg_t.eof_flag ==1)
                         eof_seq=dg_t.seqnum;
                  
            }
             
            
             //When read thread empties buffer, reset running_ind appropriately
             //running_ind is the position where the next expected segment will be stored.
        
             //Loop thru the main buffer to determine next expected sequence number
             for(j=0;j<=win_size;j++)
             {
                if (dg[j].seqnum != 0)
                   last_in_order=dg[j].seqnum;
                else  //gap detected
                   {
                      running_ind =j;
                      if (j==0)
                        next_exp_seq_num=last_read_seq_num+1;
                      else
                      {  
                        next_exp_seq_num=last_in_order+1;
                        break;
                      }
                   }
             }   
             
     
               //format outgoing acknowledgement by updating temp dg
               dg_t.seqnum = 0;
               dg_t.ack_flag='A';
               dg_t.next_seqnum=next_exp_seq_num;
               dg_t.free_dg=free_segs;
               //dg_t.free_dg=10;  //just for testing
            
               memset(&(dg_t.data),0,496); //clear out data
               
               
               //copy temp dg to send buffer
                memcpy( &(sendbuf[0]),  &(dg_t.seqnum),     sizeof(int));
                memcpy( &(sendbuf[4]),  &(dg_t.ack_flag),   sizeof(char));
                memcpy( &(sendbuf[5]),  &(dg_t.next_seqnum),sizeof(int));
                memcpy( &(sendbuf[9]),  &(dg_t.free_dg),    sizeof(short));
                memcpy( &(sendbuf[11]), &(dg_t.eof_flag),   sizeof(char));
                memcpy( &(sendbuf[12]), &(dg_t.send_ts),    sizeof(uint32_t));
                memcpy( &(sendbuf[16]), &(dg_t.data),       496);
          /*       
                 printf("\n size=%d dg[%d].seqnum= %d",     sizeof(dg[i].seqnum),      i,dg[i].seqnum);
                 printf("\n size=%d dg[%d].ack_flag= %c",   sizeof(dg[i].ack_flag),    i,dg[i].ack_flag);
                 printf("\n size=%d dg[%d].next_seqnum= %d",sizeof(dg[i].next_seqnum), i,dg[i].next_seqnum);
                 printf("\n size=%d dg[%d].free_dg= %d",    sizeof(dg[i].free_dg),     i,dg[i].free_dg);
                 printf("\n size=%d dg[%d].eof_flag= %c",   sizeof(dg[i].eof_flag),    i,dg[i].eof_flag);
         */            
           //      printf("\n recvbuf[12]= %c, recvbuf[13]= %c , recvbuf[14]=%c",recvbuf[12],recvbuf[13],recvbuf[14]);
           //      printf("\n recvbuf[509]= %c, recvbuf[510]= %c , recvbuf[511]=%c",recvbuf[509],recvbuf[510],recvbuf[511]);
                 memcpy(temp,&(dg[buf_pos].data),496);
                 temp[496]=0;
                 printf("\n Received datagram with sequence number =%d ",dg[buf_pos].seqnum);
                 Send(conn[sel].ssockfd, sendbuf, sizeof(sendbuf), 0);
                 printf("\n Sent out ACK %d and free space=%d",next_exp_seq_num,free_segs); 
                 printf("\n----------------");
            pthread_mutex_unlock(&lock);
            //printf("\n Main thread - lock released!");
            
              if (next_exp_seq_num == eof_seq + 1)
              {
                 printf("\n EOF received!");
                 break;
               }  
         }   
     }
     else   //for select 
       printf("\n Timed out!");
 }          // for infinite loop
 
 
 
 printf("\n Entering Time-wait state...");
 //if no more messages are received from server within the timeout period, 
 //program will exit.
   FD_ZERO(&fdset);
   FD_SET(conn[sel].ssockfd, &fdset);
   tv.tv_sec=10;
   tv.tv_usec=0;
   Select(conn[sel].ssockfd+1, &fdset, NULL, NULL, &tv);
   if (FD_ISSET(conn[sel].ssockfd, &fdset))
   {
       //Resend latest ACK   
       Send(conn[sel].ssockfd, sendbuf, sizeof(sendbuf), 0);
       printf("\n Sent out ACK %d and free space=%d",next_exp_seq_num,free_segs); 
   }  
   else
   printf("\n End of time wait state");  
 
//check if the read thread had a chance to read the buffer *after* the EOF was received.
for(;;)
{
sleep(3);
printf("\n Final read=%d",final_read);
 if(final_read==1)
  {
	  Close(conn[sel].ssockfd);
    printf("\n Sockets closed...Exiting program");
    break;  //out of infinite loop
  }
}  

exit(0);
}


static void *
readthr(void *arg)
{
int read_i,copy_i;
short gap,ack_flag=0;
char tempstr[501];
fd_set		fdset2;
struct timeval tv2;
Pthread_detach(pthread_self());
int sleep_time(int,int);
int st;

srand48(seed);
for(;;)
{

   printf("\n Read thread going to sleep...");
   st=sleep_time(mu,seed);
   tv2.tv_sec = (int)(st/1000);  //convert ms value to seconds
   tv2.tv_usec= (st%1000)*1000;   //convert remaining milliseconds to microseconds
   printf("\n Sleep time set to %d seconds and %d microseconds",tv2.tv_sec,tv2.tv_usec);
   FD_ZERO(&fdset2);
//   tv2.tv_sec=10;
//   tv2.tv_usec=0;
   Select(1, &fdset2, NULL, NULL, &tv2);
printf("\n Read thread now awake!");
//printf("\n Read thread attempting to set lock");
pthread_mutex_lock(&lock);
//printf("\n Read thread - lock set; now active!");

if ( (free_segs ==0) && (next_exp_seq_num != eof_seq + 1) )
  ack_flag=1;  //if buffer is full, need to send out a new ACK after the buffer is emptied.
  
for(read_i=0,copy_i=0,gap=0;read_i<win_size;read_i++)
{
   if ( (dg[read_i].seqnum != 0)  && (gap==0) )
   {
      printf("\n Reading datagram with sequence number %d buffer pos %d",dg[read_i].seqnum,read_i);
      memcpy(tempstr,&(dg[read_i].data),496);
      tempstr[496]=0;
      printf("\n--------------------------------------------------------");
      printf("\n %s",tempstr);
      printf("\n--------------------------------------------------------");
      last_read_seq_num = dg[read_i].seqnum;
      free_segs++;
    }
    
    
    if ( (dg[read_i].seqnum == 0) || (gap ==1))  
    {
        dg[copy_i].seqnum=dg[read_i].seqnum;
        memcpy( &(dg[copy_i].data), &(dg[read_i].data), 496);
        gap=1;  //Gap detected
 //       printf("\n Copied data from buffer position %d to %d",read_i,copy_i);
        copy_i++;
    }
    
    
}
printf("\n No more segments to read");

      running_ind=0;  //next in-order segment will come in at position running_ind
   
//printf("\n Running Index set to %d",running_ind);

for(;copy_i<win_size;copy_i++)
{
 dg[copy_i].seqnum=0;
 memset(&(dg[copy_i].data),0,496); //flush temp dg 
 //printf("\n Emptied buffer position %d",copy_i);
}

if (ack_flag ==1)
{
  //send ACK
//  skipped_ack=0;
  //format outgoing acknowledgement by updating temp dg
  dg_t.seqnum = 0;
  dg_t.ack_flag='A';
  dg_t.next_seqnum=next_exp_seq_num;
  dg_t.free_dg=free_segs;
  //dg_t.free_dg=10;  //just for testing
  memset(&(dg_t.data),0,496); //clear out data
  
  
  //copy temp dg to send buffer
   memcpy( &(sendbuf[0]),  &(dg_t.seqnum),     sizeof(int));
   memcpy( &(sendbuf[4]),  &(dg_t.ack_flag),   sizeof(char));
   memcpy( &(sendbuf[5]),  &(dg_t.next_seqnum),sizeof(int));
   memcpy( &(sendbuf[9]),  &(dg_t.free_dg),    sizeof(short));
   memcpy( &(sendbuf[11]), &(dg_t.eof_flag),   sizeof(char));
   memcpy( &(sendbuf[12]), &(dg_t.send_ts),   sizeof(uint32_t));
   memcpy( &(sendbuf[16]), &(dg_t.data),       496);
   Send(conn[sel].ssockfd, sendbuf, sizeof(sendbuf), 0);
   printf("\n Sent out ACK %d and free space=%d",next_exp_seq_num,free_segs);
   ack_flag = 0;  //reset ack_flag for next iteration
}       

if (next_exp_seq_num == eof_seq + 1)
{
   printf("\n End of file - Final iteration of read thread");
   final_read=1;
   pthread_mutex_unlock(&lock);
   break;
}   
           
pthread_mutex_unlock(&lock);
//printf("\n Read thread - lock released");

} 

}   



 int sleep_time(int mu1,int seed1)
		 {
//		 srand48(seed1);
     double drand = drand48();
//	   printf("value of drand %f\n",drand);
		 double st=(log(drand))*mu1*(-1);
//		 printf("value of st is %f\n",st);
		 return ((int)st); 
				 
		 }
      
      
      int segdrop(float p)
		 {
//		 srand48(seed1);
     double drand = drand48();
//	   printf("value of drand %f\n",drand);
		 if (drand < p) 
        return 0;  //drop segment
      else
        return 1;  //keep segment
    		 
		 }
    
      
  







