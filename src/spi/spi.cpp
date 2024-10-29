#include <spi/spi.hpp>
#include <config/config.hpp>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define SPI_DEVICE  "/dev/spidev0.0"

#define CMD_WRITE 0x2
#define CMD_READ 0x3

#define READ 0b011 //Read data from memory array beginning at selected address
#define WRITE 0b010 //Write data to memory array beginning at selected address
#define WRDI 0b100//Reset the write enable latch (disable write operations)
#define WREN 0b110//Set the write enable latch (enable write operations)
#define RDSR 0b101//Read STATUS
#define WRSR 0b001//Write STATUS register

namespace SPI {

  void Spi_t::settings_spi(){
      spi->tx_buf = (unsigned long)tx_buffer;
      spi->rx_buf = (unsigned long)rx_buffer;
      spi->bits_per_word = 0;
      spi->speed_hz = spi_speed;
      spi->delay_usecs = 1;
      spi->len = 3;

          //  tx_buffer[0] = tx_buffer[1] = tx_buffer[2] = tx_buffer[3] = 0x00;
          //  rx_buffer[0]=rx_buffer[1]  = rx_buffer[2]  =0xFF;rx_buffer[3]  =0xff;
          tx_buffer[0] = 0x00;
          tx_buffer[1] = 0x00;
          tx_buffer[2] = 0x00;
          tx_buffer[3] = 0x00;
          rx_buffer[0] = 0xFF;
          rx_buffer[1] = 0xFF;
          rx_buffer[2] = 0xFF;
          rx_buffer[3] = 0xff;
    return;
  }


  void Spi_t::init(){
  fs = open(SPI_DEVICE, O_RDWR);
  if(fs < 0) {
      msj_fail();
      exit(EXIT_FAILURE);
    }
    ret = ioctl(fs, SPI_IOC_RD_MODE, &scratch32);
  if(ret != 0) {
        msj_fail();
        if(fs)close(fs);
        exit(EXIT_FAILURE);
    }
    scratch32 |= SPI_MODE_0;
    ret = ioctl(fs, SPI_IOC_WR_MODE, &scratch32);   //SPI_IOC_WR_MODE32
  if(ret != 0) {
      msj_fail();
      close(fs);
      exit(EXIT_FAILURE);
    }
    ret = ioctl(fs, SPI_IOC_RD_MAX_SPEED_HZ, &scratch32);
  if(ret != 0) {
      close(fs);
      exit(EXIT_FAILURE);
    }
      scratch32 = spi_speed;
      ret = ioctl(fs, SPI_IOC_WR_MAX_SPEED_HZ, &scratch32);

      if(ret != 0) {
          msj_fail();
          close(fs);
          exit(EXIT_FAILURE);
      }
      return;
  }


const uint8_t Spi_t::Transfer2bytes(const uint16_t cmd){
    spi->len = sizeof(cmd);
    rx_buffer[0]=rx_buffer[1]=0xff;
    rx_buffer[2]=rx_buffer[3]=0x00;
    memcpy(tx_buffer, &cmd, sizeof(cmd));
    ret = ioctl(fs, SPI_IOC_MESSAGE(1), spi.get());
    if((cmd>>8&0xff)==0x00)
        printDBGSpi(); 
      //if(ret != 0) return rx_buffer[1];  
  return rx_buffer[1];
  }


  const uint8_t Spi_t::Transfer3bytes(const uint32_t cmd){
    spi->len = 3;
    rx_buffer[0]=rx_buffer[1]=rx_buffer[2]==0xff;
    rx_buffer[3]=0x00;
    memcpy(tx_buffer, &cmd, sizeof(cmd));
    ret = ioctl(fs, SPI_IOC_MESSAGE(1), spi.get());
      if((cmd>>16&0xff)==0x00) 
        printDBGSpi();
        //if(ret != 0) return rx_buffer[2];       
    return rx_buffer[2];
    }


    void Spi_t::spi_close(){
        if(fs)close(fs);
      return;
    }

    Spi_t::Spi_t()
    :      
      tx_buffer { 0x00 },
      rx_buffer { 0x00 },
      spi_speed { SPI_SPEED }, 
      spi       { std::make_unique<struct spi_ioc_transfer >() } 
    {
          #ifdef DBG_SPI
              std::cout<<"Spi()\n";
          #endif
          settings_spi();   
          init(); 
        return;
    }

      Spi_t::~Spi_t(){
      spi_close();
      #ifdef DBG_SPI
          std::cout<<"~Spi()\n";
      #endif
      exit(EXIT_SUCCESS);
    }


}//end namespace SPI_H
