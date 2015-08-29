#include	"unp.h"
#include	"hw_addrs.h"
#include	<netinet/in_systm.h>
//#include	<netinet/ip.h>
#include	<netinet/ip_icmp.h>
//#include	"ping.h"
#include <sys/time.h>

//struct proto	proto_v4 = { proc_v4, send_v4, NULL, NULL, NULL, 0, IPPROTO_ICMP };

int	datalen = 56;		/* data that goes with ICMP echo request */

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#define MY_ETH_FRAME_LEN 1000


#define IPPROTO_PB 83    //unique protocol ID for my implementation
#define IPID_PB    38    //unique Identification field for my implementation
#define USID_PROTO 0xAA83

#define ARP_LISPATH "/tmp/arpl_pb.dg" 
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

int error;
socklen_t errlen=sizeof(error);

struct hwaddr {
		     int             sll_ifindex;	 /* Interface number */
		     unsigned short  sll_hatype;	 /* Hardware type */
		     unsigned char   sll_halen;		 /* Length of address */
		     unsigned char   sll_addr[8];	 /* Physical layer address */
};
int		 datalen;			/* # bytes of data following ICMP header */
char	*host;
int		 nsent;				/* add 1 for each sendto() */
pid_t	 pid;				/* our PID */
//int		 sockfd;
int		 verbose;

			/* function prototypes */
//void	 proc_v4(char *, ssize_t, struct msghdr *, struct timeval *);
void	 send_v4(void);

//void	 readloop(void);
void	 sig_alrm(int);
//void	 tv_sub(struct timeval *, struct timeval *);
void build_eth_frame(char *,char *,int,int,char *);

struct proto {
  void	 (*fproc)(char *, ssize_t, struct msghdr *, struct timeval *);
  void	 (*fsend)(void);
  void	 (*finit)(void);
  struct sockaddr  *sasend;	/* sockaddr{} for send, from getaddrinfo */
  struct sockaddr  *sarecv;	/* sockaddr{} for receiving */
  socklen_t	    salen;		/* length of sockaddr{}s */
  int	   	    icmpproto;	/* IPPROTO_xxx value for ICMP */
};

struct proto proto_v4 = { NULL, NULL, NULL, NULL, NULL, 0, IPPROTO_ICMP };
struct proto *pr;
char writebuf[30];






// Function prototypes
uint16_t checksum (uint16_t *, int);
//uint16_t tcp4_checksum (struct ip, struct tcphdr, uint8_t *, int);
char *allocate_strmem (int);
uint8_t *allocate_ustrmem (int);
int *allocate_intmem (int);
void lookup_ip(void);
void send_pkt(int,char *, char*);
int areq (struct sockaddr *, socklen_t , struct hwaddr *);

struct  sockaddr  *my_eth0_ip_addr_n;	/* eth0 IP address in network byte format */
char my_eth0_ip_addr_p[15];  // eth0 IP address in presentation format
char my_eth0_haddr[IF_HADDR];   //eth0 HW address in network byte format 
char my_host_name[50];
int my_eth0_if_index;
struct sockaddr_in *ipv4, sain;
struct ip iphdr,pngiphdr;
int *ip_flags, *pngip_flags;
uint8_t *packet , *pngpacket;
char *payload;
int payloadlen,icmplen;
char sample_mac[6] ={0x00,0x0c,0x29,0x49,0x3f,0x5b};
int pfd,arpfd;
//int error;
//socklen_t errlen=sizeof(error);

struct sockaddr_un servaddr;
char * rcv_ptr;
int rcv_i;
struct sockaddr_in  *pingipaddr;
char pinghwaddr[10];
 
