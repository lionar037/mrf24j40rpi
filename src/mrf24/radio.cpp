
#include <mrf24/radio.hpp>
#include <mrf24/mrf24j40.hpp>
#include <mrf24/mrf24j40_template.tpp>
#include <qr/qr.hpp>
#include <file/file.hpp>
#include <display/color.hpp>
#include <oled/oled.hpp>
#include <work/rfflush.hpp>

//#include <memory>
#include <string_view>



namespace MRF24J40{ 

    std::unique_ptr<Mrf24j> zigbee ;
    //struct DATA::packet_rx  buffer_receiver{};
    DATA::PACKET_RX buffer_receiver{};

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
    //mrf24j40_spi->set_promiscuous(true);
    zigbee->settings_mrf();
    
        // uncomment if you want to enable PA/LNA external control
    zigbee->set_palna(true);    // Enable PA/LNA on MRF24J40MB module.
    //zigbee->set_palna(false);     //disable PA/LNA on MRF24J40MB module.
    
        // uncomment if you want to buffer all PHY Payload
    zigbee->set_bufferPHY(true);

        //attachInterrupt(0, interrupt_routine, CHANGE); // interrupt 0 equivalent to pin 2(INT0) on ATmega8/168/328
        //last_time = millis();

        //Single send cmd
        //mrf24j40_spi->Transfer3bytes(0xE0C1);
        
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

