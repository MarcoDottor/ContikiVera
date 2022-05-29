#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdlib.h>
#include <string.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_NONE
//#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
#define BUFFER_SIZE 6
#define THRESHOLD 15

static struct simple_udp_connection udp_conn;

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL		  (10 * CLOCK_SECOND)
#define INIT_INTERVAL		  (5 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
static FILE* file;
static float *xPos, *yPos, *values;	//mobileData contains avgValue, avgX, avgY. It's used to send everything to the router
static int ind=0, flag=0, mobileFlag=0,	//mobileFlag is 1 if the device moves, so it has a position for each value read.
		enough_values_flag=0; //indica se ho letto almeno sei valori, e quindi se ha senso iniziare a mandare le medie.
static char* fileName;
static float avg=0, xAvg=0, yAvg=0,xSensor=0, ySensor=0;  //xSensor e ySensor sono i valori fissi delle coordinate di un sensore
static process_event_t init_event;


/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void	//WHENEVER A MESSAGE IS RECEIVED THIS METHOD GETS CALLED
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  if(datalen==10*sizeof(char)){	//10 IS THE SIZE GIVEN TO THE MESSAGE CONTAINING THE NAME OF THE FILE TO READ
	flag=1;
	mobileFlag= (strstr((char*) data, "m_trace")!=NULL)?1:0;  //FILE FOR MOBILE TRACES CONTAIN "m_trace" in their name
	fileName=malloc(70*sizeof (char));
	strcpy(fileName,"/home/user/contiki-ng-mw-2122/examples/rpl-udp/");
	strcat(fileName,(char*) data);	//GENERATE THE CORRECT NAME OF THE FILE
	fileName[strlen(fileName)-1]='\0';	//NECESSARY TO CHANGE \n AT THE END, WHICH GIVES PROBLEMS
	process_post(&udp_client_process, init_event, fileName);	//CREATES AN EVENT NECESSARY TO CONTINUE THE PROGRAM, LOOK AT LINES 80-81
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)	//STARTS HERE
{
  static struct etimer periodic_timer, initialDelayTimer;
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);	//TRIES TO CONNECT TO THE UDP-SERVER
  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);	//SETS A TIMER TO SIMULATE REAL-WORLD BEHAVIOR

while(flag==0){
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));	//PAUSES EACH TIME IT TRIES
	if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) //CHECKS IF CONNECTED TO SERVER
		flag=1;
    etimer_set(&periodic_timer, INIT_INTERVAL - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));	//SETS A TIMER TO SIMULATE REAL-WORLD BEHAVIOR
}
flag=0;
simple_udp_sendto(&udp_conn, "Values", 6 * sizeof(char), &dest_ipaddr);	//SENDS A MESSAGE TO ASK FOR THE NAME OF THE FILE TO READ

PROCESS_WAIT_EVENT();	//WAITS FOR ANY EVENT
if(ev==init_event){}	//IN CALLBACK (ROW 57) AN INIT_EVEN IS CREATED WHEN A REPLY IS RECEIVED FROM THE SERVER

if(file==NULL)	 file= fopen(fileName,"r");
if(values==NULL && file!=NULL) {
	values= malloc(BUFFER_SIZE* sizeof(float));
	for(int i=0; i<BUFFER_SIZE; i++) values[i]=-1;
	if(mobileFlag){	//POSITIONS FOR EACH RECORDING ARE STORED IN A VECTOR
		xPos= malloc(BUFFER_SIZE* sizeof(float));
		yPos= malloc(BUFFER_SIZE* sizeof(float));
	}
} 
  while(!feof(file)) {
      if(ind==5) {enough_values_flag=1;} //THIS FLAG IS TRUE IF I'VE READ AT LEAST 6 VALUES, AND REMAINS TRUE FOR ALL THE DURATION OF THE PROGRAM
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
	if(!mobileFlag) {//STATIC SENSOR
		if(ind==0 && !enough_values_flag){//IF I HAVN'T READ ANY VALUES YET, I KNOW THE FIRST TWO ARE X AND Y OF THE STATIC SENSOR
		static float a,b;
		fscanf(file,"%f %f",&a, &b);
		xSensor=a; ySensor=b;
		}
		fscanf(file,"%f",&values[ind]); //ALL OTHER VALUES ARE NOISE LEVELS
		LOG_INFO("\nHo letto il valore %f\n", values[ind]);
	}
	else {//MOBILE SENSOR
		if(ind==0 && !enough_values_flag){//CHECK IF I HAVN'T READ ANY VALUES YET, AND SETS AN INITIAL WAIT TO SIMULATE REAL WORLD BEHAVIOR
			int initialWait;
			fscanf(file,"%d",&initialWait);
	    		etimer_set(&initialDelayTimer, initialWait);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&initialDelayTimer));
		}	
		fscanf(file,"%f %f %f",&values[ind],&xPos[ind],&yPos[ind]);//ALL OTHER LINES ARE NOISE LEVELS AND THEIR POSITIONS
		LOG_INFO("\nHo letto i valori x:%f y:%f val:%f\n",xPos[ind],yPos[ind],values[ind]);
	}
	avg=0, xAvg=0, yAvg=0;	//COMPUTE NOISE AVG, AND POSITION AVG FOR MOBILE SENSORS
	for(int i=0; i<BUFFER_SIZE; i++){
		avg+= values[i];	
		if(mobileFlag){
			xAvg+=xPos[i];
			yAvg+=yPos[i];
		}
	}
	avg=avg/BUFFER_SIZE;
	if(mobileFlag)	{xAvg=xAvg/BUFFER_SIZE;		yAvg=yAvg/BUFFER_SIZE;}
	//l'if sotto impone che il nodo sia connesso e contenga 6 valori prima di comunicare col router 
    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
	if(enough_values_flag){
		float toSend[3];
		if(avg <= THRESHOLD){ //SENDING AVG
			if(mobileFlag)	{
				toSend[0]=xAvg;
				toSend[1]=yAvg;
			}
			else{
				toSend[0]=xSensor;
				toSend[1]=ySensor;
			}
			toSend[2]=avg;	
			simple_udp_sendto(&udp_conn, toSend, 3*sizeof(float), &dest_ipaddr);//SENDING THE ARRAY CREATED ABOVE	
		}	
		else{	//SENDING ALL VALUES 
			for(int i=0; i<BUFFER_SIZE; i++){
				if(mobileFlag) {
					toSend[0]=xPos[i];	
					toSend[1]=yPos[i];
				}
				else {
					toSend[0]=xSensor;	
					toSend[1]=ySensor;
				}	
				toSend[2]=values[i];
				simple_udp_sendto(&udp_conn, toSend, 3*sizeof(float), &dest_ipaddr);//SENDING THE ARRAYS CREATED ABOVE
			}
		}
	}
	else{
		LOG_INFO("NO ABBASTANZA VALORI \n");	
	}
      ind=(ind+1)%BUFFER_SIZE;	//CHANGE THE INDEX TO UPDATE WITH NEW VALUES
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