int
main (int argc, char **argv)
{
  int i,lj, iprsd,  *tcp_flags;
  const int on = 1;
  char *interface, *src_ip, *dst_ip;
  
  struct tcphdr tcphdr;
  
 
  
  short int total_ctr,curr_ctr;
  struct addrinfo hints, *res;
  
  struct ifreq ifr;
  void *tmp;
  char str[INET_ADDRSTRLEN];
  struct ip in_iphdr;
  struct hostent *hptr;
  char timestmp[MAXLINE];
  time_t ticks;
  struct sockaddr_in *ipaddr;
  struct hwaddr      *hwaddr; 
  socklen_t sockaddrlen;
  int				c;
	struct addrinfo	*ai;
	char *h;
 int offset,maxfd;
 void * recvbuff = (void *)malloc(PKT_LEN);
 int recvbytes;
 char arpreadbuf[MAXLINE];
 fd_set rset;
 struct timeval tv;
struct timeval *timeout;
timeout=malloc(sizeof(struct timeval));

	opterr = 0;		/* don't want getopt() writing to stderr  */
 verbose=1;
  
  payloadlen = PKT_LEN-IP4_HDRLEN  ;

  // Allocate memory for various arrays.
  packet = allocate_ustrmem (PKT_LEN);
  pngpacket=allocate_ustrmem (PKT_LEN);
  interface = allocate_strmem (40);
  src_ip = allocate_strmem (INET_ADDRSTRLEN);
  dst_ip = allocate_strmem (INET_ADDRSTRLEN);
  ip_flags = allocate_intmem (4);
  pngip_flags= allocate_intmem (4);
  tcp_flags = allocate_intmem (8);
  payload = allocate_strmem (PKT_LEN -IP4_HDRLEN );
 
  ipaddr=malloc(sizeof(struct sockaddr_in));
  pingipaddr=malloc(sizeof(struct sockaddr_in));
  
  	char			*ptr, **pptr;
//	char			str[INET_ADDRSTRLEN];
//	struct hostent	*hptr;



  
//  printf("\n argc=%d",argc);
  //get current node's eth0 IP  
  lookup_ip();
 
  // Interface to send packet through.
  strcpy (interface, "eth0");
  memset (&ifr, 0, sizeof (ifr));
  snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface);
  ifr.ifr_ifindex = my_eth0_if_index ;
   
  //-----------------------------------------------------------------------------------

  
/////////////////////////////////////////////////////////////////////////////////////////  
   // Submit request for an IP raw socket descriptor.
 //  if ((sd = socket (AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
  if ((iprsd = socket (AF_INET, SOCK_RAW, IPPROTO_PB)) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }

  // Set flag so IP raw socket expects us to provide IPv4 header.
  if (setsockopt (iprsd, IPPROTO_IP, IP_HDRINCL, &on, sizeof (on)) < 0) {
    perror ("setsockopt() failed to set IP_HDRINCL ");
    exit (EXIT_FAILURE);
  }

  // Bind IP raw socket to interface index.
  if (setsockopt (iprsd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof (ifr)) < 0) {
    perror ("setsockopt() failed to bind to interface ");
    exit (EXIT_FAILURE);
  }
  
  //create PF_PACKET socket to send out ICMP echo requests 
  pfd = socket (PF_PACKET, SOCK_RAW,  htons(USID_PROTO)  );
   printf("Created PF_PACKET socket with return code %d",pfd); 
   

//create Unix domain SOCK_STREAM socket to communicate with the ARP process
arpfd = Socket(AF_LOCAL, SOCK_STREAM, 0);   //Unix domain Sock_stream socket
printf("\n Created unix domain SOCK_STREAM socket with socket # %d",arpfd);

bzero(&servaddr, sizeof(servaddr));
servaddr.sun_family = AF_LOCAL;
strcpy(servaddr.sun_path, ARP_LISPATH);
Connect(arpfd, (SA *) &servaddr, sizeof(servaddr));

int arp_gso=getsockopt(arpfd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen);
//printf("\n Error number %d, getsockopt return code=%d",error,arp_gso);
strcpy(writebuf,"hello");
Writen(arpfd, writebuf, 30);


