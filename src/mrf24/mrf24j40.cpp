#include <mrf24/mrf24j40_cmd.hpp>
#include <mrf24/mrf24j40_settings.hpp>
#include <mrf24/mrf24j40.hpp>
#include <tyme/tyme.hpp>
#include <config/config.hpp>
#include <work/data_analisis.hpp>
#include <spi/spi.hpp>


namespace MRF24J40{
            // aMaxPHYPacketSize = 127, from the 802.15.4-2006 standard.
    static uint8_t rx_buf[127];

    static int ignoreBytes { 0 }; // bytes to ignore, some modules behaviour.
    static bool bufPHY { false }; // flag to buffer all bytes in PHY Payload, or not
    static rx_info_t rx_info{};
    static tx_info_t tx_info{};
    static RXMCR rxmcr{0x00};


    Mrf24j::Mrf24j()
    : prt_spi {std::make_unique<SPI::Spi_t>()} , m_bytes_nodata { m_bytes_MHR + m_bytes_FCS}
    {
        #ifdef DBG
            std::cout <<"Mrf24j( )\r\n";
        #endif
    }

    const uint8_t 
    Mrf24j::read_short(const uint8_t address) {
            // 0 top for short addressing, 0 bottom for read
        const uint8_t tmp = (address<<1 & 0b01111110);
        const uint8_t ret = prt_spi->Transfer2bytes(tmp); // envia 16 , los mas significativos en 0x00 , los menos significativos envia el comando
//printf("2:0x%x ",ret);
        return ret;
    }

    void 
    Mrf24j::write_short(const uint8_t address,const uint8_t data) {
            // 0 for top short address, 1 bottom for write
    const uint16_t lsb_tmp = ( (address<<1 & 0b01111110) | 0x01 ) | (data<<8);
        prt_spi->Transfer2bytes(lsb_tmp);
        return;
    }

    const uint8_t 
    Mrf24j::read_long(const uint16_t address) {

        const uint8_t lsb_address = (address >> 3 )& 0x7F;//0x7f
        const uint8_t msb_address = (address << 5) & 0xE0;//0xe0

        const uint32_t tmp = ( (0x80 | lsb_address) | (msb_address <<8) ) &  0x0000ffff;
	  //  const uint8_t ret = 
       return prt_spi->Transfer3bytes(tmp);
        
    //return ret;
    }

    void 
    Mrf24j::write_long(const uint16_t address,const uint8_t data) {
        const uint8_t lsb_address = (address >> 3) & 0x7F;
        const uint8_t msb_address = (address << 5) & 0xE0;
        const uint32_t comp = ( (0x80 | lsb_address) | ( (msb_address | 0x10) << 8 ) | (data<<16) ) & 0xffffff;
        prt_spi->Transfer3bytes(comp);
        return;
    }

    const uint16_t 
    Mrf24j::get_pan(void) {
        const uint8_t panh = read_short(MRF_PANIDH);
        return (panh << 8 | read_short(MRF_PANIDL));
    }

    void 
    Mrf24j::set_pan(const uint16_t panid) {
        write_short(MRF_PANIDH, (panid >> 8)& 0xff);
        write_short(MRF_PANIDL, panid & 0xff);
    }

    void 
    Mrf24j::address16_write(const uint16_t address16) {
        write_short(MRF_SADRH, (address16 >> 8)& 0xff);
        write_short(MRF_SADRL, address16 & 0xff);
    }

    void 
    Mrf24j::address64_write(const uint64_t addressLong){
        write_short(MRF_EADR7,(addressLong>>56)&0xff);
        write_short(MRF_EADR6,(addressLong>>48)&0xff);
        write_short(MRF_EADR5,(addressLong>>40)&0xff);
        write_short(MRF_EADR4,(addressLong>>32)&0xff);
        write_short(MRF_EADR3,(addressLong>>24)&0xff);
        write_short(MRF_EADR2,(addressLong>>16)&0xff);
        write_short(MRF_EADR1,(addressLong>>8 )&0xff);
        write_short(MRF_EADR0,(addressLong)&0xff);
    //return ;
    }

