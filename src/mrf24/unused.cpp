
//unused.cpp
#include <mrf24/radio.hpp>
#include <mrf24/mrf24j40.hpp>
#include <mrf24/mrf24j40._microchip.hpp>
#include <mrf24/mrf24j40_cmd.hpp>

#include <string_view>
#include <zlib.h>  // Para usar crc32
#include <string>
#include <cstdint>
#include <cstddef>

namespace MRF24J40{
    uint8_t 
    set_intcon_value(const SETINTCON& config) {
        return *reinterpret_cast<const uint8_t*>(&config);
    }


    void
    Mrf24j::mrf24j40_init(void){
    
    uint8_t i;
    /*
    * bit 7:3 reserved: Maintain as ‘0’
    * bit 2   RSTPWR: Power Management Reset bit
    *         1 = Reset power management circuitry (bit is automatically cleared to ‘0’ by hardware)
    * bit 1   RSTBB: Baseband Reset bit
    *         1 = Reset baseband circuitry (bit is automatically cleared to ‘0’ by hardware)
    * bit 0   RSTMAC: MAC Reset bit
    *         1 = Reset MAC circuitry (bit is automatically cleared to ‘0’ by hardware)
    */
    write_short(MRF_SOFTRST, 0b00000111);

    /*
    * wait until the radio reset is completed
    */

    //do {
    //  i = read_short(MRF_SOFTRST);
    //} while((i & 0b0000111) != 0);
    
    /*
    * bit 7   FIFOEN: FIFO Enable bit 1 = Enabled (default). Always maintain this bit as a ‘1’.
    * bit 6   reserved: Maintain as ‘0’
    * bit 5:2 TXONTS<3:0>: Transmitter Enable On Time Symbol bits(1)
    *         Transmitter on time before beginning of packet. Units: symbol period (16 μs).
    *         Minimum value: 0x1. Default value: 0x2 (2 * 16 μs = 32 μs). Recommended value: 0x6 (6 * 16 μs = 96 μs).
    * bit 1:0 TXONT<8:7>: Transmitter Enable On Time Tick bits(1)
    *         Transmitter on time before beginning of packet. TXONT is a 9-bit value. TXONT<6:0> bits are located
    *         in SYMTICKH<7:1>. Units: tick (50 ns). Default value = 0x028 (40 * 50 ns = 2 μs).
    */

    write_short(MRF_PACON2, 0b10011000);

    set_channel(CHANNEL);

    write_long(MRF_RFCON1, 0b00000010);      // program the RF and Baseband Register 
                                                        // as suggested by the datasheet 
    write_long(MRF_RFCON2, 0b10000000);      // enable PLL 

    //mrf24j40_set_tx_power                 // set power 0dBm (plus 20db power amplifier 20dBm)
    write_long(MRF_RFCON3, 0b00000000);
    
    // Set up 
    // 
    // bit 7   '1' as suggested by the datasheet
    // bit 6:5 '00' reserved
    // bit 4   '1' recovery from sleep 1 usec
    // bit 3   '0' battery monitor disabled
    // bit 2:0 '000' reserved
    
    write_long(MRF_RFCON6, 0b10010000);
    write_long(MRF_RFCON7, 0b10000000);      // Sleep clock = 100kHz 
    write_long(MRF_RFCON8, 0b00000010);      // as suggested by the datasheet 
    write_long(MRF_SLPCON1, 0b00100001);     // as suggested by the datasheet 

    // Program CCA, RSSI threshold values 
    write_short(MRF_BBREG2, 0b01111000);     // Recommended value by the datashet 
    write_short(MRF_CCAEDTH, 0b01100000);    // Recommended value by the datashet 

    #ifdef MRF24J40MB
    // Activate the external amplifier needed by the MRF24J40MB 
    write_long(MRF_TESTMODE, 0b0001111);
    std::printf("MRF24J40 Init Amplifier activated \n");
    #endif

    #ifdef ADD_RSSI_AND_LQI_TO_PACKET
    // Enable the packet RSSI 
    write_short(MRF_BBREG6, 0b01000000);
    std::printf("MRF24J40 Init append RSSI and LQI to packet\n");
    #endif

    //
    // Wait until the radio state machine is not on rx mode
    //

    do {
        i = read_long(RFSTATE);
    } while((i & 0xA0) != 0xA0);

    i = 0;

    #ifdef MRF24J40_DISABLE_AUTOMATIC_ACK
    i = i | 0b00100000;
    std::printf("MRF24J40 Init NO_AUTO_ACK\n");
    #endif

    #ifdef MRF24J40_PAN_COORDINATOR
    i = i | 0b00001000;
    std::printf("MRF24J40 Init PAN COORD\n");
    write_short(MRF_ORDER, 0b11111111);
    #endif

    #ifdef MRF24J40_COORDINATOR
    i = i | 0b00000100;
    std::printf("MRF24J40 Init COORD\n");
    #endif

    #ifdef MRF24J40_ACCEPT_WRONG_CRC_PKT
    i = i | 0b00000010;
    std::printf("MRF24J40 Init Accept Wrong CRC\n");
    #endif

    #ifdef MRF24J40_PROMISCUOUS_MODE
    i = i | 0b00000001;
    std::printf("MRF24J40 Init PROMISCUOUS MODE\n");
    #endif
    
    // Set the RXMCR register.
    // Default setting i = 0x00, which means:
    // - Automatic ACK;
    // - Device is not a PAN coordinator;
    // - Device is not a coordinator;
    // - Accept only packets with good CRC
    // - Discard packet when there is a MAC address mismatch,
    //   illegal frame type, dPAN/sPAN or MAC short address mismatch.    
    write_short(MRF_RXMCR, i);
    std::printf("RXMCR 0x%X\n", i);

    // Set the TXMCR register.
    // bit 7   '0' Enable No Carrier Sense Multiple Access (CSMA) Algorithm.
    // bit 6   '0' Disable Battery Life Extension Mode bit.
    // bit 5   '0' Disable Slotted CSMA-CA Mode bit.
    // bit 4:3 '11' MAC Minimum Backoff Exponent bits (macMinBE).
    // bit 2:0 '100' CSMA Backoff bits (macMaxCSMABackoff)   
    write_short(MRF_TXMCR, 0b00011100);

    i = read_short(MRF_TXMCR);
    std::printf("TXMCR 0x%X\n", i);
    
        // Set TX turn around time as defined by IEEE802.15.4 standard
        //
    write_short(MRF_TXSTBL, 0b10010101);
    write_short(MRF_TXTIME, 0b00110000);

    #ifdef INT_POLARITY_HIGH
    // Set interrupt edge polarity high 
    write_long(MRF_SLPCON0, 0b00000011);
    std::printf("MRF24J40 Init INT Polarity High\n");
    #else
    write_long(MRF_SLPCON0, 0b00000001);
    std::printf("MRF24J40 Init INT Polarity Low\n");
    #endif

    std::printf("MRF24J40 Inititialization completed\n");
    
    //last_lqi = 0;
    //last_rssi = 0;
    //status_tx = MRF_TX_ERR_NONE;
    //pending = 0;
    //receive_on = 1;
    //ENERGEST_ON(ENERGEST_TYPE_LISTEN);
    
    //reset_rf_state_machine
    const uint8_t rfctl = read_short(MRF_RFCTL);
    write_short(MRF_RFCTL, rfctl | 0b00000100);
    write_short(MRF_RFCTL, rfctl & 0b11111011);
  
    // Flush RX FIFO 
    write_short(MRF_RXFLUSH, read_short(MRF_RXFLUSH) | 0b00000001);

    //process_start(&mrf24j40_process, NULL);


    // Setup interrupts.
    //
    // set INTCON
    // bit 7 '1' Disables the sleep alert interrupt
    // bit 6 '1' Disables the wake-up alert interrupt
    // bit 5 '1' Disables the half symbol timer interrupt
    // bit 4 '1' Disables the security key request interrupt
    // bit 3 '0' Enables the RX FIFO reception interrupt
    // bit 2 '1' Disables the TX GTS2 FIFO transmission interrupt
    // bit 1 '1' Disables the TX GTS1 FIFO transmission interrupt
    // bit 0 '0' Enables the TX Normal FIFO transmission interrupt    
    #if __cplusplus >= 202002L  // C++20 o superior
        constexpr SETINTCON intcon_config_20 = {
            .tx_normal_fifo    = 0,
            .tx_gts1_fifo      = 1,
            .tx_gts2_fifo      = 1,
            .rx_fifo           = 0,
            .sec_key_req       = 1,
            .half_symbol_timer = 1,
            .wake_up_alert     = 1,
            .sleep_alert       = 1
        };
    #elif __cplusplus >= 201703L  // C++17
        intcon_config = { 
            0,  // tx_normal_fifo (Bit 0: habilitado)
            1,  // tx_gts1_fifo (Bit 1: deshabilitado)
            1,  // tx_gts2_fifo (Bit 2: deshabilitado)
            0,  // rx_fifo (Bit 3: habilitado)
            1,  // sec_key_req (Bit 4: deshabilitado)
            1,  // half_symbol_timer (Bit 5: deshabilitado)
            1,  // wake_up_alert (Bit 6: deshabilitado)
            1   // sleep_alert (Bit 7: deshabilitado)
        };
    #else
        #error "Se requiere al menos C++17 para este código."
    #endif

    #if __cplusplus >= 202002L  // C++20 o superior
    const uint8_t intcon_value = set_intcon_value(intcon_config_20) ;       
    #elif __cplusplus >= 201703L  // C++17
    const uint8_t intcon_value = set_intcon_value(intcon_config);
    #endif

    write_short(MRF_INTCON, intcon_value);//write_short(MRF_INTCON, 0b11110110);        
    }





/*
    const uint32_t calculate_crc32(const std::string& data) {
        // CRC32 necesita un valor inicial (0xFFFFFFFF es el más común)
        uint32_t crc = crc32(0L, Z_NULL, 0);
        crc = crc32(crc, reinterpret_cast<const unsigned char*>(data.c_str()), data.size());
        return crc;
    }

    const uint32_t calculate_crc32(const uint8_t* data, size_t length) {
        uint32_t crc = crc32(0L, Z_NULL, 0);  // Inicialización de CRC32
        crc = crc32(crc, data, length);       // Calcular CRC para el buffer de bytes
        return crc;
    }
*/
}