if (argc > 1)   //If this is the source node, initiate the tour
{  
       total_ctr=argc;
       curr_ctr=1;
       memset(payload,0,payloadlen);
       memcpy(payload,  (char *)(&total_ctr),sizeof(short int));
       memcpy(payload+2,(char *)(&curr_ctr),sizeof(short int));
       strcpy(payload+4,my_eth0_ip_addr_p);     //starting node
       offset=20;
       
      // strcpy(payload+24,"130.245.156.22");  //node 2
      // strcpy(payload+44,"130.245.156.23");  //node 3

       	while (--argc > 0) 
        {
       		ptr = *++argv;
       		if ( (hptr = gethostbyname(ptr)) == NULL) {
       			err_msg("gethostbyname error for host: %s: %s",
       					ptr, hstrerror(h_errno));
       			continue;
       		}
       		printf("official hostname: %s\n", hptr->h_name);
       
       		for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
       			printf("\talias: %s\n", *pptr);
       
       		switch (hptr->h_addrtype) {
       		case AF_INET:
       			pptr = hptr->h_addr_list;
       			for ( ; *pptr != NULL; pptr++)
             {
       				printf("\taddress: %s\n",
       					Inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
               strcpy((payload+4+offset),str);
               offset=offset+20;                              
              }                               
       			break;
       
       		default:
       			err_ret("unknown address type");
       			break;
       		}
       	}
 
        // Source IPv4 address
       strcpy (src_ip, my_eth0_ip_addr_p);
       
       //Dest IP address
       //curr_ctr++;
       strcpy (dst_ip, payload+4+(20*curr_ctr) )  ;
       
 //      printf("\n src_ip = %s , dst_ip = %s ", src_ip,dst_ip);
       
       
        ////////////////////////////////////////////////////////////////////////////////////////////////////
     
     send_pkt(iprsd,src_ip,dst_ip);
}
////////////////////////////////////////
tv.tv_sec = 1;
tv.tv_usec = 0;
timeout = NULL; //set timeout pointer to NULL

for(;;) // infinite loop starts here
{

FD_ZERO(&rset);
maxfd=0;

FD_SET(iprsd, &rset);
maxfd=iprsd;
//printf("\n added IP raw socket (rt) with fd %d to Select readset",iprsd);

FD_SET(arpfd, &rset);
if (arpfd>maxfd)
   maxfd=arpfd;
//printf("\n added Unix domain socket (ARP) with fd %d to Select readset",arpfd);

printf("\nEntering select to monitor sockets for readability...");
printf("\n");
   Select(maxfd+1, &rset, NULL, NULL, timeout);  //blocks until there is an incoming client
printf("Exited main select");

if (  FD_ISSET(iprsd, &rset)  )    
{
      
      //n=Recvfrom(sd, mesg, MAXLINE, 0, (SA *) &cliaddr, &clilen);
      //printf("\n %d bytes received from client!Client addr is %s Msg is %s",n,cliaddr.sun_path,mesg);
      
      
      recvbytes = recvfrom(iprsd,recvbuff,PKT_LEN,0,NULL,NULL);
      printf("\n %d bytes received on IP raw socket (rt)",recvbytes);
      
//        printf("\n Received Packet contents are as follows:\n");
//        for(lj=0;lj<PKT_LEN;lj++)
//        printf("%c",*( (char *)(recvbuff+lj)  ) );
        
       //examine incoming packet
        memcpy (&in_iphdr, recvbuff, IP4_HDRLEN * sizeof (uint8_t)); 
//        printf("\n Identification field = %d", ntohs(in_iphdr.ip_id) );  //convert network byte order to host byte order
//        printf("\n Source IP address = %s",  inet_ntop(AF_INET, &in_iphdr.ip_src, str, sizeof(str))  );
//        printf("\n Dest IP address = %s",    inet_ntop(AF_INET, &in_iphdr.ip_dst, str, sizeof(str))  );
        
        if ( ntohs(in_iphdr.ip_id) == IPID_PB )
        {
           hptr = gethostbyaddr(&in_iphdr.ip_src, sizeof(in_iphdr.ip_src), AF_INET);
           ticks = time(NULL);
           snprintf(timestmp, sizeof(timestmp), "%.24s\r\n", ctime(&ticks));
//           printf("\n Timestamp = %s",timestmp);
           int tlen=strlen(timestmp);
 //          printf("\n strlen of timestmp= %d",tlen);
           
           timestmp[tlen-2]='\0';  //Overlay \r character with \0
           timestmp[tlen-1]='\0';  //Overlay \n character with \0
 //          printf("\n new strlen of timestmp= %d",strlen(timestmp));
           printf("\n--------------------------------------------------");
           printf("\n [%s]: received Source routing packet from %s",timestmp,hptr->h_name  );
           printf("\n--------------------------------------------------");
           
           memcpy (payload, recvbuff + IP4_HDRLEN, (payloadlen * sizeof (uint8_t)) ); 
           
          // Source IPv4 address
          strcpy (src_ip, my_eth0_ip_addr_p);
          
          //Dest IP address
          memcpy( (char *)(&total_ctr),  payload,     sizeof(short int));
          memcpy( (char *)(&curr_ctr),   payload + 2, sizeof(short int));
  //        
           printf("Total ctr = %d , Curr ctr = %d",total_ctr,curr_ctr);
  
          if ( curr_ctr < total_ctr )
          {
              curr_ctr++;
              memcpy(payload+2,(char *)(&curr_ctr),sizeof(short int));  //update curr_ctr in payload
              strcpy (dst_ip, payload+4+(20*curr_ctr) )  ;
             
             printf("\n src_ip = %s , dst_ip = %s ", src_ip,dst_ip);
             send_pkt(iprsd,src_ip,dst_ip);   
             
             bzero (ipaddr, sizeof (struct sockaddr_in) ) ;
             (*ipaddr).sin_family = AF_INET;
             memcpy (  &(ipaddr->sin_addr), &(in_iphdr.ip_src), sizeof (struct in_addr) ) ;
             printf ("\n Calling areq() to get hardware address for IP address %s\n",
                      Sock_ntop ( (SA *) ipaddr, sizeof(*ipaddr) ) ) ;
                      
                      
             memcpy(pingipaddr,ipaddr,sizeof(*ipaddr) );
             sockaddrlen = sizeof (struct sockaddr_in) ;
             areq (  (SA *) pingipaddr, sockaddrlen, hwaddr);
           
          
            
           }  //curr_ctr<to_ctr
           
        }  //IPID_PB
     
}  //FD_ISSET
else if (  FD_ISSET(arpfd, &rset)  )
{
       printf("\n Incoming message from ARP on Unix domain socket");
       recvbytes = read(arpfd, arpreadbuf, MAXLINE);
       if (recvbytes ==0)
       {
       printf("\n Connection closed");
       exit (EXIT_FAILURE);
       }
       printf("\n-----------------------------");
       printf("\n Response to AREQ received ; %d bytes received",recvbytes);
       
       
       printf("\n HW address of %s = ",Sock_ntop ( (SA *) pingipaddr, sizeof(*pingipaddr) ));
       
      rcv_ptr = (char *) arpreadbuf;
   		rcv_i = IF_HADDR;
    			do {
    				printf("%.2x%s", *rcv_ptr++ & 0xff, (rcv_i == 1) ? " " : ":");
    			} while (--rcv_i > 0);
       printf("\n-----------------------------");
          
       memcpy(pinghwaddr,arpreadbuf,6);   
          
          
       //Set ping_active to true);
       //Set timeout of 1 sec for the select to enable pings to be sent out every 1 second
       timeout = &tv;
       
       printf("\n Start Ping process..");
           //Ping code starts here 
           host = malloc(30);
           	strcpy(host, Sock_ntop ( (SA *)pingipaddr, sizeof(*pingipaddr)  ) );
            printf("\n  host = %s",host); 
           
            pid = getpid() & 0xffff;	/* ICMP ID field is 16 bits */
           //	Signal(SIGALRM, sig_alrm);
           
           	ai = Host_serv(host, NULL, 0, 0);
           
           	h = Sock_ntop_host(ai->ai_addr, ai->ai_addrlen);
   //        	printf("PING %s (%s): %d data bytes\n",
   //        			ai->ai_canonname ? ai->ai_canonname : h,
   //        			h, datalen);
           
           		/* 4initialize according to protocol */
           	if (ai->ai_family == AF_INET) 
           {
           		pr = &proto_v4;
           } else
           		err_quit("unknown address family %d", ai->ai_family);
           
           	pr->sasend = ai->ai_addr;
           	pr->sarecv = Calloc(1, ai->ai_addrlen);
           	pr->salen = ai->ai_addrlen;
           
      //     	readloop();
       
}
else //timeout value reached
{
// printf("\n Select timeout value reached. Sending pings...");
 tv.tv_sec=1;
 tv.tv_usec=0;
 	printf("\nPING %s (%s): %d data bytes\n",
           			ai->ai_canonname ? ai->ai_canonname : h,
           			h, datalen);
 //printf("\n Timeout value = %d",tv.tv_sec);
 send_v4();         
}
      

} // inf loop ends here

  // Close socket descriptor.
  close (iprsd);

  // Free allocated memory.
  free (packet);
  free (interface);
  free (src_ip);
  free (dst_ip);
  free (ip_flags);
  free (tcp_flags);
  free (payload);
 

  return (EXIT_SUCCESS);
}

