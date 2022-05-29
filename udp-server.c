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
//#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_LEVEL LOG_LEVEL_NONE

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
#define BUFFER_SIZE 6

static struct simple_udp_connection udp_conn;
static FILE* file;
static char* nameToSend;

char* replace_char(char* str, char find, char replace);
void formatString(float x,float y,float val);
char* floatAdjust(float f);

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
static void	//USED WHENEVER A MESSAGE IS RECEIVED
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  if(datalen==6*sizeof(char)){	//caso inizializzazione, legge dal settings file il nome del file che il sensore deve aprire, e glielo invia
	if(file==NULL) file=fopen("/home/user/contiki-ng-mw-2122/examples/rpl-udp/settingsFile","r");
	if(nameToSend==NULL) nameToSend=malloc(10*sizeof(char));
 	if(file!=NULL){	
		fgets(nameToSend,10,file);
  		simple_udp_sendto(&udp_conn, nameToSend, 10*sizeof(char), sender_addr);
	}

  }
  else if(datalen==3*sizeof(float)){//caso valori in entrata, li formatta correttamente e li stampa
	float *values=(float*) data;	
	LOG_INFO("\nReceived avgs from mobile device");
	LOG_INFO("\n avg: %f xAvg: %f yAvg: %f\n",values[0],values[1],values[2]);
	formatString(values[0],values[1],values[2]);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)	//START HERE
{
  PROCESS_BEGIN();
  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);
if(file==NULL) file=fopen("/home/user/contiki-ng-mw-2122/examples/rpl-udp.settingsFile","r");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
//ALL THE METHODS BELOW ARE USED TO FORMAT FLOAT CORRECTLY
char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos) {
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}

char* floatAdjust(float f){
	char *s= malloc (50*sizeof(char));
	s=gcvt(f,8,s);
	return replace_char(s,',','.');
}

void formatString(float x, float y, float val){
	char *s1,*s2,*s3;
	s1= malloc(50*sizeof(char)),s2= malloc(50*sizeof(char)),s3= malloc(50*sizeof(char));
	sprintf(s1,"{\"x\":%s",floatAdjust(x));
	sprintf(s2,"\"y\":%s",floatAdjust(y));
	sprintf(s3,"\"val\":%s}\n",floatAdjust(val));
	printf("%s,%s,%s",s1,s2,s3);
}
