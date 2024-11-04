//radio.cpp
#include <mrf24/radio.hpp>
#include <mrf24/mrf24j40.hpp>
#include <mrf24/mrf24j40_template.tpp>
//#include <qr/qr.hpp>
#include <file/file.hpp>
#include <display/color.hpp>
#include <work/rfflush.hpp>
#ifdef USE_OLED
    #include <oled/oled.hpp>
#endif
#include <string_view>
#include <zlib.h>  // Para usar crc32
#include <string>
#include <cstdint>
#include <cstddef>



namespace MRF24J40{ 
extern std::unique_ptr<Mrf24j> zigbee ;
extern DATA::PACKET_RX buffer_receiver;

    Radio_t::Radio_t() 
    #ifdef ENABLE_INTERRUPT_MRF24
    :   status          (true)
        #ifdef USE_FS
        ,   fs              { std::make_unique<FILESYSTEM::File_t>() }
        #endif
        #ifdef ENABLE_DATABASE
    ,   database        { std::make_unique<DATABASE::Database_t>() }
        #endif
        #else
    :   status          (false)
        #ifdef USE_QR 
    ,   qr              { std::make_unique<QR::Qr_t>() }
        #endif
    #endif
    ,   gpio            { std::make_unique<GPIO::Gpio_t>(status) }
    {
        
        #ifdef ENABLE_INTERRUPT_MRF24
        
        #else
            #ifdef USR_QR
                qr->create(QR_CODE_SRT);
            #endif
        #endif
            
        #ifdef DBG_RADIO
        std::cout << "Size msj : ( "<<std::dec<<sizeof(MSJ)<<" )\n";
        #endif
        zigbee = std::make_unique<Mrf24j>();        
        //zigbee->mrf24j40_init(); 
        zigbee->init();
        zigbee->interrupt_handler();
        zigbee->set_pan(PAN_ID);
        // This is _our_ address

        #ifdef MACADDR16
            zigbee->address16_write(ADDRESS); 
        #elif defined (MACADDR64)
            zigbee->address64_write(ADDRESS_LONG);
        #endif

        // uncomment if you want to receive any packet on this channel
        //zigbee->set_promiscuous(true);
        zigbee->settings_mrf();
    
        // uncomment if you want to enable PA/LNA external control
        zigbee->set_palna(true);    // Enable PA/LNA on MRF24J40MB module.
    
        // uncomment if you want to buffer all PHY Payload
        zigbee->set_bufferPHY(true);

        //attachInterrupt(0, interrupt_routine, CHANGE); // interrupt 0 equivalent to pin 2(INT0) on ATmega8/168/328
        flag=true;                        
    }

    void Radio_t::RunProccess(void){
        system("clear"); 
        #ifdef MRF24_RECEIVER_ENABLE
            while(true)
        #endif
        {           
            gpio->app(flag);
            zigbee->interrupt_handler();
            Init(flag);        
        }
    }

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


    const uint8_t calculate_crc8(const uint8_t* data, size_t length) {
        uint8_t crc = 0x00;  // Inicialización del CRC

        for (size_t i = 0; i < length; ++i) {
            crc ^= data[i];  // XOR el byte actual con el CRC

            for (uint8_t j = 0; j < 8; ++j) {  // Procesar cada bit del byte
                if (crc & 0x80) {  // Si el bit más alto está encendido
                    crc = (crc << 1) ^ 0x07;  // Desplazar y aplicar polinomio (0x07 es común para CRC-8)
                } else {
                    crc <<= 1;  // Solo desplazar si no es necesario aplicar el polinomio
                }
            }
        }
        return crc;  // Retornar el CRC de 8 bits
    }

#define MRF24_TRANSMITER_ENABLE

