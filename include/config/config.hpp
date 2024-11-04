#pragma once


#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 4)
    // Si es de 32 bits
    #define USE_MRF24_RX
#elif defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
    // Si es de 64 bits
    #define USE_MRF24_TX
    #if defined(__APPLE__)
        // Si es macOS
        #undef USE_MRF24_TX    // Elimina la definición anterior
        #define USE_MRF24_RX   // Redefine a USE_MRF24_RX
        //#define MACOS          // Define MACOS para macOS
    #endif
#else
    #error "Arquitectura no soportada"
#endif


#define USE_MAC_ADDRESS_LONG
//#define USE_MAC_ADDRESS_SHORT


#define HEAD 0x40

#ifdef USE_MRF24_TX
    #define MODULE_TX 
    #define MODULE_TX_RST
    //#define RESET_MRF_SOFTWARE
#elif defined(USE_MRF24_RX)
    #define MODULE_RX 
    #define MODULE_RX_RST
    #define RESET_MRF_SOFTWARE
#else
    #error "no se configuro el dispositivo"
#endif

#ifdef USE_MAC_ADDRESS_SHORT
     #define MSJ "@ABCDE"
#else 
    //#define MSJ "ABCDEFGHIJKLMNOP@"
    #define MSJ "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmno_____________@"    
    //#define MSJ "ABCDEFGHIJKLMKNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz0123456@ABCDEFGHIJKLMKNOPQRS@ABCDEFGHIJKLMKNOPQ" 
#endif

#ifdef MODULE_TX
    #define ADDRESS_LONG        0x1122334455667702
    #define ADDRESS_LONG_SLAVE  0x1122334455667701
    #define ADDRESS             0x6001
    #define PAN_ID              0x1234
    #define ADDR_SLAVE          0x6002
    #define MRF24_TRANSMITER_ENABLE    
    //#define MRF24_RECEIVER_ENABLE
    //#define ENABLE_INTERRUPT_MRF24
    #define CHANNEL 24
#elif defined(MODULE_RX)
    #define ADDRESS_LONG        0x1122334455667701
    #define ADDRESS_LONG_SLAVE  0x1122334455667702
    #define ADDRESS             0x6002
    #define PAN_ID              0x1234
    #define ADDR_SLAVE          0x6001
    #define MRF24_RECEIVER_ENABLE
    #define ENABLE_INTERRUPT_MRF24
    #define CHANNEL 24
#else
    #error "no se configuro el dispositivo"
#endif



#define LOG_FILENAME "log_mrf_"


// Prints Debugger

#define DBG_PRINT_GET_INFO
//#define DBG//DBG_GPIO
//#define DBG_BUFFER
//#define DBG_GPIO //DBG_GPIO
//#define DBG_FILES //imprime Debugger en files
//#define DBG_DISPLAY_OLED
//#define DBG_SPI

//config QR string 
//Linea de configuracion para codigo Qr de una red wifi
//#define QR_CODE_SRT "WIFI:T:WPA;S:MiRedWiFi;P:MiContraseña123;;";
#ifdef MODULE_TX
    #ifdef USE_QR
        #define QR_CODE_SRT MSJ   
    #endif
#endif

#ifdef MODULE_RX
    //#define USE_OLED
    //Enable database 
    //#define ENABLE_DATABASE
    //#define USE_QR
#endif


//config MRF24J40
#define ADD_RSSI_AND_LQI_TO_PACKET
#define MRF24J40_DISABLE_AUTOMATIC_ACK
#define MRF24J40_PAN_COORDINATOR
#define MRF24J40_COORDINATOR
#define MRF24J40_ACCEPT_WRONG_CRC_PKT
#define MRF24J40_PROMISCUOUS_MODE
#define INT_POLARITY_HIGH
