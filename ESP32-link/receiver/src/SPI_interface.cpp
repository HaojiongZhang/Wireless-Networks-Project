#include <SPI_interface.hpp>

uint8_t* spi_slave_tx_buf;
uint8_t* spi_slave_rx_buf;
ESP32DMASPI::Slave slave;

void init_spi(){
    spi_slave_tx_buf = slave.allocDMABuffer(BUFFER_SIZE);
    spi_slave_rx_buf = slave.allocDMABuffer(BUFFER_SIZE);
    
    slave.setDataMode(SPI_MODE0);
    slave.setMaxTransferSize(BUFFER_SIZE);
    slave.begin();
}