// Checksum function
uint16_t
checksum (uint16_t *addr, int len)
{
  int nleft = len;
  int sum = 0;
  uint16_t *w = addr;
  uint16_t answer = 0;

  while (nleft > 1) {
    sum += *w++;
    nleft -= sizeof (uint16_t);
  }

  if (nleft == 1) {
    *(uint8_t *) (&answer) = *(uint8_t *) w;
    sum += answer;
  }

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  answer = ~sum;
  return (answer);
}



// Allocate memory for an array of chars.
char *
allocate_strmem (int len)
{
  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_strmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (char *) malloc (len * sizeof (char));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (char));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_strmem().\n");
    exit (EXIT_FAILURE);
  }
}

// Allocate memory for an array of unsigned chars.
uint8_t *
allocate_ustrmem (int len)
{
  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_ustrmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (uint8_t *) malloc (len * sizeof (uint8_t));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (uint8_t));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_ustrmem().\n");
    exit (EXIT_FAILURE);
  }
}

// Allocate memory for an array of ints.
int *
allocate_intmem (int len)
{
  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_intmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (int *) malloc (len * sizeof (int));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (int));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_intmem().\n");
    exit (EXIT_FAILURE);
  }
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
printf("   My Host name: %s\n", my_host_name);

printf("\n eth0 interface index = %d\n", my_eth0_if_index);
        
} 

