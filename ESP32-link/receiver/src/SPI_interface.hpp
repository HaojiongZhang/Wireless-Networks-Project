#include <ESP32DMASPISlave.h>

static const uint32_t BUFFER_SIZE=8192;
extern uint8_t* spi_slave_tx_buf;
extern uint8_t* spi_slave_rx_buf;

extern ESP32DMASPI::Slave slave;

void init_spi();