    const  uint16_t 
    Mrf24j::address16_read(void) {
        const uint8_t a16h = read_short(MRF_SADRH);
        return (a16h << 8 | read_short(MRF_SADRL));
    }

    //lee la direccion mac de 64 bits
    const uint64_t 
    Mrf24j::address64_read(void){
        uint64_t address64 ;
        address64  = (read_short(MRF_EADR0));
        address64 |= (read_short(MRF_EADR1))<< 8;
        address64 |= static_cast<uint64_t>(read_short(MRF_EADR2))<<16;
        address64 |= static_cast<uint64_t>(read_short(MRF_EADR3))<<24;
        address64 |= static_cast<uint64_t>(read_short(MRF_EADR4))<<32;
        address64 |= static_cast<uint64_t>(read_short(MRF_EADR5))<<40;
        address64 |= static_cast<uint64_t>(read_short(MRF_EADR6))<<48;
        address64 |= static_cast<uint64_t>(read_short(MRF_EADR7))<<56;
    return  address64;
    }

    /**
     * Simple send 16, with acks, not much of anything.. assumes src16 and local pan only.
        * @param data
    */

    void 
    Mrf24j::set_interrupts(void) {
            // interrupts for rx and tx normal complete
        write_short(MRF_INTCON, 0b11110110);
    }

            /** use the 802.15.4 channel numbers..
            */
    void 
    Mrf24j::set_channel(const uint8_t channel) {
        write_long(MRF_RFCON0, (((channel - 11) << 4) | 0x03));
    }

    void 
    Mrf24j::init(void) {
    // //Seems a bit ridiculous when I use reset pin anyway
    // write_short(MRF_SOFTRST, 0x7); // from manual
    // while (read_short(MRF_SOFTRST) & 0x7 != 0) {
        // ; // wait for soft reset to finish
    // }
           delay(192); 
           #include <config/config.hpp>
           #ifdef RESET_MRF_SOFTWARE
            write_short(MRF_SOFTRST, 0x7); 
            #else
            //write_short(MRF_SOFTRST, 0x7); 
           #endif
        write_short(MRF_PACON2, 0x98);  // – Initialize FIFOEN = 1 and TXONTS = 0x6.
        write_short(MRF_TXSTBL, 0x95);  // – Initialize RFSTBL = 0x9.

        write_long(MRF_RFCON0, 0x03);   // – Initialize RFOPT = 0x03.
        write_long(MRF_RFCON1, 0x01);   // – Initialize VCOOPT = 0x02.
        write_long(MRF_RFCON2, 0x80);   // – Enable PLL (PLLEN = 1).
        write_long(MRF_RFCON6, 0x90);   // – Initialize TXFIL = 1 and 20MRECVR = 1.
        write_long(MRF_RFCON7, 0x80);   // – Initialize SLPCLKSEL = 0x2 (100 kHz Internal oscillator).
        write_long(MRF_RFCON8, 0x10);   // – Initialize RFVCO = 1.
        write_long(MRF_SLPCON1,0x21);  // – Initialize CLKOUTEN = 1 and SLPCLKDIV = 0x01.

        //  Configuration for nonbeacon-enabled devices (see Section 3.8 “Beacon-Enabled and
        //  Nonbeacon-Enabled Networks”):
        write_short(MRF_BBREG2, 0x80);      // Set CCA mode to ED
        write_short(MRF_CCAEDTH, 0x60);     // – Set CCA ED threshold.
        write_short(MRF_BBREG6, 0x40);      // – Set appended RSSI value to RXFIFO.
        set_interrupts();
        set_channel(CHANNEL);                    //original es 12
        // max power is by default.. just leave it...
        // Set transmitter power - See “REGISTER 2-62: RF CONTROL 3 REGISTER (ADDRESS: 0x203)”.
        write_short(MRF_RFCTL, 0x04);       //  – Reset RF state machine.
        write_short(MRF_RFCTL, 0x00);       // part 2
        delay(192);                           // delay at least 192usec
    }

