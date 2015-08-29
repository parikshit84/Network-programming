#include	"unp.h"
#include	"hw_addrs.h"

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#define ODR_PATH "/tmp/odr_pb.dg"   // well known path for ODR service 
#define SRV_PATH "/tmp/srv_pb.dg"   // well known path for server
#define USID_PROTO 0xAA83
#define MY_ETH_FRAME_LEN 1000
#define MY_PACKET_OTHERHOST 3
#define MY_ETH_ALEN 6

//void walkthru_ints(void);

struct odrpayload_info {
        short msgtype;
        char senderip[15];
        int senderport;
        char destip[15];
        int destport;
        char hopcount;
        char sendervm[5];
        int broadcast_id;
        int rrep_sent;
        char req_service[10];
        char msg_body[30];
        };
        
struct routing_tbl_info {
       char nodeip[15];
       char nexthop_haddr[6];
       int out_array_ind;
       int hopcount;
       int broadcast_id;
       time_t timestamp;
       };         
        
        
void build_eth_frame(char *,char *,int,int,struct odrpayload_info);
int update_routing_tbl(char *,char *,int,int,int,time_t);
void print_routing_tbl(void);
struct routing_tbl_info check_routing_tbl(char *,time_t);
void propagate_rreq(void * ,int, int );
void generate_rrep(void * ,int,int ); 

int max_ind;


struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;
	char   *ptr;
	int    i, prflag,psd1,psd2,error;
 socklen_t errlen=sizeof(error);
 
 
struct my_intfs {
           int psockfd;     
           int my_if_index;
           char my_if_haddr[IF_HADDR];
                 };  

struct  sockaddr  *my_eth0_ip_addr_n;	/* eth0 IP address in network byte format */
char my_eth0_ip_addr_p[15];  // eth0 IP address in presentation format 
char my_host_name[5];

struct hostent *hptr;
struct in_addr sinaddr;


        
      
        
        
