/*
* CS4222/5422: Assignment 3b
* Perform neighbour discovery
*/

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "net/linkaddr.h"
#include <string.h>
#include <stdio.h> 
#include "node-id.h"

//DISCO PROTOCOL VARIABLES
#define PRIME1 7
#define PRIME2 13
#define SLOT_TIME_INTERVAL RTIMER_SECOND/10 //a slot has an interval of 0.1s
unsigned long slot_num = 0;
bool isActive = false;

#define RSSI_THRESHOLD -70

//FOR NEW SENSORTAGS DETECTED
typedef struct
{
  unsigned long sensorTag_id;

} sensorTag;
#define SENSORTAG_LIST_LENGTH 3
static volatile sensorTag sensorTag_list[SENSORTAG_LIST_LENGTH];
static int sensorTag_list_index = 0;


// For neighbour discovery, we would like to send message to everyone. We use Broadcast address:
linkaddr_t dest_addr;
linkaddr_t sensorTag_A;

#define NUM_SEND 2
/*---------------------------------------------------------------------------*/
typedef struct {
  unsigned long src_id;
  unsigned long timestamp;
  unsigned long seq;
  
} discovery_packet_struct;

typedef struct
{
  unsigned long src_id;
  unsigned long timestamp;

} request_lux_packet_struct;

typedef struct
{
  unsigned long src_id;
  unsigned long timestamp;
  int lux_readings[10];

} lux_readings_packet_struct;

// sender timer implemented using rtimer
static struct rtimer rt;
// Protothread variable
static struct pt pt;
// Structure holding the data to be transmitted
static discovery_packet_struct discovery_packet;
static request_lux_packet_struct request_lux_packet;
static lux_readings_packet_struct lux_readings_packet;
// Current time stamp of the node
unsigned long curr_timestamp;

bool toTransmitRequest = false;

// Starts the main contiki neighbour discovery process
PROCESS(nbr_discovery_process, "cc2650 neighbour discovery process");
AUTOSTART_PROCESSES(&nbr_discovery_process);

// Function called after reception of a packet
void receive_packet_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) 
{
  // Check if the received packet size matches with what we expect it to be
  if(len == sizeof(discovery_packet)) {
    curr_timestamp = clock_time();

    static discovery_packet_struct received_packet_data;
    // Copy the content of packet into the data structure
    memcpy(&received_packet_data, data, len);
    // Print the details of the received packet
    printf("Received neighbour discovery packet %lu with rssi %d from %ld", received_packet_data.seq, (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI),received_packet_data.src_id);
    printf("\n");

    bool new_sensorTag = true;
    for(int i = 0;i < 3;i++){
        if(sensorTag_list[i].sensorTag_id == received_packet_data.src_id){
            new_sensorTag = false;
        }
    }
    if(new_sensorTag){
        printf("%lu DETECT %lu\n", curr_timestamp / CLOCK_SECOND, received_packet_data.src_id);
        sensorTag_list[sensorTag_list_index].sensorTag_id = received_packet_data.src_id;
        sensorTag_list_index++;
        if(sensorTag_list_index > SENSORTAG_LIST_LENGTH - 1){
            sensorTag_list_index = 0;
        }
    }

    unsigned short rssi = (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI);
    if(rssi > RSSI_THRESHOLD){
        toTransmitRequest = true;
        linkaddr_copy(&sensorTag_A, src);
    }

  }else if(len == sizeof(lux_readings_packet)){
    curr_timestamp = clock_time();

    static lux_readings_packet_struct received_packet_data;
    // Copy the content of packet into the data structure
    memcpy(&received_packet_data, data, len);
    // Print the details of the received packet
    printf("Received neighbour lux readings packet with rssi %d from %ld", (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI),received_packet_data.src_id);
    printf("\n");    
    
    /*
    bool new_sensorTag = true;
    for(int i = 0;i < 3;i++){
        if(sensorTag_list[i].sensorTag_id == received_packet_data.src_id){
            new_sensorTag = false;
        }
    }
    if(new_sensorTag){
        printf("%lu DETECT %lu\n", curr_timestamp / CLOCK_SECOND, received_packet_data.src_id);
        sensorTag_list[sensorTag_list_index].sensorTag_id = received_packet_data.src_id;
        sensorTag_list_index++;
        if(sensorTag_list_index > SENSORTAG_LIST_LENGTH - 1){
            sensorTag_list_index = 0;
        }
    }
    */
    printf("Light: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d \n", received_packet_data.lux_readings[0], 
    received_packet_data.lux_readings[1], received_packet_data.lux_readings[2], received_packet_data.lux_readings[3], 
    received_packet_data.lux_readings[4], received_packet_data.lux_readings[5], received_packet_data.lux_readings[6], 
    received_packet_data.lux_readings[7], received_packet_data.lux_readings[8], received_packet_data.lux_readings[9]);

    toTransmitRequest = false;          
  }
}

