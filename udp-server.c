/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "sys/log.h"

#include "locale.h"


#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
#define BUFFER_SIZE 6

#define UDP_BACKEND_PORT 9999
//#define ind_IPV6 "2620:9b::191d:727f"

static struct simple_udp_connection udp_conn, udp_conn_back;
static FILE* file;
static char* nameToSend
//,*backend_IPv6="2620:9b::191d:727f"
;

uip_ipaddr_t ipAddrBackend;

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);
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
  if(datalen==6*sizeof(char)){	//caso inizializzazione
  //LOG_INFO("\nSto per mandare il nome del file\n");
	if(file==NULL) file=fopen("/home/user/contiki-ng-mw-2122/examples/rpl-udp/settingsFile","r");
	if(nameToSend==NULL) nameToSend=malloc(10*sizeof(char));
 	if(file!=NULL){	
		fgets(nameToSend,10,file);
		//LOG_INFO("\nNome file mandato: %s",fullAddress);
  		simple_udp_sendto(&udp_conn, nameToSend, 10*sizeof(char), sender_addr);
	}

  }
  else if(datalen==(BUFFER_SIZE+2)*sizeof(float)){//caso di mobile device oltre treshold
	float *values=(float*) data;	
	LOG_INFO("\nReceived values from mobile device\n");
	for(int i=0; i<BUFFER_SIZE;i++)	LOG_INFO("%f ", values[i]);
	LOG_INFO("\n xAvg: %f    yAvg: %f\n", values[BUFFER_SIZE],values[BUFFER_SIZE+1]);
	//printf("{\"x\":%d,\"y\":%d,\"val\":%d,%d,%d,%d,%d,%d}\n",values[BUFFER_SIZE],values[BUFFER_SIZE+1],values[0],values[1],values[2],values[3], 		   values[4],values[5]);
//printf("{\"x\":%f,\"y\":%f,\"val\":%f,%f,%f,%f,%f,%f}\n",values[BUFFER_SIZE],values[BUFFER_SIZE+1],values[0],values[1],values[2],values[3], 		   values[4],values[5]);
  }
  else if(datalen==3*sizeof(float)){//caso mobile device under therhold
	float *values=(float*) data;	
	LOG_INFO("\nReceived avgs from mobile device");
	LOG_INFO("\n avg: %f xAvg: %f yAvg: %f\n",values[0],values[1],values[2]);
	//printf("{\"x\":%f,\"y\":%f,\"val\":%f}\n",values[1],values[2],values[0]);
	//printf("{\"x\":%d,\"y\":%d,\"val\":%d}\n",values[1],values[2],values[0]);
  }
  else if(datalen== 1+3*sizeof(float)){		//caso di sensore fisso che NON ha sfondato la threshold
	LOG_INFO("\nReceived avgs from sensor");	
	float *values= (float* ) data;
	//LOG_INFO_6ADDR(sender_addr);
	//LOG_INFO_("\n");
	printf("{\"x\":%f,\"y\":%f,\"val\":%f}\n",values[0],values[1],values[2]);
  }
  else if( datalen > sizeof (float)){	//caso di sensore fisso che ha sfondato la threshold	
	float* values= (float* ) data;
	LOG_INFO("Received array from sensor");
	for(int i=0; i<BUFFER_SIZE; i++) printf(" %f ", values[i]);	printf("\n");
	/*LOG_INFO("from ");
	LOG_INFO_6ADDR(sender_addr);
	LOG_INFO_("\n");*/
  }
	simple_udp_send(&udp_conn_back, "test", 4*sizeof(char));
#if WITH_SERVER_REPLY
  /*LOG_INFO("Sending response to ");
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");*/
  int randomValue=1;
  simple_udp_sendto(&udp_conn, &randomValue, sizeof(int), sender_addr);
#endif /* WITH_SERVER_REPLY */
}


static void
udp_rx_callback_backend(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  PROCESS_BEGIN();
  setlocale(LC_NUMERIC, "French_Canada.1252");
  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);
 
 uip_ipaddr(&ipAddrBackend, 25,29,114,127);
 simple_udp_register(&udp_conn_back, UDP_SERVER_PORT, NULL, UDP_BACKEND_PORT, udp_rx_callback_backend);
if(file==NULL) file=fopen("/home/user/contiki-ng-mw-2122/examples/rpl-udp.settingsFile","r");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