void send_pkt(int fiprsd,char *fsrc_ip,char *fdst_ip)
{
int li,status;
  // format IPv4 header

  // IPv4 header length (4 bits): Number of 32-bit words in header = 5
  iphdr.ip_hl = IP4_HDRLEN / sizeof (uint32_t);

  // Internet Protocol version (4 bits): IPv4
  iphdr.ip_v = 4;

  // Type of service (8 bits)
  iphdr.ip_tos = 0;

  // Total length of datagram (16 bits): IP header + TCP header + TCP data
  iphdr.ip_len = htons (IP4_HDRLEN + payloadlen);

  // Identification field (16 bits): 
  iphdr.ip_id = htons (IPID_PB);

  // Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram

  // Zero (1 bit)
  ip_flags[0] = 0;

  // Do not fragment flag (1 bit)
  ip_flags[1] = 1;

  // More fragments following flag (1 bit)
  ip_flags[2] = 0;

  // Fragmentation offset (13 bits)
  ip_flags[3] = 0;

  iphdr.ip_off = htons ((ip_flags[0] << 15)
                      + (ip_flags[1] << 14)
                      + (ip_flags[2] << 13)
                      +  ip_flags[3]);

  // Time-to-Live (8 bits): default to maximum value
  iphdr.ip_ttl = 255;

  // Transport layer protocol (8 bits): 6 for TCP
  iphdr.ip_p = IPPROTO_PB;

  // Source IPv4 address (32 bits)
  if ((status = inet_pton (AF_INET, fsrc_ip, &(iphdr.ip_src))) != 1) {
    fprintf (stderr, "inet_pton() failed.\nError message: %s", strerror (status));
    exit (EXIT_FAILURE);
  }

  // Destination IPv4 address (32 bits)
  if ((status = inet_pton (AF_INET, fdst_ip, &(iphdr.ip_dst))) != 1) {
    fprintf (stderr, "inet_pton() failed.\nError message: %s", strerror (status));
    exit (EXIT_FAILURE);
  }

  // IPv4 header checksum (16 bits): set to 0 when calculating checksum
  iphdr.ip_sum = 0;
  iphdr.ip_sum = checksum ((uint16_t *) &iphdr, IP4_HDRLEN);

  

  // Prepare packet.

  // First part is an IPv4 header.
  memcpy (packet, &iphdr, IP4_HDRLEN * sizeof (uint8_t));

  // Next part of packet is upper layer protocol header.
  //memcpy ((packet + IP4_HDRLEN), &tcphdr, TCP_HDRLEN * sizeof (uint8_t));

  // Last part is upper layer protocol data.
  memcpy ((packet + IP4_HDRLEN ), payload, payloadlen * sizeof (uint8_t));

  // The kernel is going to prepare layer 2 information (ethernet frame header) for us.
  // For that, we need to specify a destination for the kernel in order for it
  // to decide where to send the raw datagram. We fill in a struct in_addr with
  // the desired destination IP address, and pass this structure to the sendto() function.
  memset (&sain, 0, sizeof (struct sockaddr_in));
  sain.sin_family = AF_INET;
  sain.sin_addr.s_addr = iphdr.ip_dst.s_addr;

 
  
  //Print contents of packet before sending
//  printf("\n Packet contents are as follows:\n");
//  for(li=0;li<PKT_LEN;li++)
//    printf("%c",*( (char *)(packet+li)  ) );

// printf("\n Payload contents are as follows:\n");
// printf("\n %d %d", *(  (short int *)(payload)  ),  *(  (short int *)(payload+2)  )  );

 //for(li=0;li<payloadlen;li++)
 //   printf("%c",*( (char *)(payload+4+li)  ) );
  
  

  // Send packet.
  if (sendto (fiprsd, packet, IP4_HDRLEN + payloadlen, 0, (struct sockaddr *) &sain, sizeof (struct sockaddr)) < 0)  {
    perror ("sendto() failed ");
    exit (EXIT_FAILURE);
  }
  printf("\n--------------------------------------------------");
  printf("\n Sent source routing packet to %s",fdst_ip);
  printf("\n--------------------------------------------------");
  }

