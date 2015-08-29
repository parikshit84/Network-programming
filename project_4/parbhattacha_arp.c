#include	"unp.h"
#include	"hw_addrs.h"

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#define IPPROTO_PB 83    //unique protocol ID for my implementation
#define IPID_PB    38    //unique Identification field for my implementation
#define USID_PROTO 0xAA83

#define ARP_LISPATH "/tmp/arpl_pb.dg" 
#define MY_ETH_FRAME_LEN 1000

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>           // close()
#include <string.h>           // strcpy, memset(), and memcpy()

#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t, uint32_t
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_RAW, IPPROTO_IP, IPPROTO_TCP, INET_ADDRSTRLEN
#include <netinet/ip.h>       // struct ip and PKT_LEN (which is 65535)
#define __FAVOR_BSD           // Use BSD format of tcp header
#include <netinet/tcp.h>      // struct tcphdr
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl.
//#include <net/if.h>           // struct ifreq

#include <errno.h>            // errno, perror()

// Define some constants.
#define IP4_HDRLEN 20         // IPv4 header length
#define TCP_HDRLEN 20         // TCP header length, excludes options data
#define PKT_LEN    1000


struct hwaddr {
		     int             sll_ifindex;	 /* Interface number */
		     unsigned short  sll_hatype;	 /* Hardware type */
		     unsigned char   sll_halen;		 /* Length of address */
		     unsigned char   sll_addr[8];	 /* Physical layer address */
};



// Function prototypes

void lookup_ip(void);
void build_eth_frame(char *,char *,int,int,char *);

int error;
socklen_t errlen=sizeof(error);

struct  sockaddr  *my_eth0_ip_addr_n;	/* eth0 IP address in network byte format */
char my_eth0_ip_addr_p[15];  // eth0 IP address in presentation format
char my_eth0_haddr[IF_HADDR];   //eth0 HW address in network byte format 
char my_host_name[50];
int my_eth0_if_index;
struct sockaddr_in *ipv4, sain;
struct ip iphdr;
int *ip_flags;
uint8_t *packet;
char *payload;
int payloadlen;
char all_dest_mac[6] ={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
char my_frame_type[2]={0xAA,0x83};
char my_arp_id[2]    ={0xAB,0x38};
uint16_t arp_hard_type = 1 ;     // 1=ethernet
uint16_t arp_prot_type = 0x800 ; // 0x800= IP address
uint8_t  arp_hard_size =6;  //size of HW addr
uint8_t  arp_prot_size =4;  //size of IP addr v4
uint16_t arp_req_type = 1 ;  //1=request,  2=reply
uint16_t arp_rep_type = 2 ;  //1=request,  2=reply
 
 
int
main (int argc, char **argv)
{
lookup_ip();
int listenfd, connfd, pfd,n,maxfd;
socklen_t clilen;
struct sockaddr_un cliaddr, servaddr;
char readbuf[MAXLINE],str[100],areqbuf[50];
fd_set rset;
struct sockaddr_in *asin;


//Create Unix domain SOCK_STREAM socket for communication with tour module on this node.
listenfd = Socket(AF_LOCAL, SOCK_STREAM, 0);
unlink(ARP_LISPATH);
bzero(&servaddr, sizeof(servaddr));
servaddr.sun_family = AF_LOCAL;
strcpy(servaddr.sun_path, ARP_LISPATH);
Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));


Listen(listenfd, LISTENQ);
printf("\n Unix domain SOCK_STREAM Socket in Listen state.Waiting for incoming connections...");
printf("\n");

    clilen = sizeof(cliaddr);
    if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) > 0) 
    {
       printf("\n Connection successful!connfd=%d",connfd);
       n = read(connfd, readbuf, MAXLINE);
       
       if ( strcmp(readbuf,"hello") ==0 )
       printf("\n Hello received");
    }   



//Create PF_PACKET socket for communication with arp modules running on other nodes
pfd = socket (PF_PACKET, SOCK_RAW,  htons(USID_PROTO)  );
   printf("Created PF_PACKET socket with return code %d",pfd);   
   