// Scheduler function for the sender of neighbour discovery packets
char sender_scheduler(struct rtimer *t, void *ptr) {
  // Begin the protothread
  PT_BEGIN(&pt);

  // Get the current time stamp
  curr_timestamp = clock_time();
  printf("Start clock %lu ticks, timestamp %3lu.%03lu\n", curr_timestamp, curr_timestamp / CLOCK_SECOND, 
  ((curr_timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND);

  while(1){
    if(toTransmitRequest){
        NETSTACK_RADIO.on();

        nullnet_buf = (uint8_t *)&request_lux_packet; //data transmitted
        nullnet_len = sizeof(request_lux_packet); //length of data transmitted
        curr_timestamp = clock_time();
        request_lux_packet.src_id = node_id;
        request_lux_packet.timestamp = curr_timestamp;

        NETSTACK_NETWORK.output(&sensorTag_A); 
        //rtimer_set(t, RTIMER_TIME(t) + (SLOT_TIME_INTERVAL*3), 1, (rtimer_callback_t)sender_scheduler, ptr);
        //PT_YIELD(&pt);

        NETSTACK_RADIO.off();
    }
    if(isActive){ //turns off radio after 1 SLOT_TIME_INTERVAL to put sensorTag to sleep
        NETSTACK_RADIO.off();
        isActive = false;
    }
    //if slot reached prime1 or prime2 value
    if((slot_num % PRIME1 == 0 || slot_num % PRIME2 == 0)){
        //active
        isActive = true;
        NETSTACK_RADIO.on();
        //transmit once
        nullnet_buf = (uint8_t *)&discovery_packet; //data transmitted
        nullnet_len = sizeof(discovery_packet); //length of data transmitted
        discovery_packet.seq++; 
        curr_timestamp = clock_time();
        discovery_packet.timestamp = curr_timestamp;
        printf("Send seq# %lu  @ %8lu ticks   %3lu.%03lu\n", discovery_packet.seq, curr_timestamp, curr_timestamp / CLOCK_SECOND, ((curr_timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND);
        NETSTACK_NETWORK.output(&dest_addr); //Packet transmission              
    }else{
        NETSTACK_RADIO.off();
        //printf("Sleep\n");
    }

    slot_num++;
    rtimer_set(t, RTIMER_TIME(t) + SLOT_TIME_INTERVAL, 1, (rtimer_callback_t)sender_scheduler, ptr);
    PT_YIELD(&pt);
  }
  PT_END(&pt);
}

// Main thread that handles the neighbour discovery process
PROCESS_THREAD(nbr_discovery_process, ev, data)
{
 // static struct etimer periodic_timer;
  PROCESS_BEGIN();
    // initialize data packet sent for neighbour discovery exchange
  discovery_packet.src_id = node_id; //Initialize the node ID
  discovery_packet.seq = 0; //Initialize the sequence number of the packet
  nullnet_set_input_callback(receive_packet_callback); //initialize receiver callback
  linkaddr_copy(&dest_addr, &linkaddr_null);
  printf("CC2650 neighbour discovery\n");
  printf("Node %d will be sending packet of size %d Bytes\n", node_id, (int)sizeof(discovery_packet_struct));

  


  // Start sender in one millisecond.
  rtimer_set(&rt, RTIMER_NOW() + (RTIMER_SECOND / 1000), 1, (rtimer_callback_t)sender_scheduler, NULL);
  PROCESS_END();
}