int areq (struct sockaddr *aipaddr, socklen_t asockaddrlen, struct hwaddr *ahwaddr)
{

int lk;



char str[100];
struct sockaddr_in *asin;



bzero(writebuf, sizeof(writebuf));

asin = (struct sockaddr_in *) my_eth0_ip_addr_n;
printf("\n My IP address = %s",    inet_ntop(AF_INET, &asin->sin_addr, str, sizeof(str))  );
memcpy(writebuf,&asin->sin_addr,4);   //Bytes 0-3:  My IP addr in network byte format

memcpy(writebuf+10,my_eth0_haddr,6);             //Bytes 10-15: My HW addr in network byte format         

asin = (struct sockaddr_in *) aipaddr;
printf("\n Dest IP address = %s",   inet_ntop(AF_INET, &asin->sin_addr, str, sizeof(str))  );
memcpy(writebuf+20,&asin->sin_addr,4);          //Bytes 20-23: Dest IP addr in network byte format


//printf ("\n Calling ARP service to get hardware address for IP address %s\n",
//                      Sock_ntop ( aipaddr, sizeof(*aipaddr) ) ) ;

Writen(arpfd, writebuf, 30);
printf("\n Sent request to ARP service with the following fields:");
printf("\n My IP address = %s",  inet_ntop(AF_INET, writebuf, str, sizeof(str))  );
printf("\n My eth0 HW addr = %hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
       *(writebuf+10),*(writebuf+11),*(writebuf+12),*(writebuf+13),*(writebuf+14),*(writebuf+15) );
printf("\n HW address requested for dest IP address = %s",  inet_ntop(AF_INET, writebuf+20, str, sizeof(str))  );

                      
                      
return(0);
}






