#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdlib.h>
#include <string.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
#define BUFFER_SIZE 6
#define THRESHOLD 15

static struct simple_udp_connection udp_conn;

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL		  (60 * CLOCK_SECOND)
#define INIT_INTERVAL		  (5 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
static FILE* file;
static int* values, *xPos, *yPos;
static float *mobileData;	//mobileData contains avgValue, avgX, avgY. It's used to send everything to the router
static int ind=0, flag=0, mobileFlag=0;	//mobileFlag is 1 if the device moves, so it has a position for each value read.
static char* fileName;
static float avg=0, xAvg=0, yAvg=0;
static process_event_t init_event;


/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  if(datalen=10*sizeof(char)){
	flag=1;
	mobileFlag= (strstr((char*) data, "values")==NULL)?1:0;
	fileName=malloc(70*sizeof (char));
	strcpy(fileName,"/home/user/contiki-ng-mw-2122/examples/rpl-udp/");
	strcat(fileName,(char*) data);
	fileName[strlen(fileName)-1]='\0';
	LOG_INFO("\nNome file ricevuto: %s\n",fileName);
	process_post(&udp_client_process, init_event, fileName);
  }
else{
  LOG_INFO("Received response from ");
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
 }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);
  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);

while(flag==0){
LOG_INFO("\nNon connesso");
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
	if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr))
		flag=1;
    etimer_set(&periodic_timer, INIT_INTERVAL - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
}
flag=0;
simple_udp_sendto(&udp_conn, "Values", 6 * sizeof(char), &dest_ipaddr);

PROCESS_WAIT_EVENT();//entra qui!vedere come creare il percorso giusto!
if(ev==init_event)	LOG_INFO("\nfileName: %s",fileName);

if(file==NULL)	 file= fopen(fileName,"r");
if(values==NULL && file!=NULL) {
	values= malloc(BUFFER_SIZE* sizeof(int));
	for(int i=0; i<BUFFER_SIZE; i++) values[i]=-1;
	if(mobileFlag){
		xPos= malloc(BUFFER_SIZE* sizeof(int));
		yPos= malloc(BUFFER_SIZE* sizeof(int));
	}
} 

  while(!feof(file)) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
	fscanf(file,"%d",&values[ind]);
	avg=0, xAvg=0, yAvg=0;
	for(int i=0; i<BUFFER_SIZE; i++){
		avg+= values[i];	
		if(mobileFlag){
			xAvg+=xPos[i];
			yAvg+=yPos[i];
		}
	}
	avg= avg/ BUFFER_SIZE;
    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
	if(avg <= THRESHOLD){		
		/* Send to DAG root */
	   if(!mobileFlag){
		LOG_INFO("Avg under threshold from static device");
		LOG_INFO("Sending avg %f ", avg);
		simple_udp_sendto(&udp_conn, &avg, sizeof(avg), &dest_ipaddr);}
	   else{
		if(mobileData==NULL)	mobileData=malloc (3*sizeof(float));
		mobileData[0]=avg;
		mobileData[1]=xAvg;
		mobileData[2]=yAvg;
		LOG_INFO("Avg under threshold from mobile device");
		simple_udp_sendto(&udp_conn, mobileData, 3*sizeof(float), &dest_ipaddr);}		
	   }	  
	}
	else{	//adesso devo crare il vettore che contiene i 6 valori, più la media della posizione. Poi fare la parte del server e runnare per vedere se va.	
		LOG_INFO("Avg over threshold");
		simple_udp_sendto(&udp_conn, values, BUFFER_SIZE* sizeof(int), &dest_ipaddr);
	}
      ind=(ind+1)%BUFFER_SIZE;
    } else {
      LOG_INFO("Not reachable yet\n");
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*-------------------------------------------------------------------------*/