    /**
     * Call this from within an interrupt handler connected to the MRFs output
     * interrupt pin.  It handles reading in any data from the module, and letting it
     * continue working.
     * Only the most recent data is ever kept.
     */
            
    void 
    Mrf24j::interrupt_handler(void) {
        const uint8_t last_interrupt = read_short(MRF_INTSTAT);
        if(last_interrupt & MRF_I_RXIF) {
            m_flag_got_rx++;
                // read out the packet data...
            noInterrupts();
            rx_disable();
                // read start of rxfifo for, has 2 bytes more added by FCS. frame_length = m + n + 2
            const uint8_t frame_length = read_long(0x300);

                // buffer all bytes in PHY Payload
            if(bufPHY){
                int rb_ptr = 0;
                for (int i = 0; i < frame_length; ++i) { // from 0x301 to (0x301 + frame_length -1)
                    rx_buf[++rb_ptr] = read_long(0x301 + i);
                }
            }

            // buffer data bytes
            int rd_ptr = 0;
            // from (0x301 + bytes_MHR) to (0x301 + frame_length - bytes_nodata - 1)
            // printf(" frame length : %d \n",frame_length);
            // printf(" rx datalength : %d \n",rx_datalength());

            for(uint16_t i = 0; i < frame_length ; ++i) {//original
            //for (uint16_t i = 0; i < frame_length + rx_datalength(); i++) {
                rx_info.rx_data[++rd_ptr] = read_long(0x301 + m_bytes_MHR + i);
            }

            rx_info.frame_length = frame_length;
                    // same as datasheet 0x301 + (m + n + 2) <-- frame_length
            rx_info.lqi = read_long(0x301 + frame_length);
                    // same as datasheet 0x301 + (m + n + 3) <-- frame_length + 1
            rx_info.rssi = read_long(0x301 + frame_length + 1);

            rx_enable();
            interrupts();
        }
        if (last_interrupt & MRF_I_TXNIF) {
            m_flag_got_tx++;
            const uint8_t tmp = read_short(MRF_TXSTAT);
                // 1 means it failed, we want 1 to mean it worked.
            tx_info.tx_ok = !(tmp & ~(1 << TXNSTAT));
            tx_info.retries = tmp >> 6;
            tx_info.channel_busy = (tmp & (1 << CCAFAIL));
        }
    }

    /**
     * Call this function periodically, it will invoke your nominated handlers
     */
    bool 
    Mrf24j::check_flags(void (*rx_handler)(), void (*tx_handler)()){
            // TODO - we could check whether the flags are > 1 here, indicating data was lost?
        if (m_flag_got_rx) {
            m_flag_got_rx = 0;
            #ifdef DBG
                std::cout<< "recibe algo \n";
            #endif
            rx_handler();
            return true;
        }
        if (m_flag_got_tx) {
            m_flag_got_tx = 0;
            #ifdef DBG_MRF
                std::cout<< "transmite algo \n";
            #endif
            tx_handler();
            return false;
        }
        return false;
    }

    /**
     * Set RX mode to promiscuous, or normal
     */
    void 
    Mrf24j::set_promiscuous(bool enabled) {
        if (enabled) {
            write_short(MRF_RXMCR, 0x01);
        } else {
            write_short(MRF_RXMCR, 0x00);
        }
    }


    void 
    Mrf24j::settings_mrf(void){
        rxmcr.PANCOORD=true;
        rxmcr.COORD=false;
        rxmcr.PROMI=true;
        #ifdef DBG_MRF
            printf("*reinterpret_cast : 0x%x\n",*reinterpret_cast<uint8_t*>(&rxmcr));
        #endif
        write_short(MRF_RXMCR, *reinterpret_cast<uint8_t*>(&rxmcr));
        return;
    }

    rx_info_t* 
    Mrf24j::get_rxinfo(void) {
        return &rx_info;
    }

    tx_info_t* 
    Mrf24j::get_txinfo(void) {
        return &tx_info;
    }

    uint8_t* 
    Mrf24j::get_rxbuf(void) {
        return rx_buf;
    }