    void Radio_t::Init(bool& flag) {
        flag = zigbee->check_flags(&handle_rx, &handle_tx);
        const unsigned long current_time = 100000;//1000000 original
        if (current_time - last_time > tx_interval) {
            last_time = current_time;
        #ifdef MRF24_TRANSMITER_ENABLE
            #ifdef DBG_RADIO
                #ifdef MACADDR64
                    std::cout<<"send msj 64() ... \n";
                #else
                    std::cout<<"send msj 16() ... \n";
                #endif
            #endif
        
        //typedef struct packet_tx{
        //        uint8_t head;            
        //        uint16_t size;
        //        uint8_t data[107];            
        //        uint8_t checksum;
        //        uint8_t end;
        //}PACKET_TX;            

        auto checksum = calculate_crc8 ( reinterpret_cast<const uint8_t *>(MSJ ) , sizeof(MSJ)); 
        struct DATA::packet_tx bufferTransReceiver{HEAD,sizeof(MSJ)+sizeof(HEAD)+sizeof(checksum),MSJ,127,0xff};
        
        std::cout<<"\n strlen(MSJ) + strlen(head) + strlen(checksum) = total : ( "<< std::to_string(bufferTransReceiver.size) << " ) , budeffer size :  \n";                            
        std::cout<<"bufferTransReceiver.data size :  " << std::to_string(sizeof(MSJ))<<"\n";
        std::cout<<"hex checksum : " <<hex_to_text(bufferTransReceiver.checksum);

        std::cout<<"\nBuffer Send : \n";

        //imprime lo que tendria en la salida del dispositivo zigbee            
        
        std::vector<uint8_t> vect(sizeof(bufferTransReceiver));
        std::memcpy(vect.data(),&bufferTransReceiver,vect.size());

        for(const auto& byte : vect) std::cout << byte ; 
            std::cout<<"\n" ;         
            
        uint64_t mac_address;
        zigbee->mrf24j40_get_extended_mac_addr(&mac_address);
        std::cout<<"local address mac: " ;  print_to_hex(mac_address);
            
            #ifdef MACADDR64
            zigbee->send(ADDRESS_LONG_SLAVE,vect);
            //zigbee->send64(ADDRESS_LONG_SLAVE, buffer_transmiter);
            
            #elif defined(MACADDR16)
                zigbee->send(ADDR_SLAVE, vect);                                
            #endif
        #endif
        }
    }

    void Radio_t::interrupt_routine() {
        zigbee->interrupt_handler(); // mrf24 object interrupt routine
    }

    void update(std::string_view str_view){
        
        const int positionAdvance{15};
        auto            fs          { std::make_unique<FILESYSTEM::File_t> () };
        #ifdef USE_QR
        auto            qr_img      { std::make_unique<QR::Qr_img_t>() };
        //auto            qr_oled      { std::make_unique<QR::QrOled_t>() };
        #endif
        auto            monitor     { std::make_unique <FFLUSH::Fflush_t>()};
        #ifdef USE_OLED
            static auto     oled        { std::make_unique<OLED::Oled_t>() };    //inicializar una sola vez 
        #endif
        const auto*     packet_data = reinterpret_cast<const char*>(str_view.data());
        
        std::string  packetData (packet_data += positionAdvance);
        packetData.resize(38);

        SET_COLOR(SET_COLOR_GRAY_TEXT);
    
        #ifdef USE_OLED
            oled->create(packetData.c_str());  
        #endif
        #ifdef USE_QR
        auto qr = std::make_unique<QR::QrOled_t>();

        //De momento no hace nada
        std::string packet_data2 = "1234567890";    
        std::vector<int> infoQrTmp; 
        qr->create_qr(packet_data2, infoQrTmp);
        monitor->insert( std::to_string(infoQrTmp.size()));
        std::cout << " Size info of Qr Buffer : " << infoQrTmp.size() << std::endl;    
        #endif
                        
        fs->create(packet_data);
        std::cout<<"\n";
        #ifdef USE_QR
            qr_img->create(packet_data);
    #endif
    return ;    
    }


        Radio_t::~Radio_t() {
            #ifdef DBG_RADIO
                std::cout<<"~Radio_t()\n";
            #endif
        }
}



