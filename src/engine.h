#pragma once
#include <memory>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <chrono>
#include <iomanip>
#include "SEObject.hpp"
#include "log_handle.h"
#include "dispatcher.h"
#include "Strategy.hpp"


class Engine
{
public:
    Engine(){
        std::cout<<"[Engeine created]"<<std::endl;
    };

    ~Engine(){
        std::cout<<"[Engeine destoried]"<<std::endl;
    };

    void Init();
    void Start();
    void add_L2_quoter(){};
    void Stop();
    void add_strategy(int id);
    void remove_strategy(int id);

    


private:
    std::map<std::string, Lev2MdSpi*> m_L2_quoter_list;
    Lev2MdSpi m_L2_quoter;
    TradeSpi m_trader;
    LoggerPtr m_logger;
    Dispatcher dispatcher;
};