#include <stdio.h>
#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/light-sensor.h"
#include "dev/leds.h"
#include "dev/cc2420.h"
#include "net/rime.h"

#define ON 1
#define OFF 0

#define HEADER "RTST"
#define PACKET_SIZE 20
#define PORT 9345

struct indicator{

  int onoff;
  int led;
  clock_time_t interval;
  struct etimer timer;

};

static struct etimer send_timer;
static struct indicator recv, other, flash;

/*---------------------------------------------------------------------------*/
PROCESS(light_radio_process, "light radio");
AUTOSTART_PROCESSES(&light_radio_process);
/*---------------------------------------------------------------------------*/

static void set(struct indicator *indicator, int onoff) {
  if(indicator->onoff ^ onoff) {
      indicator->onoff = onoff;
      if(onoff) {
        leds_on(indicator->led);
      }
      else {
        leds_off(indicator->led);
      }
  }
  if(onoff) {
    etimer_set(&indicator->timer, indicator->interval);
  }
}

static void abc_recv(struct abc_conn *c, const rimeaddr_t *from){
  /* packet received */
  if(packetbuf_datalen() < PACKET_SIZE || strncmp((char *)packetbuf_dataptr(), HEADER, sizeof(HEADER))){
    /* invalid message */
  }
  else{
    PROCESS_CONTEXT_BEGIN(&light_radio_process);
    set(&recv, ON);

    uint8_t light_value = ((uint8_t *)packetbuf_dataptr())[sizeof(HEADER)];

    /* synchronize te sending to keep the nodes from sending simutaneously */
    etimer_set(&send_timer, CLOCK_SECOND);
    etimer_adjust(&send_timer, -(int)(CLOCK_SECOND >> 1));
    printf("Activate Light: %d\n", light_value);
    PROCESS_CONTEXT_END(&light_radio_process);
  }
}

static const struct abc_callbacks abc_callbacks = { abc_recv };
static struct abc_conn abc;

PROCESS_THREAD(light_radio_process, ev, data){

  PROCESS_BEGIN();
  
  static uint8_t txpower = CC2420_TXPOWER_MAX;

   /* Initialize the indicators */
  recv.onoff = other.onoff = flash.onoff = OFF;
  recv.interval = other.interval = CLOCK_SECOND;
  flash.interval = 1;
  flash.led = LEDS_RED;
  recv.led = LEDS_GREEN;
  other.led = LEDS_BLUE;

  abc_open(&abc, PORT, &abc_callbacks);

  SENSORS_ACTIVATE(button_sensor);
  
  etimer_set(&send_timer, CLOCK_SECOND);

  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == PROCESS_EVENT_TIMER) {
        if(data == &send_timer){
            etimer_reset(&send_timer);
            SENSORS_ACTIVATE(light_sensor);
            uint8_t light_value = light_sensor.value(0);
            cc2420_set_txpower(txpower);
            /* send packet */
            packetbuf_copyfrom(HEADER, sizeof(HEADER));
            ((uint8_t *)packetbuf_dataptr())[sizeof(HEADER)] = light_value;
            /* send arbitrary data to fill the packet size */
            packetbuf_set_datalen(PACKET_SIZE);
            set(&flash, ON);
            abc_send(&abc);
        } 
        else if(data == &other.timer) {
            set(&other, OFF);
        }
        else if(data == &recv.timer) {
            set(&recv, OFF);
        }
        else if(data == &flash.timer) {
            set(&flash, OFF);
        }       
    }
    else if(ev == sensors_event && data == &button_sensor) {
        //
    }
  }
  PROCESS_END();
}
