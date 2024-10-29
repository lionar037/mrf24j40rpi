/*
author : lion
*/
#include <mrf24/radio.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>



int main(){
    //system("clear"); 
    auto mrf { std::make_unique<MRF24J40::Radio_t>()};
    // std::vector<std::thread> threadVect;
    // const int ThreadCant{4};

//std::cout << "\033[2J\033[H" << std::flush;
     try    {
// 
        // for (int i = 0; i < ThreadCant; ++i) {
            // threadVect.emplace_back([&mrf]() {
            mrf->Run();
            // });
        //}
// 
        //Esperar a que todos los hilos terminen
        // for (auto& threadRun : threadVect) {
            // threadRun.join();
        // }      
      }
       catch(...){
         std::cerr<<"\nerror :(\n";
      }
    return 0 ;
}
