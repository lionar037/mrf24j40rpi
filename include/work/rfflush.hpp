#pragma once
#include <string_view>
#include <mutex>
#include <vector>
#include <string>


namespace FFLUSH{

struct Fflush_t
{
    
        Fflush_t()=default;
            //initscr(); 
        
        ~Fflush_t()=default;
        /* data */
        
        int funcThread() ;
        void print(std::string_view,int& , int) ;
        void terminal(std::string_view,int& , int) ;
        void print_all();
    //private:
    protected:
        std::mutex m_mtx;
        inline static int row={0};
        std::vector<std::string>message{};

};



}


