#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdlib.h>

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

static struct simple_udp_connection udp_conn;
static FILE* file;
static int* values;
static int ind=0;
static float avg=0;


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
  LOG_INFO("Received response from ");
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();
if(file==NULL)	file= fopen("/home/user/contiki-ng-mw-2122/examples/rpl-udp/values1","r");
if(values==NULL) {
	values= malloc(BUFFER_SIZE* sizeof(int));
	for(int i=0; i<BUFFER_SIZE; i++) values[i]=-1;
}

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(!feof(file)) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
	fscanf(file,"%d",&values[ind]);
	avg=0;
	for(int i=0; i<BUFFER_SIZE; i++){
		avg+= values[i];	
	}
	avg= avg/ BUFFER_SIZE;
    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
	if(avg <= THRESHOLD){		
		/* Send to DAG root */
		LOG_INFO("Avg under threshold");
		LOG_INFO("Sending avg %f ", avg);
		simple_udp_sendto(&udp_conn, &avg, sizeof(avg), &dest_ipaddr);
	}
	else{		
		LOG_INFO("Avg over threshold");
		//magari value senza & davanti
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
/*---------------------------------------------------------------------------*/
