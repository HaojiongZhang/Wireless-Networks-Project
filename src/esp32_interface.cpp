#include <esp32_interface.hpp>
#include <wiringPiSPI.h>

#include <stdio.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int LOCKED = -1, UNLOCKED = 0;
const int SPIchannel = 0;

unsigned char *TX_buf, *RX_buf, *RW_buf;
int TX_buf_locked, RX_buf_locked;
int TX_ready, RX_ready;
int terminate;
int packetSize;

int isNullPkt();
void setNullPkt();

/* Thread for exchanging packet between pi and ESP32 through SPI */
void * ESP32_worker(void *){
    // do sth

    while (1){
        if (terminate != 0){
            break;
        }


        if (TX_ready == 1 && TX_buf_locked == UNLOCKED){
            /* packet ready for transmit */
            TX_buf_locked = LOCKED;
            memcpy(RW_buf, TX_buf, packetSize);
            TX_ready = 0;
            TX_buf_locked = UNLOCKED;
        }
        else {
            setNullPkt();
        }

        /* Write RW_buf to ESP32 through SPI and read packet from ESP32 to the same buffer */
        wiringPiSPIDataRW(SPIchannel, RW_buf, packetSize);
        if (isNullPkt()){
            /* no packet received from ESP32 */
            continue;
        }

        while(RX_ready || RX_buf_locked == LOCKED){ 
            /* wait until last packet is received by PI host */ 
        }
        RX_buf_locked = LOCKED;
        memcpy(RX_buf, RW_buf, packetSize);
        RX_ready = 0;
        RX_buf_locked = UNLOCKED;
    }

    return NULL;
}

int ESP32interface_init(size_t packetMaxSize){
    TX_buf = (unsigned char *)malloc(packetMaxSize);
    RX_buf = (unsigned char *)malloc(packetMaxSize);
    RW_buf = (unsigned char *)malloc(packetMaxSize);
    packetSize = packetMaxSize;
    TX_buf_locked = UNLOCKED;
    RX_buf_locked = UNLOCKED;
    terminate = 0;

    pthread_create(&esp32_interface, NULL, ESP32_worker, NULL);
}

/* Mark a packet to send to ESP32 but does not send it immediately (non blocking?)
   Return: 0 if pkt is rdy to send (Does not mean ACKed), 
          -1 if last packet is still sending 
*/
int ESP32_send(char* packet, size_t length){
    if (TX_buf_locked != UNLOCKED){
        /* bus is busy */
        return TX_buf_locked;
    }

    TX_buf_locked = LOCKED;
    memcpy(TX_buf, packet, length);
    TX_buf_locked = UNLOCKED;
    TX_ready = 1;
    packetSize = length;
    return 0;
}

/* Get the new packet that ESP32 has received 
   Param: packet: pointer to where RX packet (if recved) will be stored 
   Return: 0 if a packet is received
          -1 if a packet is still ins transmission
          -2 if no new packet has arrived
*/
int ESP32_recv(char* packet){
    if (RX_buf_locked != UNLOCKED){
        return RX_buf_locked;
    }
    if (RX_ready){
        return -2;
    }

    if (RX_buf_locked == 1){
        RX_buf_locked = LOCKED;
        memcpy(packet, RX_buf, packetSize);
        RX_ready = 1;
        RX_buf_locked = UNLOCKED;
    }
    return 0;
}

void ESP32_disconnect(){
    terminate = 1;
}

/* Check if RW_buf contains a NULL packet */
int isNullPkt(){
    return (RW_buf[0] == 0 && memcmp(RW_buf, RW_buf+1, packetSize-1) == 0);
}

/* Create a NULL packet in RW_buf */
void setNullPkt(){
    memset(RW_buf, 0, packetSize);
}