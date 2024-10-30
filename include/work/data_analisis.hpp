#pragma once 
#include <cstdint>

namespace DATA{
#pragma pack(push, 1)
    typedef struct MacAdress
    {
        //uint8_t ignore[3];

        uint8_t ignore;
    }MACADDRESS;


    typedef struct packet_rx
    {
        //uint32_t mac_msb;
        //uint32_t mac_lsb;
        //uint8_t ignore[4];
        //uint8_t head;
        //uint16_t size;
        //uint8_t data[107];
        //uint16_t checksum;

        uint8_t ignore[4];
        uint16_t panid;                
        uint32_t mac_msb; 
        uint32_t mac_lsb;
        uint8_t ignored[6];       
        uint8_t head;
        uint16_t size;
        uint8_t data[107];
        uint16_t checksum;
    }PACKET_RX;


    typedef struct packet_tx
        {
            uint8_t head;
            uint16_t size;
            char data[107];
            uint16_t checksum;
        }PACKET_TX;

#pragma pack(pop)





}


namespace MRF24J40{

        typedef struct setINTCON {
        uint8_t tx_normal_fifo   : 1;  // Bit 0: TX Normal FIFO transmission interrupt (0 = enabled, 1 = disabled)
        uint8_t tx_gts1_fifo     : 1;  // Bit 1: TX GTS1 FIFO transmission interrupt (0 = enabled, 1 = disabled)
        uint8_t tx_gts2_fifo     : 1;  // Bit 2: TX GTS2 FIFO transmission interrupt (0 = enabled, 1 = disabled)
        uint8_t rx_fifo          : 1;  // Bit 3: RX FIFO reception interrupt (0 = enabled, 1 = disabled)
        uint8_t sec_key_req      : 1;  // Bit 4: Security key request interrupt (0 = enabled, 1 = disabled)
        uint8_t half_symbol_timer: 1;  // Bit 5: Half symbol timer interrupt (0 = enabled, 1 = disabled)
        uint8_t wake_up_alert    : 1;  // Bit 6: Wake-up alert interrupt (0 = enabled, 1 = disabled)
        uint8_t sleep_alert      : 1;  // Bit 7: Sleep alert interrupt (0 = enabled, 1 = disabled)
    } SETINTCON;
    

}