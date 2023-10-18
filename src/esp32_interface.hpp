#include <pthread.h>

pthread_t esp32_interface;



int ESP32interface_init(size_t packetMaxSize);

int ESP32_send(char* packet, size_t length);

int ESP32_recv(char* packet);

void ESP32_disconnect();