    const int 
    Mrf24j::rx_datalength(void) {
        return rx_info.frame_length - m_bytes_nodata;
    }

    void 
    Mrf24j::set_ignoreBytes(int ib) {
        // some modules behaviour
        ignoreBytes = ib;
    }

    /**
     * Set bufPHY flag to buffer all bytes in PHY Payload, or not
     */
    void 
    Mrf24j::set_bufferPHY(bool bp) {
        bufPHY = bp;
    }

    bool 
    Mrf24j::get_bufferPHY(void) {
        return bufPHY;
    }

    /**
     * Set PA/LNA external control
     */
    void 
    Mrf24j::set_palna(const bool enabled) {
        if (enabled) {
            write_long(MRF_TESTMODE, 0x07); // Enable PA/LNA on MRF24J40MB module.
        }else{
            //write_long(MRF_TESTMODE, 0x00); // Disable PA/LNA on MRF24J40MB module.//original
            write_long(MRF_TESTMODE, 0x08); // Disable PA/LNA on MRF24J40MB module.
        }
    }

    void 
    Mrf24j::rx_flush(void) {
        write_short(MRF_RXFLUSH, 0x01);
    }

    void 
    Mrf24j::rx_disable(void) {
        write_short(MRF_BBREG1, 0x04);  // RXDECINV - disable receiver
    }

    void 
    Mrf24j::rx_enable(void) {
        write_short(MRF_BBREG1, 0x00);  // RXDECINV - enable receiver
    }

    void 
    Mrf24j::pinMode(const int i,const bool b){
    //return;
    }

    void 
    Mrf24j::digitalWrite(const int i,const bool b){
    //return;
    }

    void 
    Mrf24j::delay(const uint16_t t){
        TYME::Time_t time ;
        time.delay_ms(t);
    //return;
    }

    void 
    Mrf24j::interrupts(){
        set_interrupts();//verificar 
    }

    void 
    Mrf24j::noInterrupts(){
    }

    Mrf24j::~Mrf24j( ){
        #ifdef DBG_MRF
            std::cout <<"~Mrf24j( )\r\n";
        #endif
    }