for(;;)  //inf loop begins here
{ 
FD_ZERO(&rset);
maxfd=0;

//FD_SET(listenfd, &rset);
//maxfd=listenfd; 
//printf("\n added listening socket with fd %d to Select readset",listenfd);

FD_SET(connfd,&rset);
   maxfd=connfd;
printf("\n added Unix domain socket with fd %d to Select readset",connfd);


FD_SET(pfd, &rset);
if (pfd>maxfd)
   maxfd=pfd;
printf("\n added PF_PACKET socket with fd %d to Select readset",pfd);


printf("\nEntering select to monitor sockets for readability...");
printf("\n");
   Select(maxfd+1, &rset, NULL, NULL, NULL);  //blocks until there is an incoming client
printf("Exited main select");
   
   
//check if select was trigerred by connfd
if ( FD_ISSET(connfd, &rset)   )
{
    printf("\n FD_ISSET - Incoming mesage on Unix domain socket");
    clilen = sizeof(cliaddr);
//    if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) > 0) 
//    {
//       printf("\n Connection successful!connfd=%d",connfd);
       n = read(connfd, readbuf, MAXLINE);
       printf("\n %d bytes received",n);
       
       if (n==0)
       {
       printf("\n Connection closed");
       exit (EXIT_FAILURE); 
       }
       
       
       int gso=getsockopt(connfd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen);
       printf("\n Error number %d, getsockopt return code=%d",error,gso);
       
       if ( strcmp(readbuf,"hello") ==0 )
       printf("\n Hello received");
       
       if ( ( n  > 0 ) && (strcmp(readbuf,"hello") != 0)  )
       {
       printf("\n--------------------------------------------------------");
       printf("\n Received an areq request with the following details:");
       printf("\n Requestor's IP address = %s",  inet_ntop(AF_INET, readbuf, str, sizeof(str))  );
       printf("\n Requestor's eth0 HW addr = %hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           *(readbuf+10),*(readbuf+11),*(readbuf+12),*(readbuf+13),*(readbuf+14),*(readbuf+15) );
       printf("\n HW address requested for dest IP address = %s",  inet_ntop(AF_INET, readbuf+20, str, sizeof(str))  );
       printf("\n--------------------------------------------------------");
           
       
     
      //format ARP request
    //  memcpy(areqbuf,all_dest_mac,6);      //Bytes 0-5: Broadcast HW addr - all 1s
    //  memcpy(areqbuf+6,my_eth0_haddr,6);   //Bytes 6-11: My HW addr in network byte format  
    //  memcpy(areqbuf+12,my_frame_type,2);  //Bytes12-13: Unique Frame type (Protocol ID)
      //
      memcpy(areqbuf+14,my_arp_id,2);      //Bytes 14-15: Extra 2 byte identification field
      memcpy(areqbuf+16,&arp_hard_type,2);  //Bytes 16-17: Hardware type
      memcpy(areqbuf+18,&arp_prot_type,2);  //Bytes 18-19: protocol Type
      memcpy(areqbuf+20,&arp_hard_size,1);  //Byte 20: hardware size
      memcpy(areqbuf+21,&arp_prot_size,1);  //Byte 21: Protocol size
      memcpy(areqbuf+22,&arp_req_type,2);   //Bytes 22-23: ARP request type
      memcpy(areqbuf+24,readbuf+10,6);     //Bytes 24-29: sender's Ethernet addr 
      memcpy(areqbuf+30,readbuf,4);        //Bytes 30-33: sender's IP addr 
      //-------------------------------------Bytes 34-39: Target Ethernet addr - leave blank 
      memcpy(areqbuf+40,readbuf+20,4);     //Bytes 40 thru 43 : Target IP addr
      
    // void build_eth_frame(char *eth_src_mac, char *eth_dest_mac,  int eth_if_index,  int eth_psockfd, 
    //                     struct odrpayload_info odrpayload) 
      build_eth_frame(my_eth0_haddr, all_dest_mac,my_eth0_if_index, pfd , areqbuf+14);   
   }
    
}  //FD_ISSET ends
else if (  FD_ISSET(pfd, &rset)  )
{
    printf("\n Incoming message on PF_PACKET socket");
    void * pfrecvbuf = (void *)malloc(MY_ETH_FRAME_LEN);
      int recvbytes = recvfrom(pfd,pfrecvbuf,MY_ETH_FRAME_LEN,0,NULL,NULL);
      printf("\n recvbytes=%d",recvbytes);
      printf("\n Contents of received msg:");
      
      char * rcv_ptr;
      int rcv_i;
      short int pfrecvbuf_msgtype = *( (short int *)(pfrecvbuf+22)  );
      
      printf("\n ARP Message type = %d", pfrecvbuf_msgtype );
      printf("\n Recv buffer eth_dest_mac= ");
      rcv_ptr = (char *) pfrecvbuf;
   		rcv_i = IF_HADDR;
    			do {
    				printf("%.2x%s", *rcv_ptr++ & 0xff, (rcv_i == 1) ? " " : ":");
    			} while (--rcv_i > 0);



       printf("\n Recv buffer eth_src_mac= ");
       rcv_ptr = (char *) (pfrecvbuf+6) ;
       rcv_i = IF_HADDR;
    			do {
    				printf("%.2x%s", *rcv_ptr++ & 0xff, (rcv_i == 1) ? " " : ":");
    			} while (--rcv_i > 0);    
          
          
          printf("\n Recv buffer- ARP ID = %c%c",*( (char *)(pfrecvbuf+14)),*  ((char *)(pfrecvbuf+15)) );
          printf("\n Recv buffer- Sender eth id:");
          
          rcv_ptr = (char *) (pfrecvbuf+24);
   		    rcv_i = IF_HADDR;
    			do {
    				printf("%.2x%s", *rcv_ptr++ & 0xff, (rcv_i == 1) ? " " : ":");
    			} while (--rcv_i > 0);
          
          
          printf("\n Recv buffer- Sender's IP addr: %s", inet_ntop(AF_INET, pfrecvbuf+30, str, sizeof(str))   );
          
          printf("\n Recv buffer- Target eth id:");
          
          rcv_ptr = (char *) (pfrecvbuf+34);
   		    rcv_i = IF_HADDR;
    			do {
    				printf("%.2x%s", *rcv_ptr++ & 0xff, (rcv_i == 1) ? " " : ":");
    			} while (--rcv_i > 0);
          
                    
          printf("\n Recv buffer- Target IP addr : %s",  inet_ntop(AF_INET, pfrecvbuf+40, str, sizeof(str))  );
          
          //check if target IP is same as my IP
          if (  ( strcmp(str,my_eth0_ip_addr_p)==0  )  &&  (pfrecvbuf_msgtype == 1)  )
          {
          
          printf("\n AREQ is directed to me. Generating a response to the AREQ..");
          //format ARP reply packet     
           memcpy(areqbuf+14,my_arp_id,2);      //Bytes 14-15: Extra 2 byte identification field
           memcpy(areqbuf+16,&arp_hard_type,2);  //Bytes 16-17: Hardware type
           memcpy(areqbuf+18,&arp_prot_type,2);  //Bytes 18-19: protocol Type
           memcpy(areqbuf+20,&arp_hard_size,1);  //Byte 20: hardware size
           memcpy(areqbuf+21,&arp_prot_size,1);  //Byte 21: Protocol size
           memcpy(areqbuf+22,&arp_rep_type,2);   //Bytes 22-23: ARP reply 
         
           memcpy(areqbuf+24,my_eth0_haddr,6);     //Bytes 24-29: sender's Ethernet addr 
           
           asin = (struct sockaddr_in *) my_eth0_ip_addr_n;
           memcpy(areqbuf+30,&asin->sin_addr,4);   //Bytes 30-33: sender's IP addr 
           memcpy(areqbuf+34,pfrecvbuf+24,6);      //Bytes 34-39: Target Ethernet addr 
           memcpy(areqbuf+40,pfrecvbuf+30,4);      //Bytes 40 thru 43 : Target IP addr
           
          // void build_eth_frame(char *eth_src_mac, char *eth_dest_mac,  int eth_if_index,  int eth_psockfd, 
          //                     struct odrpayload_info odrpayload) 
            build_eth_frame(my_eth0_haddr, pfrecvbuf+24, my_eth0_if_index, pfd , areqbuf+14);  
            printf("\n--------------------------------------------------------");
            printf("\n Response to AREQ sent");
            printf("\n--------------------------------------------------------");
          }
          
          if (pfrecvbuf_msgtype == 2)
          {
          printf("\n--------------------------------------------------------");
          printf("\n Response to AREQ received");
          Writen(connfd, (pfrecvbuf+24), 6);
          printf("\n Sent the requested HW address to the tour module");
          printf("\n--------------------------------------------------------");
          }
           
          
          
          
          
}

} //inf loop ends here

 
  return (EXIT_SUCCESS);
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
        my_eth0_if_index = hwa->if_index;
        memcpy(my_eth0_haddr,hwa->if_haddr,IF_HADDR);
        break;     
      }
   }   
        
