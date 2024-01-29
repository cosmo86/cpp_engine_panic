#pragma once
#include <memory>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <string>
#include "SEObject.hpp"
#include "log_handle.h"
#include "dispatcher.h"
#include "HitBanStrategy.cpp"


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
    void Start(std::string mode);
    void add_L2_quoter(){};
    void Stop();
    void add_strategy(const nlohmann::json& j);
    void remove_strategy(int id,std::string SecurityID, const char& eid);
    int GetEvent_q_size();
    nlohmann::json check_runningStrategy();
    nlohmann::json check_removedStrategy();
    void update_delayDuration(int s_id, int new_delay_duration);

    


private:
    std::map<std::string, Lev2MdSpi*> m_L2_quoter_list;
    Lev2MdSpi m_L2_quoter;
    TradeSpi m_trader;
    LoggerPtr m_logger;
    Dispatcher dispatcher;
};