    void 
    Mrf24j::send(const uint64_t dest, const std::vector<uint8_t> pf) 
    {
        const auto len = pf.size();
        int i = 0;
        write_long(i++, m_bytes_MHR); // header length
                        // +ignoreBytes is because some module seems to ignore 2 bytes after the header?!.
                        // default: ignoreBytes = 0;
        write_long(i++, m_bytes_MHR+ignoreBytes+len);

                        // 0 | pan compression | ack | no security | no data pending | data frame[3 bits]
        write_long(i++, 0b01100001); // first byte of Frame Control
                        // 16 bit source, 802.15.4 (2003), 16 bit dest,
        write_long(i++, 0b10001000); // second byte of frame control
        write_long(i++, 1);  // sequence number 1

        const uint16_t panid = get_pan();
        #ifdef DBG
            printf("\npanid : 0x%X\n",panid);
        #endif

        write_long(i++, panid & 0xff);  // dest panid
        write_long(i++, panid >> 8);

        write_long(i++, dest & 0xff);  // dest16 low
        write_long(i++, dest >> 8); // dest16 high

        if(sizeof(dest)>2){
            #ifdef DBG_MRF
                std::cout <<"es un mac de 64 bytes\n";
            #endif
        write_long(i++, (dest >> 16 ) & 0xff);
        write_long(i++, (dest >> 24 ) & 0xff);
        write_long(i++, (dest >> 32 ) & 0xff);
        write_long(i++, (dest >> 40 ) & 0xff);
        write_long(i++, (dest >> 48 ) & 0xff);
        write_long(i++, (dest >> 56 ) & 0xff);
        }
        else{
            #ifdef DBG_MRF
            std::cout <<"es un mac de 16 bytes\n";
            #endif
        }
 
        const uint64_t src = address64_read();
        write_long(i++, src & 0xff); // src16 low
        write_long(i++, src >> 8); // src16 high

    // si lee una direccion mac de 64 bits la envia
       if(sizeof(src)>2){
            write_long(i++, (src >> 16 ) & 0xff); 
            write_long(i++, (src >> 24 ) & 0xff); 
            write_long(i++, (src >> 32 ) & 0xff); 
            write_long(i++, (src >> 40 ) & 0xff); 
            write_long(i++, (src >> 48 ) & 0xff); 
            write_long(i++, (src >> 56 ) & 0xff); 
        }
                // All testing seems to indicate that the next two bytes are ignored.
                //2 bytes on FCS appended by TXMAC
         i+=ignoreBytes;

        for(const auto& byte : pf) write_long(i++,static_cast<uint8_t>(byte));
        
        // ack on, and go!
        write_short(MRF_TXNCON, (1<<MRF_TXNACKREQ | 1<<MRF_TXNTRIG));

        mode_turbo();
    }

 
    void 
    Mrf24j::send64(uint64_t dest64, const struct DATA::packet_tx packet_tx) {
        const uint8_t len = strlen(packet_tx.data); // get the length of the char* array
        //const uint8_t len = strlen(packet_tx); // get the length of the char* array
        int i = 0;
        write_long(i++, m_bytes_MHR); // header length
                        // +ignoreBytes is because some module seems to ignore 2 bytes after the header?!.
                        // default: ignoreBytes = 0;
        write_long(i++, m_bytes_MHR+ignoreBytes+len);//9 + 2 + tamaño del paquete

                        // 0 | pan compression | ack | no security | no data pending | data frame[3 bits]
        write_long(i++, 0b01100001); // first byte of Frame Control
                        // 16 bit source, 802.15.4 (2003), 16 bit dest,
        write_long(i++, 0b10001000); // second byte of frame control
        write_long(i++, 1);  // sequence number 1

        const uint16_t panid = get_pan();
        #ifdef DBG
            printf("\npanid: 0x%X\n",panid);
        #endif
        write_long(i++, panid >> 8);
        write_long(i++, panid & 0xff);  // dest panid
        
        //direccion de destino a enviar el mensaje
        set_macaddress64(i, dest64 );
        //write_long(i++, (dest64 >> 56 ) & 0xff);
        //write_long(i++, (dest64 >> 48 ) & 0xff);
        //write_long(i++, (dest64 >> 40 ) & 0xff);
        //write_long(i++, (dest64 >> 32 ) & 0xff);
        //write_long(i++, (dest64 >> 24 ) & 0xff);
        //write_long(i++, (dest64 >> 16 ) & 0xff);
        //write_long(i++, (dest64 >> 8  ) & 0xff);
        //write_long(i++, dest64  & 0xff); // uint64_t
       
        //lee la direccion mac de 64 bits obtenida
        //const uint64_t origin_64 = address64_read();

        set_macaddress64(i, address64_read() );
        //write_long(i++, origin_64  & 0xff ); // uint64_t
        //write_long(i++, (origin_64 >> 56 ) & 0xff); 
        //write_long(i++, (origin_64 >> 48 ) & 0xff); 
        //write_long(i++, (origin_64 >> 40 ) & 0xff); 
        //write_long(i++, (origin_64 >> 32 ) & 0xff); 
        //write_long(i++, (origin_64 >> 24 ) & 0xff); 
        //write_long(i++, (origin_64 >> 16 ) & 0xff); 
        //write_long(i++, (origin_64 >> 8  ) & 0xff); 

#include <mrf24/mrf24j40._microchip.hpp>
        write_long(RFCTRL2,0x80);

        // All testing seems to indicate that the next two bytes are ignored.
        //2 bytes on FCS appended by TXMAC
        i+=ignoreBytes;

                //for(const auto& byte : static_cast<const char *>(buf.head) )
                // for(const auto& byte : static_cast<const char *>(buf.size) )
        //write_long(i++,packet_tx.head);        
                //write_long(i++,buf.head&0xff);
                //write_long(i++,(buf.head>>8)&0xff);

        std::vector<uint8_t> vect(sizeof(packet_tx));
        std::memcpy(vect.data(), &packet_tx, sizeof(packet_tx)); // Copiar los datos de la estructura al vector

        for(const auto& byte : vect)write_long(i++,byte);
        //write_long(i++,packet_tx.checksum>>8);
        //write_long(i++,packet_tx.checksum&0xff);
        // ack on, and go!
        write_short(MRF_TXNCON, (1<<MRF_TXNACKREQ | 1<<MRF_TXNTRIG));        
        mode_turbo();
    }
   