void
send_v4(void)
{
	printf("\n Entered send_v4 function");
	struct icmp	*icmpptr;
 int li,status;
 char	 icmpbuf[1500];


  // format IPv4 header

  // IPv4 header length (4 bits): Number of 32-bit words in header = 5
  pngiphdr.ip_hl = IP4_HDRLEN / sizeof (uint32_t);

  // Internet Protocol version (4 bits): IPv4
  pngiphdr.ip_v = 4;

  // Type of service (8 bits)
  pngiphdr.ip_tos = 0;

  // Total length of datagram (16 bits): IP header + TCP header + TCP data
  pngiphdr.ip_len = htons (IP4_HDRLEN + payloadlen);

  // Identification field (16 bits): 
  pngiphdr.ip_id = htons (IPID_PB);

  // Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram


  
  // Zero (1 bit)
  pngip_flags[0] = 0;

  // Do not fragment flag (1 bit)
  pngip_flags[1] = 1;

  // More fragments following flag (1 bit)
  pngip_flags[2] = 0;

  // Fragmentation offset (13 bits)
  pngip_flags[3] = 0;

  pngiphdr.ip_off = htons ((pngip_flags[0] << 15)
                      + (pngip_flags[1] << 14)
                      + (pngip_flags[2] << 13)
                      +  pngip_flags[3]);

  // Time-to-Live (8 bits): default to maximum value
  pngiphdr.ip_ttl = 255;

  // ICMP protocol (8 bits):
  pngiphdr.ip_p = IPPROTO_ICMP;  //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  // Source IPv4 address (32 bits)
  if ((status = inet_pton (AF_INET, my_eth0_ip_addr_p, &(pngiphdr.ip_src))) != 1) {
    fprintf (stderr, "inet_pton() failed.\nError message: %s", strerror (status));
    exit (EXIT_FAILURE);
  }

  // Destination IPv4 address (32 bits)
  if ((status = inet_pton (AF_INET, host, &(pngiphdr.ip_dst))) != 1) {
    fprintf (stderr, "inet_pton() failed.\nError message: %s", strerror (status));
    exit (EXIT_FAILURE);
  }

  // IPv4 header checksum (16 bits): set to 0 when calculating checksum
  pngiphdr.ip_sum = 0;
  pngiphdr.ip_sum = checksum ((uint16_t *) &pngiphdr, IP4_HDRLEN);
 // printf("\n IPV4 hdr checksum calculated");

//>>>>>>>>>>>>>>format the ICMP request<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	icmpptr = (struct icmp *) icmpbuf;
	icmpptr->icmp_type = ICMP_ECHO;
	icmpptr->icmp_code = 0;
	icmpptr->icmp_id = pid;
	icmpptr->icmp_seq = nsent++;
	memset(icmpptr->icmp_data, 0xa5, datalen);	/* fill with pattern */
 
	Gettimeofday((struct timeval *) icmpptr->icmp_data, NULL);

	icmplen = 8 + datalen;		/* checksum ICMP header and data */
 
	icmpptr->icmp_cksum = 0;
	icmpptr->icmp_cksum = in_cksum((u_short *) icmpptr, icmplen);
 //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  
  // Prepare packet.

  // First part is an IPv4 header.
  memcpy (pngpacket, &pngiphdr, IP4_HDRLEN * sizeof (uint8_t));

  
  // Next part is ICMP echo request.
  memcpy ((pngpacket + IP4_HDRLEN ), icmpbuf, icmplen);

  // The kernel is going to prepare layer 2 information (ethernet frame header) for us.
  // For that, we need to specify a destination for the kernel in order for it
  // to decide where to send the raw datagram. We fill in a struct in_addr with
  // the desired destination IP address, and pass this structure to the sendto() function.
  //memset (&sain, 0, sizeof (struct sockaddr_in));
  //sain.sin_family = AF_INET;
  //sain.sin_addr.s_addr = iphdr.ip_dst.s_addr;

 
  
  /*
  printf("\n Packet contents are as follows:\n");
  for(li=0;li<PKT_LEN;li++)
    printf("%c",*( (char *)(packet+li)  ) );

 printf("\n Payload contents are as follows:\n");
 printf("\n %d %d", *(  (short int *)(payload)  ),  *(  (short int *)(payload+2)  )  );

 for(li=0;li<payloadlen;li++)
    printf("%c",*( (char *)(payload+4+li)  ) );
  
*/  

	//Sendto(sockfd, sendbuf, len, 0, pr->sasend, pr->salen);
 
  build_eth_frame(my_eth0_haddr, pinghwaddr ,my_eth0_if_index, pfd , (char *) pngpacket );
}