    void Radio_t::Init(bool& flag) {
        flag = zigbee->check_flags(&handle_rx, &handle_tx);
        const unsigned long current_time = 1;//1000000 original
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

            buffer_transmiter.head=HEAD; 
            std::string buff = MSJ;
            //buffer_transmiter.size=(~buff.size())&0xffff ;
            buffer_transmiter.size = static_cast<uint16_t>(buff.size()) + sizeof(buffer_transmiter.head) + sizeof(buffer_transmiter.checksum) ;
            std::cout<<"\n strlen(MSJ) + strlen(head) + strlen(checksum) : ( "<< std::to_string(buffer_transmiter.size) << " ) , budeffer size : ( " << std::to_string(buff.size())  <<" )\n";    
                        
            std::memcpy(buffer_transmiter.data ,buff.c_str(),buff.size());
            buffer_transmiter.checksum=0xffff;

            std::vector<uint8_t> vect(sizeof(buffer_transmiter));
            std::memcpy(vect.data(), &buffer_transmiter, sizeof(buffer_transmiter)); // Copiar los datos de la estructura al vector


            const char* msj = reinterpret_cast<const char* >(&buffer_transmiter);

            //  const auto* buff {reinterpret_cast<const char *>(mrf24j40_spi.get_rxinfo()->rx_data)};
            std::cout<<"\nTX Vect : size ( "<<  std::to_string(vect.size()) <<" , "<<sizeof(msj) << " )\n" ;
            std::cout<<"\n" ;
        
        const std::string pf(msj);
            
        for(const auto& byte : pf) std::cout << byte ; 
            std::cout<<"\n" ;         
            
        uint64_t mac_address;
        zigbee->mrf24j40_get_extended_mac_addr(&mac_address);
        std::cout<<"get address mac: " ;  print_to_hex(mac_address);
            
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

    void handle_tx() {
        #ifdef MRF24_TRANSMITER_ENABLE
        const auto status = zigbee->get_txinfo()->tx_ok;
            if (status) {
                std::cout<<"TX went ok, got ack \n";
            } else {
                std::cout<<"\nTX failed after \n";
                std::cout<<zigbee->get_txinfo()->retries;
                std::cout<<" retries\n";
            }
        #endif     
        }

    //@brief 
    //@params
    //@params

    
    
    template<typename T>
    void print_to_hex(const T int_to_hex) {
    // El tamaño en bytes del tipo de dato se multiplica por 2 para obtener el número de dígitos en hexadecimal.
    std::cout << std::hex 
              << std::setw(sizeof(T) * 2)  // Ancho basado en el tamaño del tipo de dato.
              << std::setfill('0')         // Rellena con ceros si es necesario.
              << +int_to_hex               // El símbolo '+' asegura que el tipo char se trate como número.
              << "\n";
}

    template<typename T>
    std::string hex_to_text(const T int_to_hex) {
        // Crear un flujo de salida para construir la cadena hexadecimal.
        std::ostringstream oss;

        // El tamaño en bytes del tipo de dato se multiplica por 2 para obtener el número de dígitos en hexadecimal.
        oss << std::hex 
            << std::setw(sizeof(T) * 2)   // Ancho basado en el tamaño del tipo de dato.
            << std::setfill('0')          // Rellena con ceros si es necesario.
            << +int_to_hex;               // El símbolo '+' asegura que el tipo char se trate como número.

        // Devolver la cadena construida.
        return oss.str();
    }
    void 
    handle_rx() {
        
        #ifdef MRF24_RECEIVER_ENABLE                
        auto  monitor{std::make_unique <FFLUSH::Fflush_t>()};

        std::ostringstream oss_zigbee{};        

        //detecto una interrupcion
        monitor->insert("received a packet ... ");

        // get_rxinfo()->frame_length devuelve un uint8_t
        const uint8_t frame_length = zigbee->get_rxinfo()->frame_length;

        // Usar std::ostringstream para construir el string en formato hexadecimal
        oss_zigbee << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(frame_length);

        // Mostrar el string resultante        
        monitor->insert(oss_zigbee.str() );
        oss_zigbee.str("");
        oss_zigbee.clear(); 

        if(zigbee->get_bufferPHY()){
            monitor->insert(" Packet data (PHY Payload) :");
            #ifdef DBG_PRINT_GET_INFO               

            for (std::size_t i = 0; i < std::size_t(zigbee->get_rxinfo()->frame_length); i++) {
                //if (i<=21){
                //    oss_zigbee << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(zigbee->get_rxbuf()[i]) << ":";
                //}
                //else
                {
                    oss_zigbee <<std::hex<< zigbee->get_rxbuf()[i];
                }
            }
            monitor->insert(oss_zigbee.str());
            oss_zigbee.str("");   // Limpiar el contenido
            oss_zigbee.clear();   // Restablecer el estado

            #endif
        }            
            SET_COLOR(SET_COLOR_CYAN_TEXT);
            monitor->insert("ASCII data (relevant data) :");
            monitor->insert("data_length : " + std::to_string(zigbee->rx_datalength()) );        

        for (auto& byte : zigbee->get_rxinfo()->rx_data)        
            {
                if(byte!=0x00)oss_zigbee << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << ":";
            }
            monitor->insert("info_zigbee : " );
            monitor->insert( oss_zigbee.str());        
            oss_zigbee.str("");   // Limpiar el contenido
            oss_zigbee.clear();   // Restablecer el estado

        #ifdef DBG_PRINT_GET_INFO                     
          std::memcpy (  &buffer_receiver , zigbee->get_rxbuf() , sizeof(DATA::packet_rx));
        const uint64_t mac_address_rx = (static_cast<uint64_t>(buffer_receiver.mac_msb_rx) << 32) | buffer_receiver.mac_lsb_rx;
        const uint64_t mac_address_tx = (static_cast<uint64_t>(buffer_receiver.mac_msb) << 32) | buffer_receiver.mac_lsb;
            monitor->insert (" " );
            monitor->insert (" " );
            //compara la direccion de mac "slave" con la mac de "entrada"
        if(ADDRESS_LONG_SLAVE == mac_address_rx){
            monitor->insert ("mac aceptada" ); }
        else { //muestra una direcion mac diferente a la configurada
            monitor->insert ("mac no es aceptada" );}
            monitor->insert( "rx data_receiver->mac : "         + hex_to_text( mac_address_rx )); 
            monitor->insert( "tx data_receiver->mac : "         + hex_to_text( mac_address_tx )); 
            monitor->insert( "buffer_receiver->head : "         + hex_to_text( buffer_receiver.head ));
            //auto bs = (!buffer_receiver.size)&0xffff;
            monitor->insert( "buffer_receiver->size : "         + std::to_string( buffer_receiver.size )); 
            monitor->insert( "buffer_receiver->panid : "        + hex_to_text( buffer_receiver.panid ));
            monitor->insert( "buffer_receiver->checksum : "     + hex_to_text( buffer_receiver.checksum ));
            monitor->insert( "buffer_receiver->head : "         + std::to_string( buffer_receiver.head ));

            std::string txt_tmp ;
            txt_tmp.assign(reinterpret_cast<const char*>(buffer_receiver.data), sizeof(buffer_receiver.data));
            monitor->insert( "data_receiver->data : "         + txt_tmp );

            //obtiene la direccion de mac seteada en el mrf24j40
            uint64_t mac_address;
            zigbee->mrf24j40_get_extended_mac_addr(&mac_address);
            monitor->insert("get address mac: "               + hex_to_text(mac_address));
            
        #endif
        
            RST_COLOR() ; 
            SET_COLOR(SET_COLOR_CYAN_TEXT);
            monitor->insert("LQI : " + std::to_string (zigbee->get_rxinfo()->lqi) );
            monitor->insert("RSSI : " + std::to_string(zigbee->get_rxinfo()->rssi) );
        
        
        //imprime todo los datos obtenidos
        monitor->print_all();
        #endif
        RST_COLOR() ;   
        //SET_COLOR(SET_COLOR_RED_TEXT);
        
        update(reinterpret_cast<const char*>(zigbee->get_rxinfo()->rx_data));
    
    }

        Radio_t::~Radio_t() {
            #ifdef DBG_RADIO
                std::cout<<"~Radio_t()\n";
            #endif
        }
}



