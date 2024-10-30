
#include <mrf24/radio.hpp>
#include <mrf24/mrf24j40.hpp>
#include <mrf24/mrf24j40_template.tpp>
#include <qr/qr.hpp>
#include <file/file.hpp>
#include <display/color.hpp>
#include <oled/oled.hpp>
#include <work/rfflush.hpp>
#include <sstream>  // Para std::ostringstream
#include <iomanip>  // Para std::hex y std::setfill

//#include <app/src/data_analisis.h>
#include <memory>
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
    // Run();

    }

    void Radio_t::RunProccess(void){
        system("clear"); 
        #ifdef MRF24_RECEIVER_ENABLE
            while(true)
        #endif
        {   
            //std::cout << "\033[2J\033[H" << std::flush;
            gpio->app(flag);
            //system("clear"); 

            zigbee->interrupt_handler();
            Init(flag);        
        }
    }

    void Radio_t::Init(bool& flag) {
        flag = zigbee->check_flags(&handle_rx, &handle_tx);
        const unsigned long current_time = 10000;//1000000 original
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
            buffer_transmiter.size=(~buff.size())&0xffff ;
            std::cout<<"\n strlen(MSJ) : "<< std::to_string(buff.size())  <<"\n";    
                        
            std::memcpy(buffer_transmiter.data ,buff.c_str(),buff.size());

            std::vector<uint8_t> vect(sizeof(buffer_transmiter));
            std::memcpy(vect.data(), &buffer_transmiter, sizeof(buffer_transmiter)); // Copiar los datos de la estructura al vector


            const char* msj = reinterpret_cast<const char* >(&buffer_transmiter);

            //  const auto* buff {reinterpret_cast<const char *>(mrf24j40_spi.get_rxinfo()->rx_data)};
            std::cout<<"\nTX MSJ : size ( "<<  strlen(msj) <<" , "<<sizeof(msj) << " )\n" ;
            std::cout<<"\n" ;
        
        const std::string pf(msj);
            
        for(const auto& byte : pf) std::cout << byte ; 
            std::cout<<"\n" ;         
            
            
            #ifdef MACADDR64
            zigbee->send(ADDRESS_LONG_SLAVE,vect);
            //zigbee->send64(ADDRESS_LONG_SLAVE, buffer_transmiter);
            
            #elif defined(MACADDR16)
                zigbee->send(ADDR_SLAVE, msj);
                //zigbee->send(ADDR_SLAVE, pf );
                //zigbee->send16(ADDR_SLAVE, MSJ );//send data//original//mrf24j40_spi.send16(0x4202, "abcd")
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
        
        std::string  PacketDataTmp (packet_data += positionAdvance);
        PacketDataTmp.resize(38);

        SET_COLOR(SET_COLOR_GRAY_TEXT);
    
        #ifdef USE_OLED
            oled->create(PacketDataTmp.c_str());  
        #endif
        #ifdef USE_QR
        auto qr = std::make_unique<QR::QrOled_t>();

        //De momento no hace nada
        std::string packet_data2 = "1234567890";    
        std::vector<int> infoQrTmp; 
        qr->create_qr(packet_data2, infoQrTmp);
        monitor->print( std::to_string(infoQrTmp.size()),N_FILE_INIT+10,17);
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

    void handle_rx() {
        #ifdef MRF24_RECEIVER_ENABLE
        int files {POSITIOM_INIT_PRINTS};
        int col {0};
        auto  monitor{std::make_unique <FFLUSH::Fflush_t>()};

        files=POSITIOM_INIT_PRINTS;

        monitor->print("received a packet ... ",files++,col);

        // Suponiendo que get_rxinfo()->frame_length devuelve un uint8_t
        const uint8_t frame_length = zigbee->get_rxinfo()->frame_length;

        // Usar std::ostringstream para construir el string en formato hexadecimal
        std::ostringstream oss;
        oss << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(frame_length);

        // Obtener el string resultante
        std::string bufferMonitor = oss.str() + "\n";
        
        monitor->print(bufferMonitor,files++,col);
        
        if(zigbee->get_bufferPHY()){
            monitor->print(" Packet data (PHY Payload) :",files++,col);
            #ifdef DBG_PRINT_GET_INFO
            for (std::size_t i = 0; i < std::size_t(zigbee->get_rxinfo()->frame_length); i++) {
                if (i<21){
                    //std::cout << std::to_string(zigbee->get_rxbuf()[i])<<":";
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(zigbee->get_rxbuf()[i]) << ":";
                }
                else{
                    std::cout <<std::hex<< zigbee->get_rxbuf()[i];
                }
            }
            #endif
        }
            std::cout << "\n";
            SET_COLOR(SET_COLOR_CYAN_TEXT);
            monitor->print("ASCII data (relevant data) :",files++,col);

            const auto recevive_data_length = zigbee->rx_datalength();
            monitor->print("\t\tdata_length : " + std::to_string(recevive_data_length) + "\n",files++,col);        

        for (auto& byte : zigbee->get_rxinfo()->rx_data)        
            {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << ":";
            }
        std::cout<<"\n";

    
        uint64_t add =ADDRESS_LONG;

        #ifdef DBG_PRINT_GET_INFO 
        
        if(ADDRESS_LONG_SLAVE == add){ std::cout<< "\nmac es igual\n"; }
        else { std::cout<< "\nmac no es igual\n" ; }
            std::cout<< "\ndata_receiver->mac : " << std::hex<< add<<"\n";
            std::cout<< "buffer_receiver->head : " << std::hex << buffer_receiver.head <<"\n";
            auto bs = (!buffer_receiver.size)&0xffff;
            std::cout<< "buffer_receiver->size : " << reinterpret_cast<const int *>(bs)<<"\n";
            std::cout<< "data_receiver->data : " <<reinterpret_cast<const char *>(buffer_receiver.data)<<"\n";
            std::cout<<"\nbuff: \n"<<buffer_receiver.size;
            std::cout<<"\r\n";
        #endif
        
            RST_COLOR() ; 
            SET_COLOR(SET_COLOR_RED_TEXT);
            files++;files++;files++;files++;
            monitor->print(" LQI : " + std::to_string(zigbee->get_rxinfo()->lqi) ,files++,col);
            monitor->print(" RSSI : " + std::to_string(zigbee->get_rxinfo()->rssi) ,files++,col);
        //printf("\nLQI : %d , ",zigbee->get_rxinfo()->lqi);
        //printf("RSSI : %d \n",zigbee->get_rxinfo()->rssi);

        #endif
        RST_COLOR() ;   
        SET_COLOR(SET_COLOR_RED_TEXT);
        update(reinterpret_cast<const char*>(zigbee->get_rxinfo()->rx_data));
    
    }

        Radio_t::~Radio_t() {
            #ifdef DBG_RADIO
                std::cout<<"~Radio_t()\n";
            #endif
        }
}



