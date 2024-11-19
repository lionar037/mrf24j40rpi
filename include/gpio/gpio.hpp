//codigo gpio.hpp
#pragma once
#ifdef LIBRARIES_BCM2835
#include <config/config.hpp>
#else
#include <string_view> // Para usar std::string_view
#include <fstream>     // Para usar std::ifstream
#include <string>      // Para operaciones con cadenas
//#include <iostream>    // Opcional, para depuración con std::cerr o std::cout
#endif

#include <cstdint>


#ifdef LIBRARIES_BCM2835
#define IN_INTERRUPT    RPI_GPIO_P1_16  // GPIO 23
#define OUT_INTERRUPT   RPI_GPIO_P1_12  // GPIO 18
#else
#define IN_INTERRUPT    23      //GPIO INTERRUPT 
#define OUT_INTERRUPT   12    //GPIO LED DBG    
#endif

#define READING_STEPS   2     //10 originalmente

#define SYSFS_GPIO_PATH             "/sys/class/gpio"
#define SYSFS_GPIO_EXPORT_FN        "/export"
#define SYSFS_GPIO_UNEXPORT_FN      "/unexport"
#define SYSFS_GPIO_VALUE            "/value"
#define SYSFS_GPIO_DIRECTION        "/direction"
#define SYSFS_GPIO_EDGE             "/edge"

#define DIR_IN                      "in"
#define DIR_OUT                     "out"

#define VALUE_HIGH                  "1"
#define VALUE_LOW                   "0"

#define EDGE_RISING                 "rising"
#define EDGE_FALLING                "falling"

#define POLL_TIMEOUT        10*1000

#define DBG_GPIO_PRINT(x) std::cout<<"Step :"<<( x )<<"\n"


namespace GPIO{
/*
#ifdef LIBRARIES_BCM2835
    struct Gpio_t {
            explicit Gpio_t(bool& st);
            ~Gpio_t();
            const bool app(bool& flag);
            void set();

        private:
            void configurePinAsInput    (const uint8_t pin);
            void configurePinAsOutput   (const uint8_t pin);
            void setPinValue            (const uint8_t pin,const bool value);
            bool getPinValue            (const uint8_t pin);
            void waitForInterrupt       (const uint8_t pin);
            bool m_state{false};
    };
*/
//#else
    struct Gpio_t{   
            explicit Gpio_t(bool& st);
            ~Gpio_t();
            const bool app(bool&) ;
        protected:
            int file_open_and_write_value(const std::string_view, const std::string_view);
            int gpio_export(const int);
            int gpio_unexport(const int);
            int gpio_set_direction(const int , const std::string_view );
            int gpio_set_value(const int , const std::string_view);
            int gpio_set_edge(const int , const std::string_view);
            int gpio_get_fd_to_value(const int );                
            bool settings(const int , const std::string_view , std::ifstream& );
            void CloseGpios(void);        
            void set();
        private :
            static inline int static_file_open_and_write_value{0};
            bool m_state{false};
            int m_gpio_in_fd{};
            int m_res{};
            const int   m_gpio_out  { OUT_INTERRUPT };
            const int   m_gpio_in   { IN_INTERRUPT };
            std::ifstream filenameGpio;
    };

//#endif 
}