    void 
    Mrf24j::mode_turbo(){
        // Define TURBO_MODE if more bandwidth is required
        // to enable radio to operate to TX/RX maximum 
        // 625Kbps
        #ifdef TURBO_MODE        
            write_short(WRITE_BBREG0, 0x01);
            write_short(WRITE_BBREG3, 0x38);
            write_short(WRITE_BBREG4, 0x5C);            
            write_short(WRITE_RFCTL,0x04);
            write_short(WRITE_RFCTL,0x00);    
        #endif          
    }

    void 
    Mrf24j::set_macaddress64( int& i , const uint64_t origin_64 ){                
            
            write_long(i++, (origin_64 >> 56 ) & 0xff); 
            write_long(i++, (origin_64 >> 48 ) & 0xff); 
            write_long(i++, (origin_64 >> 40 ) & 0xff); 
            write_long(i++, (origin_64 >> 32 ) & 0xff); 
            write_long(i++, (origin_64 >> 24 ) & 0xff); 
            write_long(i++, (origin_64 >> 16 ) & 0xff); 
            write_long(i++, (origin_64 >> 8  ) & 0xff); 
            write_long(i++, origin_64  & 0xff ); // uint64_t
    }

    void
    Mrf24j::flush_rx_fifo(void)
    {
      write_short(MRF_RXFLUSH, read_short(MRF_RXFLUSH) | 0b00000001);
    }

    //static void
    void
    Mrf24j::reset_rf_state_machine(void)
    {
      /*
       * Reset RF state machine
       */

      const uint8_t rfctl = read_short(MRF_RFCTL);

      write_short(MRF_RFCTL, rfctl | 0b00000100);
      write_short(MRF_RFCTL, rfctl & 0b11111011);
    
      //TYME::delay_us(2500);
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
    
    //
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
    SETINTCON intcon_config = {
        .tx_normal_fifo    = 0, // Bit 0: habilitado
        .tx_gts1_fifo      = 1, // Bit 1: deshabilitado
        .tx_gts2_fifo      = 1, // Bit 2: deshabilitado
        .rx_fifo           = 0, // Bit 3: habilitado
        .sec_key_req       = 1, // Bit 4: deshabilitado
        .half_symbol_timer = 1, // Bit 5: deshabilitado
        .wake_up_alert     = 1, // Bit 6: deshabilitado
        .sleep_alert       = 1  // Bit 7: deshabilitado
    };
#elif __cplusplus >= 201703L  // C++17
    SETINTCON intcon_config = { 
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


    // Obtener el valor binario a escribir
    uint8_t intcon_value = set_intcon_value(intcon_config);

    // Escribir el valor (por ejemplo, al registro MRF_INTCON)
    write_short(MRF_INTCON, intcon_value);//write_short(MRF_INTCON, 0b11110110);
    
    
    }
    


    void 
    Mrf24j::mrf24j40_get_extended_mac_addr(uint64_t *address)
    {
        uint8_t* addr_ptr = reinterpret_cast<uint8_t*>(address);
        addr_ptr[0] = read_short(MRF_EADR7);
        addr_ptr[1] = read_short(MRF_EADR6);
        addr_ptr[2] = read_short(MRF_EADR5);
        addr_ptr[3] = read_short(MRF_EADR4);
        addr_ptr[4] = read_short(MRF_EADR3);
        addr_ptr[5] = read_short(MRF_EADR2);
        addr_ptr[6] = read_short(MRF_EADR1);
        addr_ptr[7] = read_short(MRF_EADR0);
    }

}//END NAMESPACE MRF24