void build_eth_frame(char *eth_src_mac, char *eth_dest_mac,  int eth_if_index,  int eth_psockfd, 
                     char *pngpayload)
{
int eth_i,eth_j;
char *eth_ptr;
struct in_addr sinaddr;

//printf("\n Entered build_eth_frame() with if_index = %d , eth_psockfd=%d",eth_if_index,eth_psockfd);
printf("\n For ping packet, eth_src_mac= ");
 eth_ptr = eth_src_mac;
   		eth_i = IF_HADDR;
    			do {
    				printf("%.2x%s", *eth_ptr++ & 0xff, (eth_i == 1) ? " " : ":");
    			} while (--eth_i > 0);


printf("   eth_dest_mac= ");
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
socket_address.sll_pkttype  = PACKET_OTHERHOST;
//socket_address.sll_pkttype  = PACKET_BROADCAST; 

/*address length*/
socket_address.sll_halen    = 6 ;		

memcpy(socket_address.sll_addr,eth_dest_mac,6);
socket_address.sll_addr[6]  = 0x00;/*not used*/
socket_address.sll_addr[7]  = 0x00;/*not used*/


/*set the frame header*/
memcpy((void*)buffer, (void*)dest_mac, 6);
memcpy((void*)(buffer+6), (void*)src_mac, 6);
//eh->h_proto = htons(USID_PROTO);
eh->h_proto = htons(ETH_P_IP);

socklen_t sockaddrlen = sizeof(struct sockaddr_ll);
//char test_str[2];
//memcpy(test_str,&(socket_address.sll_protocol),2);

//strcpy(odrpayload.sendervm,my_host_name);

memcpy(data,pngpayload,IP4_HDRLEN+icmplen);

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
printf("\n Send result for ping packet=%d",send_result);

int eth_gso=getsockopt(eth_psockfd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen);
//printf("\n Error number %d, getsockopt return code=%d",error,eth_gso);

}