struct routing_tbl_info routing_tbl[20];
char all_dest_mac[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
struct my_intfs my_intfs_list[10];
char src_host_name[5],dest_host_name[5];



 

int
main(int argc, char **argv)
{
	int					sockfd,n,j,better_rt_ind,ind_lookup,ind_lookup2,ind_lookup3;
	struct sockaddr_un	odraddr, cliaddr, srvaddr;
	char		mesg[MAXLINE],indestip[20],tstmsgbody[30];
	socklen_t	clilen=sizeof(cliaddr);
 struct odrpayload_info odrpayload;
int bcast_id = 0;
struct routing_tbl_info routing_tbl_lookup,routing_tbl_lookup2,routing_tbl_lookup3;
int srvlen;
char servbuff[100],clibuff[100];

 
 
  printf("\nODR service started");
  
  memset(routing_tbl, 0, sizeof(struct routing_tbl_info)*20 );  //initialize all rows of routing tbl to 0
   
 
	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
// printf("\n sockfd=%d",sockfd);
 
 //psd1 = socket (PF_PACKET, SOCK_RAW,  htons(ETH_P_ALL)  );
 //psd2 = socket (AF_PACKET, SOCK_RAW,  htons(ETH_P_ALL)  );
 //printf("\n psd1=%d, psd2=%d",psd1,psd2);

	unlink(ODR_PATH);
//  printf("\n Unlink done");
	bzero(&odraddr, sizeof(odraddr));
	odraddr.sun_family = AF_LOCAL;
	strcpy(odraddr.sun_path, ODR_PATH);

//  printf("\n starting Bind"); 
	Bind(sockfd, (SA *) &odraddr, sizeof(odraddr));
 printf("\n Bind done successfuly");
 
 /*
  struct sockaddr_un	srvaddr;
  bzero(&srvaddr, sizeof(srvaddr));	
  srvaddr.sun_family = AF_LOCAL;
  strcpy(srvaddr.sun_path, SRV_PATH);
  
  int srvlen=sizeof(srvaddr);   
  strcpy(tstmsgbody,"Test msg from ODR to server!");
 //Send test msg to server from ODR
  Sendto(sockfd, tstmsgbody, strlen(tstmsgbody), 0, (SA *) &srvaddr, srvlen); 
  printf("\n Test msg sent to server");
 
 */


////////////////////////////////////////////////////////////////////////////////////

int sd,gso,ind,maxfd;

char   *my_ptr;
fd_set rset;

 struct odrpayload_info recv_odrpayload;

error=0;

printf("\n");

	for (hwahead = hwa = Get_hw_addrs(),ind=0; hwa != NULL; hwa = hwa->hwa_next) {
		
		printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
		
		if ( (sa = hwa->ip_addr) != NULL)
			printf("         IP addr = %s\n", Sock_ntop_host(sa, sizeof(*sa)));
				
		prflag = 0;
		i = 0;
		do {
			if (hwa->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		if (prflag) {
			printf("         HW addr = ");
			ptr = hwa->if_haddr;
			i = IF_HADDR;
			do {
				printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);
		}

		printf("\n         interface index = %d\n\n", hwa->if_index);
   
   
//define a PF_packet socket if current interface is not lo or eth0 
//printf("\n Current interface= %s\n", hwa->if_name); 

     if(  strcmp(hwa->if_name,"eth0") == 0)
        my_eth0_ip_addr_n= hwa->ip_addr;
      



    if( (strcmp(hwa->if_name,"lo")) && (strcmp(hwa->if_name,"eth0")) != 0)
    {
//       printf("\n Attempting to create PF packet socket");  
//       sd = socket (PF_PACKET, SOCK_RAW,  htons(ETH_P_ALL)  );
         sd = socket (PF_PACKET, SOCK_RAW,  htons(USID_PROTO)  );
//       printf("Created PF_PACKET socket with return code %d",sd);
       gso=getsockopt(sd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen);
//       printf("\n Error number %d, gso return code=%d",error,gso);
       my_intfs_list[ind].psockfd=sd;
       
       
 //Bind socket created above to the current interface ! (Very important step!)
 struct sockaddr_ll sall,sall2;
bzero(&sall , sizeof(sall));
sall.sll_family   = AF_PACKET;
sall.sll_protocol = htons(USID_PROTO);
sall.sll_ifindex  = hwa->if_index;
int bind_ret_code=bind(sd , (struct sockaddr *)&sall , sizeof(sall));
//printf("\n bind_ret_code=%d",bind_ret_code);
//send test--------------------
memcpy(&sall2,&sall,sizeof(sall));
sall2.sll_hatype   = ARPHRD_ETHER;
sall2.sll_pkttype  = PACKET_BROADCAST; 
sall2.sll_halen    = 6 ;	

/*MAC - begin*/
memcpy(&(sall2.sll_addr[0]),all_dest_mac,6);
/*sall2.sll_addr[0]  = 0xff;
sall2.sll_addr[1]  = 0xff;
sall2.sll_addr[2]  = 0xff;
sall2.sll_addr[3]  = 0xff;
sall2.sll_addr[4]  = 0xff;
sall2.sll_addr[5]  = 0xff;*/
socklen_t sall2len = sizeof(struct sockaddr_ll);

unsigned char* sbuff = malloc(MY_ETH_FRAME_LEN);

memcpy(&(sbuff[0]),all_dest_mac,6);
memcpy(&(sbuff[6]),hwa->if_haddr,6);
sbuff[12]=0xAA;
sbuff[13]=0x83;
/*send the packet*/
//int send_ret = sendto(sd, sbuff, MY_ETH_FRAME_LEN, 0,(struct sockaddr*)&sall2, sall2len);
//printf("\n Immediate Send result=%d",send_ret);
//int sentbytes=write(sd, sbuff, MY_ETH_FRAME_LEN);
//printf("\n sentbytes =%d",sentbytes);
//printf( "\nWrite error message: %s\n", strerror( errno ) );
 
      
       
 //Copy interface index and MAC address into my_intfs_list array
       my_intfs_list[ind].my_if_index=hwa->if_index;
       memcpy(my_intfs_list[ind].my_if_haddr,hwa->if_haddr,IF_HADDR);
       ind++;
    }   
	}
 
//convert eth0 IP address from network byte order to presentation format 
strcpy(my_eth0_ip_addr_p,Sock_ntop( my_eth0_ip_addr_n, sizeof(*my_eth0_ip_addr_n)));
printf("\n My eth0 IP address=%s",my_eth0_ip_addr_p);

//memcpy(&sinaddr,my_eth0_ip_addr_n->sa_data,4);
//hptr = gethostbyaddr(&sinaddr, sizeof(sinaddr), AF_INET);
//printf("\n My Host name: %s", hptr->h_name);

inet_pton(AF_INET, my_eth0_ip_addr_p, &sinaddr);
hptr = gethostbyaddr(&sinaddr, sizeof(sinaddr), AF_INET);
strcpy(my_host_name,hptr->h_name);
printf("My Host name: %s\n", my_host_name);


/*
 for(;;)   //keep waiting for incoming connections from client
 {
 printf("\nWaiting for incoming connections");
 printf("\n calling Recvfrom...");
 
 //Recvfrom(sockfd,mesg,MAXLINE,0,(SA *) &cliaddr, &clilen);
 //sleep(3);
 n=Recvfrom(sockfd, mesg, MAXLINE, 0, (SA *) &cliaddr, &clilen);
 printf("\n %d bytes received from client!Client addr is %s Msg is %s",n,cliaddr.sun_path,mesg);
 
 memset(mesg,0,sizeof(mesg));
 }
*/ 
 
max_ind = ind-1;


for (;;) {   //infinite loop begins here
FD_ZERO(&rset);
maxfd=0;

//Add the Unix SOCK_DGRAM socket to the select read set
FD_SET(sockfd, &rset);
maxfd=sockfd; 
printf("\n added socket %d to Select readset",sockfd);

//printf("\n Walking thru my_intfs_list array...");
for (ind=0;ind<=max_ind;ind++)
{
/*
    printf("\n Interface # %d ; MAC address  ",my_intfs_list[ind].my_if_index);
    
    my_ptr = my_intfs_list[ind].my_if_haddr;
    			i = IF_HADDR;
    			do {
    				printf("%.2x%s", *my_ptr++ & 0xff, (i == 1) ? " " : ":");
    			} while (--i > 0);
    printf("  PF PACKET sockfd = %d",my_intfs_list[ind].psockfd);
*/    

    
    FD_SET(my_intfs_list[ind].psockfd, &rset);  //add current PF SOCKET fd to select rset
    printf("\n Added socket %d to select readset",my_intfs_list[ind].psockfd);
    
    if  (my_intfs_list[ind].psockfd > maxfd)
         maxfd = my_intfs_list[ind].psockfd;

}

   printf("\nEntering select to monitor sockets for readability...");
   Select(maxfd+1, &rset, NULL, NULL, NULL);  
     //blocks until there is an incoming client
   printf("Exited main select");
   
   
   //check if select was trigerred by the SOCK_DGRAM socket
if (FD_ISSET(sockfd, &rset))
  {
    n=Recvfrom(sockfd, mesg, MAXLINE, 0, (SA *) &cliaddr, &clilen);
    printf("\n %d bytes received from client!Client addr is %s Msg is %s",n,cliaddr.sun_path,mesg);
    
    memcpy(indestip,mesg,14);  //copy ip address of destination from 1st 14 bytes of mesg 
    indestip[14]=0;  //add null terminator
    
    strcpy(odrpayload.senderip,my_eth0_ip_addr_p);
    strcpy(odrpayload.destip,indestip);
    odrpayload.hopcount=0;    
    
    odrpayload.msgtype = mesg[20];
    strcpy(odrpayload.msg_body,mesg+21);
    memcpy(&odrpayload.destport,(int *)(mesg+15),sizeof(int));
    printf("\n Destport is %d",odrpayload.destport);
    
    routing_tbl_lookup=check_routing_tbl(indestip,time(NULL));
    
    if (  strlen(routing_tbl_lookup.nodeip) ==0        ) //entry for this node does not exist
    {
      //generate RREQs
        bcast_id++;
              
        odrpayload.broadcast_id=bcast_id;
        
        for (ind=0;ind<=max_ind;ind++)
        build_eth_frame(my_intfs_list[ind].my_if_haddr,   all_dest_mac,    
                        my_intfs_list[ind].my_if_index,  my_intfs_list[ind].psockfd , odrpayload);
    }
    else
    { 
    printf("\n Sending message out only on one interface");
    //forward msg to appropriate interface  
    
    ind_lookup=routing_tbl_lookup.out_array_ind;  //array index of destination node
    
    build_eth_frame(my_intfs_list[ind_lookup].my_if_haddr,   routing_tbl_lookup.nexthop_haddr,    
                        my_intfs_list[ind_lookup].my_if_index,  my_intfs_list[ind_lookup].psockfd , odrpayload);
    }
  }
else
 { 

   //walk thru all pF packet sockets to determine which one triggered the select
   for (ind=0;ind<=max_ind;ind++)
   {
      if (FD_ISSET(my_intfs_list[ind].psockfd, &rset))
      {
      printf("\n Incoming connection on socket # %d",my_intfs_list[ind].psockfd);
      void * recvbuff = (void *)malloc(MY_ETH_FRAME_LEN);
      int recvbytes = recvfrom(my_intfs_list[ind].psockfd,recvbuff,MY_ETH_FRAME_LEN,0,NULL,NULL);
      printf("\n recvbytes=%d",recvbytes);
      printf("\n Contents of received msg:");
      
      char * rcv_ptr;
      int rcv_i;
      
      printf("\n Recv buffer eth_dest_mac= ");
      rcv_ptr = (char *) recvbuff;
   		rcv_i = IF_HADDR;
    			do {
    				printf("%.2x%s", *rcv_ptr++ & 0xff, (rcv_i == 1) ? " " : ":");
    			} while (--rcv_i > 0);



       printf("\n Recv buffer eth_src_mac= ");
       rcv_ptr = (char *) (recvbuff+6) ;
       rcv_i = IF_HADDR;
    			do {
    				printf("%.2x%s", *rcv_ptr++ & 0xff, (rcv_i == 1) ? " " : ":");
    			} while (--rcv_i > 0);    
      
 //     printf("\n char 13 and 14 of recvbuff = %hhx %hhx", *( (char *)(recvbuff+12)) ,* ((char *)(recvbuff+13)) );
      
      //printf("\n data payload=%s",(char *)recvbuff+14);
      
      memcpy(&recv_odrpayload,(char *)recvbuff+14,sizeof(struct odrpayload_info));
      
      printf("\n recv_odrpayload.msgtype = %d",recv_odrpayload.msgtype);
      printf("\n recv_odrpayload.senderip = %s",recv_odrpayload.senderip);
      printf("\n recv_odrpayload.destip = %s",recv_odrpayload.destip);
      printf("\n recv_odrpayload.hopcount = %d",recv_odrpayload.hopcount);
      printf("\n recv_odrpayload.sendervm = %s",recv_odrpayload.sendervm);
      
      better_rt_ind=0;
    //  if ( strcmp(recv_odrpayload.senderip,my_eth0_ip_addr_p) != 0 ) //dont add your own IP to the dest table
    //  better_rt_ind=update_routing_tbl(recv_odrpayload.senderip,(char *)recvbuff+6,ind,recv_odrpayload.hopcount+1,time(NULL));
      //add or update entry on dest table for reverse route
      
      if ( (recv_odrpayload.msgtype == 0) && ( strcmp(recv_odrpayload.senderip, my_eth0_ip_addr_p) != 0 ) ) //ignore loopback RREQs
      { 
          printf("\n-------\n Valid RREQ detected!\n-------");
          print_routing_tbl();
          
         //add or update entry on routing table for reverse route to sender
         better_rt_ind=update_routing_tbl(recv_odrpayload.senderip,(char *)recvbuff+6,ind,
                                       recv_odrpayload.hopcount+1,recv_odrpayload.broadcast_id,time(NULL));
          
           routing_tbl_lookup2=check_routing_tbl(recv_odrpayload.destip,time(NULL));                         
          //LOGIC to generate RREPs:
         //check if destination node is THIS node 
         if (   (strcmp(recv_odrpayload.destip,my_eth0_ip_addr_p) == 0) && (recv_odrpayload.rrep_sent != 1)  )
          {
            printf("\n Packet meant for me!");
            generate_rrep(recvbuff,ind,routing_tbl_lookup2.hopcount);   //send RREP out on the interface the RREQ came in on
                if (better_rt_ind ==1)  //discovered a better route to the source
             {
                printf("\n RREP already sent , but discovered a new or better route to the source - so propagating RREQs"); 
                propagate_rreq(recvbuff,ind,1);  
              }    
          }
         else if (  (strlen(routing_tbl_lookup2.nodeip)  != 0 )  && (recv_odrpayload.rrep_sent != 1)  ) 
              //routing table has an unexpired route to the sought destination
          {
            printf("\nDest node exists on routing table");
            generate_rrep(recvbuff,ind,routing_tbl_lookup2.hopcount);   //send RREP out on the interface the RREQ came in on
                if (better_rt_ind ==1)  //discovered a better route to the source
             {
                printf("\n RREP already sent , but discovered a new or better route to the source - so propagating RREQs"); 
                propagate_rreq(recvbuff,ind,1);  
              }    
            
          } 
          
          else
          {
            printf("\n RREPs not sent, and propagating RREQ");
            propagate_rreq(recvbuff,ind,0);  
          }  
            
            
     printf("\n End of RREQ handling");       
      print_routing_tbl();
      
      }    //end of RREQ handling         
         
      
     else  if  (recv_odrpayload.msgtype == 1)     //RREPs
       {
         printf("\n Valid RREP detected!");
         print_routing_tbl();
         //add or update entry on routing table for forward route to destination
         update_routing_tbl(recv_odrpayload.destip,(char *)recvbuff+6,ind,
                                       recv_odrpayload.hopcount+1,recv_odrpayload.broadcast_id,time(NULL));
                                       
       
            if ( strcmp(recv_odrpayload.senderip, my_eth0_ip_addr_p) != 0  )   //The RREP is meant for another node
            {
              printf("\n RREP is meant for a different node");
              routing_tbl_lookup3=check_routing_tbl(recv_odrpayload.senderip,time(NULL));
              
              ind_lookup3=routing_tbl_lookup3.out_array_ind;  //array index of destination node
              
              recv_odrpayload.hopcount=recv_odrpayload.hopcount+1;
    
              build_eth_frame(my_intfs_list[ind_lookup3].my_if_haddr,   routing_tbl_lookup3.nexthop_haddr,    
                              my_intfs_list[ind_lookup3].my_if_index,  my_intfs_list[ind_lookup3].psockfd , recv_odrpayload);
            }
            else
            {
            printf("\n The RREP is meant for me!");
            //Now, send the application message to the destination.
            routing_tbl_lookup3=check_routing_tbl(recv_odrpayload.destip,time(NULL));
              
              ind_lookup3=routing_tbl_lookup3.out_array_ind;  //array index of destination node
              
              recv_odrpayload.msgtype = 2;   //Application payload message
              recv_odrpayload.hopcount= 0;
              recv_odrpayload.destport = 1000; //well known port number of server at destination
              strcpy(recv_odrpayload.req_service,"time");  //indicates that time service is requested on the server
    
              build_eth_frame(my_intfs_list[ind_lookup3].my_if_haddr,   routing_tbl_lookup3.nexthop_haddr,    
                              my_intfs_list[ind_lookup3].my_if_index,  my_intfs_list[ind_lookup3].psockfd , recv_odrpayload);
            
            }
            printf("\n End of RREP handling");
            print_routing_tbl();
            
       }   // end of RREP handling
       
       
    else   if  (recv_odrpayload.msgtype == 2)     //Application Payload msgs
       {
          printf("\n App payload msg detected!");
          
          if ( strcmp(recv_odrpayload.destip, my_eth0_ip_addr_p) != 0  )   //The msg is meant for another node
            {
              printf("\n App payload msg is meant for a different node");
              routing_tbl_lookup3=check_routing_tbl(recv_odrpayload.destip,time(NULL));
              
              ind_lookup3=routing_tbl_lookup3.out_array_ind;  //array index of destination node
              
              recv_odrpayload.hopcount=recv_odrpayload.hopcount+1;
    
              build_eth_frame(my_intfs_list[ind_lookup3].my_if_haddr,   routing_tbl_lookup3.nexthop_haddr,    
                              my_intfs_list[ind_lookup3].my_if_index,  my_intfs_list[ind_lookup3].psockfd , recv_odrpayload);
            }
            else
            {
            printf("\n The App payload msg is meant for me!\n Destport=%d",recv_odrpayload.destport);
            //look at the destination port number to determine if the msg is to be routed to client or server
            
              if (recv_odrpayload.destport == 1000)  //server
                {
                 bzero(&srvaddr, sizeof(srvaddr));	
                  srvaddr.sun_family = AF_LOCAL;
                 strcpy(srvaddr.sun_path, SRV_PATH);
                  
                  srvlen=sizeof(srvaddr);   
                  
                  memcpy(servbuff,recv_odrpayload.senderip,15);
                  memcpy(servbuff+15,recv_odrpayload.destip,15);
                  strcpy(servbuff+30,recv_odrpayload.req_service);
                  
                  //Send msg to server from ODR
                  Sendto(sockfd, servbuff, 100, 0, (SA *) &srvaddr, srvlen); 
                  printf("\n App payload msg sent to server");
                }
              else if (recv_odrpayload.destport == 2000)  //client 
                 { 
                    memcpy(clibuff,recv_odrpayload.senderip,15);
                    memcpy(clibuff+15,recv_odrpayload.destip,15);
                    strcpy(clibuff+30,recv_odrpayload.msg_body);
                   
                   Sendto(sockfd, clibuff, 100, 0, (SA *) &cliaddr, clilen); 
                   printf("\n Response from server sent to client");
                 }  
            
             
                        
            }
            printf("\n End of App payload handling");
       }    //End of App Payload handling
          
          
          
          
      
      
   }   //close ifFDSSET
   }   //close for
 }     //close else 

} // infinite loop ends here




	free_hwa_info(hwahead);
 //return(void);
 printf("\n End of prog");
 exit(0);
}
 //end of main function
 
///////////////////////////////////////////////////////////////
 
void build_eth_frame(char *eth_src_mac, char *eth_dest_mac,  int eth_if_index,  int eth_psockfd, 
                     struct odrpayload_info odrpayload)
{
int eth_i,eth_j;
char *eth_ptr;
struct in_addr sinaddr;

//printf("\n Entered build_eth_frame() with if_index = %d , eth_psockfd=%d",eth_if_index,eth_psockfd);
//printf("\n eth_src_mac= ");
 eth_ptr = eth_src_mac;
   		eth_i = IF_HADDR;
    			do {
    				printf("%.2x%s", *eth_ptr++ & 0xff, (eth_i == 1) ? " " : ":");
    			} while (--eth_i > 0);


//printf("\n eth_dest_mac= ");
 eth_ptr = eth_dest_mac;
   		eth_i = IF_HADDR;
    			do {
    				printf("%.2x%s", *eth_ptr++ & 0xff, (eth_i == 1) ? " " : ":");
    			} while (--eth_i > 0);


/*target address*/
struct sockaddr_ll socket_address;

/*buffer for ethernet frame*/
void* buffer = (void*)malloc(MY_ETH_FRAME_LEN);
 
/*pointer to ethenet header*/
unsigned char* etherhead = buffer;
	
/*userdata in ethernet frame*/
unsigned char* data = buffer + 14;
	
/*another pointer to ethernet header*/
struct ethhdr *eh = (struct ethhdr *)etherhead;
 
int send_result = 0;

/*our MAC address*/
//unsigned char src_mac[6] = {0x00, 0x01, 0x02, 0xFA, 0x70, 0xAA};

/*other host MAC address*/
//unsigned char dest_mac[6] = {0x00, 0x04, 0x75, 0xC8, 0x28, 0xE5};

unsigned char src_mac[6],dest_mac[6];
memcpy(src_mac,eth_src_mac,6);
memcpy(dest_mac,eth_dest_mac,6);




/*prepare sockaddr_ll*/

/*RAW communication*/
socket_address.sll_family   = PF_PACKET;	
/*we don't use a protocoll above ethernet layer
  ->just use anything here*/
//socket_address.sll_protocol = htons(ETH_P_IP);	
socket_address.sll_protocol = htons(USID_PROTO);

//socket_address.sll_protocol = USID_PROTO;

//index of the network device
socket_address.sll_ifindex  = eth_if_index;

/*ARP hardware identifier is ethernet*/
socket_address.sll_hatype   = ARPHRD_ETHER;
	
/*target is another host*/
//socket_address.sll_pkttype  = MY_PACKET_OTHERHOST;
socket_address.sll_pkttype  = PACKET_BROADCAST; 
//printf("\n socket_address.sll_pkttype = %c , %d",
//           socket_address.sll_pkttype,socket_address.sll_pkttype);

/*address length*/
socket_address.sll_halen    = 6 ;		
//printf("\n socket_address.sll_halen = %c , %d",
//        socket_address.sll_halen,socket_address.sll_halen);
        
//Set the 2 fields below to 0 while sending
//socket_address.sll_hatype = 0;
//socket_address.sll_pkttype = 0;


/*MAC - begin*/
//socket_address.sll_addr[0]  = 0x00;		
//socket_address.sll_addr[1]  = 0x04;		
//socket_address.sll_addr[2]  = 0x75;
//socket_address.sll_addr[3]  = 0xC8;
//socket_address.sll_addr[4]  = 0x28;
//socket_address.sll_addr[5]  = 0xE5;
/*MAC - end*/
memcpy(socket_address.sll_addr,eth_dest_mac,6);
socket_address.sll_addr[6]  = 0x00;/*not used*/
socket_address.sll_addr[7]  = 0x00;/*not used*/


/*set the frame header*/
memcpy((void*)buffer, (void*)dest_mac, 6);
memcpy((void*)(buffer+6), (void*)src_mac, 6);
eh->h_proto = htons(USID_PROTO);
//eh->h_proto = USID_PROTO;
/*fill the frame with some data*/
/*fill the frame with some data*/
//for (eth_j = 0; eth_j < 1500; eth_j++) {
//	data[eth_j] = (unsigned char)((int) (255.0*rand()/(RAND_MAX+1.0)));
// }
//strcpy(data,"Test RREQ from PB");

//strcpy(data,"Test message");

//printf("\n Send buffer = %s",buffer);



/*
printf("\n In buffer eth_dest_mac= ");
 eth_ptr = (char *) buffer;
   		eth_i = IF_HADDR;
    			do {
    				printf("%.2x%s", *eth_ptr++ & 0xff, (eth_i == 1) ? " " : ":");
    			} while (--eth_i > 0);



printf("\n In buffer eth_src_mac= ");
 eth_ptr = (char *) (buffer+6) ;
   		eth_i = IF_HADDR;
    			do {
    				printf("%.2x%s", *eth_ptr++ & 0xff, (eth_i == 1) ? " " : ":");
    			} while (--eth_i > 0);

printf("\n Packet type %hhx %hhx ",*( (char *)(buffer+12)) ,* ((char *)(buffer+13)) );
printf("\n Packet type %d ",*( (short int *)(buffer+12)) );
*/
//printf("\n Payload = %s", (char *)(buffer+14) );
socklen_t sockaddrlen = sizeof(struct sockaddr_ll);
char test_str[2];
//printf("\n ----Contents of sockaddr_ll structure---");
//printf("\n sockaddr_ll.sll_family = %d",socket_address.sll_family);
memcpy(test_str,&(socket_address.sll_protocol),2);
//printf("\n test_str = %hhx %hhx",test_str[0],test_str[1]);
//printf("\n sockaddr_ll.sll_protocol = %x",socket_address.sll_protocol);
//printf("\n sockaddr_ll.sll_ifindex = %d",socket_address.sll_ifindex);
//printf("\n sockaddr_ll.sll_hatype = %d",socket_address.sll_hatype);
//printf("\n sockaddr_ll.sll_pkttype = %x",socket_address.sll_pkttype);
//printf("\n sockaddr_ll.sll_halen = %d",socket_address.sll_halen);
//printf("\n sockaddr_ll.sll_addr = %hhx , %hhx ,%hhx , %hhx ,%hhx ,%hhx ,%hhx ,%hhx ",
//       socket_address.sll_addr[0],socket_address.sll_addr[1],
//      socket_address.sll_addr[2],socket_address.sll_addr[3],socket_address.sll_addr[4],
//       socket_address.sll_addr[5],socket_address.sll_addr[6],socket_address.sll_addr[7]);

strcpy(odrpayload.sendervm,my_host_name);



memcpy(data,&odrpayload,sizeof(struct odrpayload_info));


inet_pton(AF_INET, odrpayload.senderip, &sinaddr);
hptr = gethostbyaddr(&sinaddr, sizeof(sinaddr), AF_INET);
strcpy(src_host_name,hptr->h_name);

inet_pton(AF_INET, odrpayload.destip, &sinaddr);
hptr = gethostbyaddr(&sinaddr, sizeof(sinaddr), AF_INET);
strcpy(dest_host_name,hptr->h_name);

        


printf("\n-----------------------------------------------------------------------------------");
printf("\n ODR at node %s : Sending frame hdr src %s dest %hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           my_host_name,my_host_name,  *( (char *)(buffer)) ,* ((char *)(buffer+1)) , 
           *( (char *)(buffer+2)) ,* ((char *)(buffer+3)) , *( (char *)(buffer+4)) ,* ((char *)(buffer+5)) );
printf("\n                  ODR msg type %d src %s dest %s",odrpayload.msgtype,src_host_name,dest_host_name);
printf("\n-----------------------------------------------------------------------------------");




/*send the packet*/
send_result = sendto(eth_psockfd, buffer, MY_ETH_FRAME_LEN, 0,(struct sockaddr*)&socket_address, sockaddrlen);
printf("\n Send result=%d",send_result);

int eth_gso=getsockopt(eth_psockfd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen);
//printf("\n Error number %d, getsockopt return code=%d",error,eth_gso);

}

//update_routing_tbl(recv_odrpayload.senderip,(char *)recvbuff+6,ind,recv_odrpayload.hopcount+1,time(null));
int update_routing_tbl(char * tbl_ip,char * tbl_mac,int tbl_array_ind,int tbl_hopcount,int tbl_broadcast_id,time_t tbl_timestamp)
{

int rownum,better_rt=0;
//check if an entry on the table already exists
//loop thru the array routing_tbl looking for a match on tbl_ip
printf("\n entered update_routing_tbl() function");
printf("\n tbl_ip = %s",tbl_ip);

for(rownum=0;rownum<20;rownum++)
{
 if ( strcmp(routing_tbl[rownum].nodeip,tbl_ip) ==0 )   //node matches
   {
      printf("\n Row already exists! Checking if update is required");
      
      if( tbl_broadcast_id == routing_tbl[rownum].broadcast_id)  //bcast ID matches - so RREQ is duplicate
         better_rt = 2;
         
      if( (tbl_timestamp - routing_tbl[rownum].timestamp > 30 ) || (tbl_hopcount < routing_tbl[rownum].hopcount ) )
        {    
          //expired or new hopcount is less than current
          better_rt = 1; 
          printf("\n Updating existing row in the routing table"); 
          memcpy(routing_tbl[rownum].nexthop_haddr,tbl_mac,6); 
          routing_tbl[rownum].out_array_ind=tbl_array_ind;
          routing_tbl[rownum].hopcount = tbl_hopcount;
          routing_tbl[rownum].timestamp=tbl_timestamp;
          routing_tbl[rownum].broadcast_id  = tbl_broadcast_id;
          break;
        } 
       else
       printf("\n Update to existing row not required");
   }
 else if ( strlen(routing_tbl[rownum].nodeip) == 0)  //row is empty
   {
   printf("\n Inserting a new row in the routing table"); 
   strcpy(routing_tbl[rownum].nodeip,tbl_ip);
   memcpy(routing_tbl[rownum].nexthop_haddr,tbl_mac,6); 
   routing_tbl[rownum].out_array_ind=tbl_array_ind;
   routing_tbl[rownum].hopcount = tbl_hopcount;
   routing_tbl[rownum].timestamp=tbl_timestamp;
   routing_tbl[rownum].broadcast_id  = tbl_broadcast_id;
   better_rt=1;
   break;
   }
}

return(better_rt);
//print_routing_tbl();
}


void print_routing_tbl(void)
{
int printind;
printf("\n Current contents of routing table:");
printf("\n IP addr | MAC addr | Array Index |Hop Count |Timestamp");

for(printind=0;printind<20;printind++)
{
if ( strlen(routing_tbl[printind].nodeip) ==0 )
   break;
else
  {
  
   printf("\n %s  | %hhx %hhx %hhx %hhx %hhx %hhx | %d | %d | %d  |%s",
   routing_tbl[printind].nodeip,
   *(routing_tbl[printind].nexthop_haddr),*(routing_tbl[printind].nexthop_haddr +1),
   *(routing_tbl[printind].nexthop_haddr+2),*(routing_tbl[printind].nexthop_haddr+3),
   *(routing_tbl[printind].nexthop_haddr+4),*(routing_tbl[printind].nexthop_haddr+5),
   routing_tbl[printind].out_array_ind,
   routing_tbl[printind].hopcount,routing_tbl[printind].broadcast_id,
   ctime(&(routing_tbl[printind].timestamp)));
  }

}
      
}

////////
struct routing_tbl_info check_routing_tbl(char * ctbl_ip,time_t ctbl_timestamp)
{

int crownum;
struct routing_tbl_info routing_tbl_lookup;
memset(&routing_tbl_lookup,0,sizeof(struct routing_tbl_info));
//check if an entry on the table already exists
//loop thru the array routing_tbl looking for a match on tbl_ip
printf("\n entered check_dest_tbl() function");
printf("\n tbl_ip = %s",ctbl_ip);

for(crownum=0;crownum<20;crownum++)
{
 if ( strcmp(routing_tbl[crownum].nodeip,ctbl_ip) ==0 )
   {
     if ( ctbl_timestamp - routing_tbl[crownum].timestamp < 30 )
     {
     printf("\n Check_dest_tbl function found an unexpired match- so returning 1");
     strcpy(routing_tbl_lookup.nodeip,routing_tbl[crownum].nodeip);
     memcpy(routing_tbl_lookup.nexthop_haddr,routing_tbl[crownum].nexthop_haddr,6);
     routing_tbl_lookup.out_array_ind  = routing_tbl[crownum].out_array_ind;
     routing_tbl_lookup.hopcount       =routing_tbl[crownum].hopcount;
     routing_tbl_lookup.broadcast_id  =routing_tbl[crownum].broadcast_id;
     routing_tbl_lookup.timestamp  =routing_tbl[crownum].timestamp;
     }
     break;
    } 
 }
 return (routing_tbl_lookup);    
     
}  


void propagate_rreq(void * precvbuff,int pincoming_ind,int prrep_sent)
{
struct odrpayload_info precv_odrpayload;
int pind;

memcpy(&precv_odrpayload,(char *)precvbuff+14,sizeof(struct odrpayload_info));
//increment hop count
precv_odrpayload.hopcount=precv_odrpayload.hopcount+1;
precv_odrpayload.rrep_sent = prrep_sent;
//memcpy((char *)precvbuff+14,&precv_odrpayload,sizeof(struct odrpayload_info));
/*
struct sockaddr_ll psocket_address;
psocket_address.sll_family   = PF_PACKET;
psocket_address.sll_protocol = htons(USID_PROTO);
psocket_address.sll_hatype   = ARPHRD_ETHER;
psocket_address.sll_pkttype  = PACKET_BROADCAST; 
psocket_address.sll_halen    = 6 ;	
memcpy(psocket_address.sll_addr,all_dest_mac,6);
psocket_address.sll_addr[6]  = 0x00;
psocket_address.sll_addr[7]  = 0x00;
socklen_t psockaddrlen = sizeof(struct sockaddr_ll);
*/


//loop thru all interfaces
for (pind=0;pind<=max_ind;pind++)
{
  if(pind != pincoming_ind)  //send RREQ on all interfaces except the one it came in on
  {
    build_eth_frame(my_intfs_list[pind].my_if_haddr,   all_dest_mac,    
                        my_intfs_list[pind].my_if_index,  my_intfs_list[pind].psockfd , precv_odrpayload);
    printf("\n Forwarded RREQ on array index %d",pind);
  }
 }
 

 } 
  
      
      
void generate_rrep(void * grecvbuff,int gincoming_ind,int ghopcount)
{
struct odrpayload_info grecv_odrpayload;

memcpy(&grecv_odrpayload,(char *)(grecvbuff+14),sizeof(struct odrpayload_info));

grecv_odrpayload.hopcount=ghopcount;   //ghopcount looked up from routing tbl
grecv_odrpayload.msgtype =1; //RREP
strcpy(grecv_odrpayload.sendervm,my_host_name);

build_eth_frame(my_intfs_list[gincoming_ind].my_if_haddr,   (char *) (grecvbuff+6),    
                my_intfs_list[gincoming_ind].my_if_index,  my_intfs_list[gincoming_ind].psockfd , grecv_odrpayload);


 } 
      
      
      
