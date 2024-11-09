extern "C"{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <errno.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <poll.h>
}

#include <gpio/gpio.hpp>
#include <config/config.hpp>

#include <iostream>
#include <chrono>
#include <thread>
#include <string_view>
#include <string>


namespace GPIO{

      Gpio_t::Gpio_t(bool& st)
      : m_state   { st }
      {
            settings( m_gpio_in  , DIR_IN  ,filenameGpio);
            settings( m_gpio_out , DIR_OUT ,filenameGpio);
      }

        void Gpio_t::set(){        
                gpio_set_edge (m_gpio_in,EDGE_FALLING);
                #ifdef USE_MRF24_RX
                gpio_set_value(m_gpio_out,VALUE_LOW);
                #else
                gpio_set_value(m_gpio_out,VALUE_HIGH);
                #endif
        }

    /*      HELPER FUNCTIONS       */
    // FILE OPERATION
    int Gpio_t::file_open_and_write_value(const std::string_view fname, const std::string_view wdata)
    {
        //@params : verifica que exista el pin . Si no existe ,retorna -1 .
        const int fd = open(fname.data(), O_WRONLY | O_NONBLOCK);        
        if (fd < 0)
        {
            printf("Could not open file %s...%d\r\n", fname.data(), fd);
            }
        write(fd, wdata.data(), strlen(wdata.data()));        
        close(fd);
        return 0;
    }


    // GPIO EXPORT
    int Gpio_t::gpio_export(const int gpio_num)
    {
        char gpio_str[4];
        sprintf(gpio_str, "%d", gpio_num);
          //std::cout<<"gpio_export\n";
        return file_open_and_write_value(SYSFS_GPIO_PATH SYSFS_GPIO_EXPORT_FN,std::to_string(gpio_num) /*gpio_str*/);
    }

    // GPIO UNEXPORT
    int Gpio_t::gpio_unexport(const int gpio_num)
    {
        char gpio_str[6];
        //    char gpio_str[4];
        sprintf(gpio_str, "%d", gpio_num);
        //std::cout<<"dbg gpio_unexport "<< gpio_num <<"\n";
        return file_open_and_write_value(SYSFS_GPIO_PATH SYSFS_GPIO_UNEXPORT_FN, std::to_string(gpio_num) /*gpio_str*/);
    }

    // GPIO DIRECTION
    int Gpio_t::gpio_set_direction(const int gpio_num, const std::string_view dir)
    {
        char path_str[64];
        sprintf(path_str, "%s/gpio%d%s", SYSFS_GPIO_PATH, gpio_num, SYSFS_GPIO_DIRECTION);
        //std::cout<<"gpio_set_direction\n";
        return file_open_and_write_value(path_str, dir.data());
    }

    // GPIO SET VALUE
    int Gpio_t::gpio_set_value(const int gpio_num, const std::string_view value)
    {
        char path_str[64];
        sprintf(path_str, "%s/gpio%d%s", SYSFS_GPIO_PATH, gpio_num, SYSFS_GPIO_VALUE);
        //std::cout<<"gpio_set_value\n";
        // DBG_GPIO_PRINT(8);
        return file_open_and_write_value(path_str, value.data());
    }

    // GPIO SET EDGE
    int Gpio_t::gpio_set_edge(const int gpio_num, const std::string_view edge)
    {
        char path_str[64];
        sprintf(path_str, "%s/gpio%d%s", SYSFS_GPIO_PATH, gpio_num, SYSFS_GPIO_EDGE);
        //std::cout<<"gpio_set_edge\n";
        return file_open_and_write_value(path_str, edge.data());
    }

    int Gpio_t::gpio_get_fd_to_value(const int gpio_num)
    {
        //int fd;
        char fname[64];
        sprintf(fname, "%s/gpio%d%s", SYSFS_GPIO_PATH, gpio_num, SYSFS_GPIO_VALUE);
        const int fd = open(fname, O_RDONLY | O_NONBLOCK);
        //printf("name : %s\n",fname);
        if (fd < 0)
        {
            printf("Could not open file %s... %d\r\n", fname, fd);
        }
        return fd;
    }



