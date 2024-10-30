#pragma once
#include <string_view>
#include <mutex>


namespace FFLUSH{

struct Fflush_t
{
    
        Fflush_t()=default;
            //initscr(); 
        
        ~Fflush_t()=default;
        /* data */
        
        int funcThread() ;
        void print(std::string_view,int& , int) ;
    //private:
    protected:
        std::mutex m_mtx;
        inline static int row{0};

};



}


