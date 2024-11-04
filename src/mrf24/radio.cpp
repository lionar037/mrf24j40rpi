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

//#define MRF24_TRANSMITER_ENABLE

void Radio_t::Init(bool& flag) {
    // Actualización de estado a través del controlador Zigbee
    flag = zigbee->check_flags(&handle_rx, &handle_tx);
    const unsigned long current_time = 100000;  // Tiempo actual simulado para el ejemplo

    if (current_time - last_time > tx_interval) {
        last_time = current_time;

        #ifdef MRF24_TRANSMITER_ENABLE
            // Mensaje de transmisión configurado a 100 caracteres
            const std::string msj_to_zb = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz0123456ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuv";
            const std::string msj_to_zb_short = msj_to_zb.substr(0, MAX_PACKET_TX);

            // Cálculo de checksum
            uint8_t checksum = calculate_crc8(reinterpret_cast<const uint8_t*>(msj_to_zb_short.c_str()), msj_to_zb_short.size());

            // Creación del buffer de transmisión
            std::vector<uint8_t> buffer_zb(msj_to_zb_short.begin(), msj_to_zb_short.end());
            size_t total_size = buffer_zb.size() + sizeof(HEAD) + sizeof(checksum);

            // Configuración del paquete de transmisión
            struct DATA::packet_tx bufferTransReceiver{ HEAD, static_cast<uint16_t>(total_size), checksum ,{} };
            std::memcpy(bufferTransReceiver.data, buffer_zb.data(), std::min(buffer_zb.size(), sizeof(bufferTransReceiver.data)));

            // Información de depuración opcional
            #ifdef DBG_RADIO
                std::cout << "\nTotal packet size: " << bufferTransReceiver.size << ", Buffer size: " << buffer_zb.size() << "\n";
                std::cout << "Checksum (hex): " << hex_to_text(bufferTransReceiver.checksum) << "\n";
                std::cout << "Sending Buffer:\n";
            #endif

            // Convertir bufferTransReceiver en un vector para enviar
            std::vector<uint8_t> transmission_buffer(sizeof(bufferTransReceiver));
            std::memcpy(transmission_buffer.data(), &bufferTransReceiver, transmission_buffer.size());

            // Mostrar el contenido del buffer en hexadecimal
            for (const auto& byte : transmission_buffer) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << ' ';
            }
            std::cout << std::endl;

            // Envío del paquete según la configuración de dirección
            uint64_t mac_address;
            zigbee->mrf24j40_get_extended_mac_addr(&mac_address);
            std::cout << "Local MAC address: "; 
            print_to_hex(mac_address);

            #ifdef MACADDR64
                zigbee->send(ADDRESS_LONG_SLAVE, transmission_buffer);
            #elif defined(MACADDR16)
                zigbee->send(ADDR_SLAVE, transmission_buffer);
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