    bool Gpio_t::settings(const int pin , const std::string_view str_v ,std::ifstream& fileTmp){
        const std::string filePathGpio = "/sys/class/gpio/gpio" + std::to_string(pin) + "/direction";                                
        const std::string fNameResult ="echo " + std::to_string(pin) + " > /sys/class/gpio/export";               
        fileTmp.open(filePathGpio.c_str());        
        if(!fileTmp.is_open()){

            const int result_output = std::system(fNameResult.c_str());
            if (result_output == 0) {
                #ifdef DBG_GPIO
                    std::cout << "Pin GPIO "+ std::to_string(pin) +" exported successfully." << "\n";
                #endif
            } else {
                #ifdef DBG_GPIO
                    std::cerr << "Error unexporting GPIO "+ std::to_string(pin) +"." <<"\n";
                #endif
                return false;
            }
        }  
            fileTmp.close();  
            gpio_unexport(pin);        
            gpio_export(pin);             
            gpio_set_direction(pin,str_v.data());
        return true;
    }

    const bool Gpio_t::app(bool& flag) {   // siempre retorna un bool false     
        struct pollfd fdpoll;
        int m_num_fdpoll { 1 };        
        int m_looper { 0 };
        char *buf[64];    

        set();        
        m_gpio_in_fd = gpio_get_fd_to_value(m_gpio_in);//verifica como se encuentra pin in 
        // We will wait for button press here for 10s or exit anyway
        if(m_state==true){
        while(m_looper<READING_STEPS) {//mientras m_looper sea menor a 2 
            memset((void *)&fdpoll,0,sizeof(fdpoll));/// setea todo fdpoll a cero
            fdpoll.fd = m_gpio_in_fd;//pone el resultado de fd
            fdpoll.events = POLLPRI; // POLLPRI = 2 
            m_res = poll(&fdpoll,m_num_fdpoll,POLL_TIMEOUT);//result

            if(m_res < 0) {
                #ifdef DBG_GPIO
                printf("Poll failed...%d\r\n",m_res);   
                #endif         
                }
            if(m_res == 0) {
                #ifdef DBG_GPIO
                    std::cout<<"\nPoll success...timed out or received button press...\r\n";
                #endif
                }
            if(fdpoll.revents & POLLPRI) {
                lseek(fdpoll.fd, 0, SEEK_SET);
                read(fdpoll.fd, buf, 64);// lee el fichero fd del pin 
                #ifdef DBG_GPIO
                    std::cout<<"Standby reading msj mrf24j40...\n";
                #endif
                }
            ++m_looper;
            fflush(stdout);
            }
            //std::this_thread::sleep_for(std::chrono::milliseconds(50));   
        }
        else{            
            #ifdef USE_MRF24_RX
            if(flag==true)gpio_set_value(m_gpio_out,VALUE_HIGH);// pon el pinout en alto

            #else 
            gpio_set_value(m_gpio_out,VALUE_LOW);// pon el pinout en alto
            #endif 
            std::this_thread::sleep_for(std::chrono::milliseconds(100));            
        }
        #ifdef USE_MRF24_RX    
        
        if(flag==true){gpio_set_value(m_gpio_out,VALUE_HIGH);}        
        else {gpio_set_value(m_gpio_out,VALUE_LOW);   }
        #else
        gpio_set_value(m_gpio_out,VALUE_HIGH);// pon el pinout en alto
        #endif

        return false;
    }

    void Gpio_t::CloseGpios()
    {
        if(filenameGpio.is_open())filenameGpio.close();   
        close(m_gpio_in_fd);        
        gpio_set_value(m_gpio_out,VALUE_LOW);
     
        gpio_unexport(m_gpio_out);
        gpio_unexport(m_gpio_in);
    }

    Gpio_t::~Gpio_t(){            
            CloseGpios();                        
        #ifdef DBG_GPIO
            std::cout<<"~Gpio()\n";
        #endif       
    }
}