strcpy(my_eth0_ip_addr_p,Sock_ntop( my_eth0_ip_addr_n, sizeof(*my_eth0_ip_addr_n)));
printf("\n My eth0 IP address=%s",my_eth0_ip_addr_p);


inet_pton(AF_INET, my_eth0_ip_addr_p, &sinaddr);
hptr = gethostbyaddr(&sinaddr, sizeof(sinaddr), AF_INET);
strcpy(my_host_name,hptr->h_name);
printf("My Host name: %s\n", my_host_name);

printf("\n eth0 interface index = %d\n", my_eth0_if_index);
        
} 


void build_eth_frame(char *eth_src_mac, char *eth_dest_mac,  int eth_if_index,  int eth_psockfd, 
                     char *arppayload)
{
int eth_i,eth_j;
char *eth_ptr;
struct in_addr sinaddr;

printf("\n Entered build_eth_frame() with if_index = %d , eth_psockfd=%d",eth_if_index,eth_psockfd);
printf("\n eth_src_mac= ");
 eth_ptr = eth_src_mac;
   		eth_i = IF_HADDR;
    			do {
    				printf("%.2x%s", *eth_ptr++ & 0xff, (eth_i == 1) ? " " : ":");
    			} while (--eth_i > 0);


printf("\n eth_dest_mac= ");
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

unsigned char src_mac[6],dest_mac[6];
memcpy(src_mac,eth_src_mac,6);
memcpy(dest_mac,eth_dest_mac,6);




/*prepare sockaddr_ll*/

/*RAW communication*/
socket_address.sll_family   = PF_PACKET;	
socket_address.sll_protocol = htons(USID_PROTO);
//index of the network device
socket_address.sll_ifindex  = eth_if_index;

/*ARP hardware identifier is ethernet*/
socket_address.sll_hatype   = ARPHRD_ETHER;
	
/*target is another host*/
//socket_address.sll_pkttype  = MY_PACKET_OTHERHOST;
socket_address.sll_pkttype  = PACKET_BROADCAST; 

/*address length*/
socket_address.sll_halen    = 6 ;		

memcpy(socket_address.sll_addr,eth_dest_mac,6);
socket_address.sll_addr[6]  = 0x00;/*not used*/
socket_address.sll_addr[7]  = 0x00;/*not used*/


/*set the frame header*/
memcpy((void*)buffer, (void*)dest_mac, 6);
memcpy((void*)(buffer+6), (void*)src_mac, 6);
eh->h_proto = htons(USID_PROTO);

socklen_t sockaddrlen = sizeof(struct sockaddr_ll);
//char test_str[2];
//memcpy(test_str,&(socket_address.sll_protocol),2);

//strcpy(odrpayload.sendervm,my_host_name);

memcpy(data,arppayload,30);

/*
inet_pton(AF_INET, odrpayload.senderip, &sinaddr);
hptr = gethostbyaddr(&sinaddr, sizeof(sinaddr), AF_INET);
strcpy(src_host_name,hptr->h_name);

inet_pton(AF_INET, odrpayload.destip, &sinaddr);
hptr = gethostbyaddr(&sinaddr, sizeof(sinaddr), AF_INET);
strcpy(dest_host_name,hptr->h_name);

        


printf("\n-----------------------------------------------------------------------------------");
printf("\n ARP at node %s : Sending frame hdr src %s dest %hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           my_host_name,my_host_name,  *( (char *)(buffer)) ,* ((char *)(buffer+1)) , 
           *( (char *)(buffer+2)) ,* ((char *)(buffer+3)) , *( (char *)(buffer+4)) ,* ((char *)(buffer+5)) );
printf("\n                  ODR msg type %d src %s dest %s",odrpayload.msgtype,src_host_name,dest_host_name);
printf("\n-----------------------------------------------------------------------------------");

*/


/*send the packet*/
send_result = sendto(eth_psockfd, buffer, MY_ETH_FRAME_LEN, 0,(struct sockaddr*)&socket_address, sockaddrlen);
printf("\n Send result=%d",send_result);

int eth_gso=getsockopt(eth_psockfd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen);
printf("\n Error number %d, getsockopt return code=%d",error,eth_gso);

}