#include "engine.h"

void Engine::Init()
{
    m_logger = GetLogger();
    
}

void Engine::Start(std::string mode_str)
{
    if(mode_str == "test")
    {
        std::cout<<"[Engine] [TEST] dispatcher started"<< std::endl;
        m_logger->info("[Engine] dispatcher started");
        char LEV2MD_TCP_FrontAddress[64];
        const char* TD_TCP_FrontAddress = "tcp://210.14.72.21:4400";
        char UserID[21];
        char Password[41];
        char mode[21];
        std::cout<<"[Engine] user ,passwork inited"<< std::endl;
        m_logger->info("[Engine] user ,passwork inited");
        strcpy(LEV2MD_TCP_FrontAddress,"tcp://210.14.72.17:6900");
        strcpy(UserID,"00032129");
        strcpy(Password,"19359120");
        strcpy(mode,"test");

        m_trader.init_trader(&dispatcher._event_q, m_logger);
        m_trader.connect( UserID, Password,TD_TCP_FrontAddress,mode);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        m_L2_quoter.init_quoter(&dispatcher._event_q, m_logger);
        m_L2_quoter.connect( UserID, Password,LEV2MD_TCP_FrontAddress,mode);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        dispatcher.init(&m_L2_quoter , &m_trader,m_logger);
        dispatcher.Start();
        m_logger->info("Engine Test started");
    }
    else if(mode == "server")
    {
        std::cout<<"[Engine] [SERVER] dispatcher started"<< std::endl;
        m_logger->info("[Engine] dispatcher started");
        char LEV2MD_TCP_FrontAddress[64];
        const char* TD_TCP_FrontAddress = "tcp://10.224.78.106:6500";
        char UserID[21];
        char Password[41];
        char mode[21];
        std::cout<<"[Engine] user ,passwork inited"<< std::endl;
        m_logger->info("[Engine] user ,passwork inited");
        strcpy(LEV2MD_TCP_FrontAddress,"udp://224.224.2.2:7889");
        strcpy(UserID,"320000077997");
        strcpy(Password,"298500");
        strcpy(mode,"server");

        m_trader.init_trader(&dispatcher._event_q, m_logger);
        m_trader.connect( UserID, Password,TD_TCP_FrontAddress,mode);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        m_L2_quoter.init_quoter(&dispatcher._event_q, m_logger);
        m_L2_quoter.connect( UserID, Password,LEV2MD_TCP_FrontAddress,mode);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        dispatcher.init(&m_L2_quoter , &m_trader,m_logger);
        dispatcher.Start();
        m_logger->info("Engine Server started");
    }


}

void Engine::Stop()
{
    m_L2_quoter.Stop();
    dispatcher.Stop();
    m_logger->info("Engine stopped");
}


void Engine::add_strategy(const nlohmann::json& j)
{
    // Convert the string to an integer
    std::string idStr = j["ID"].get<std::string>();
    std::string SecurityID = j["SecurityID"].get<std::string>();
    char strate_exchangeID = j["ExchangeID"].get<std::string>()[0];
    int idInt = std::atoi(idStr.c_str());

    std::shared_ptr<HitBanStrategy> temp_strategy = std::make_shared<HitBanStrategy>(j, &dispatcher,m_logger);

    dispatcher.add_strategy(idInt,SecurityID, strate_exchangeID , temp_strategy);

}

void Engine:: remove_strategy(int id, std::string SecurityID, const char& eid)
{
    dispatcher.remove_strategy(id,SecurityID, eid );
}

void Engine:: update_delayDuration(int s_id, int new_delay_duration)
{
    dispatcher.update_delay_duration(s_id,new_delay_duration);
}

int Engine:: GetEvent_q_size()
{
    return dispatcher.get_event_q_size();
}

nlohmann::json Engine:: check_runningStrategy()
{
    return dispatcher.check_running_strategy();
}
                 
nlohmann::json Engine:: check_removedStrategy()
{
    nlohmann::json res = dispatcher.check_removed_strategy();
    std::cout<<"[Engine] "<<res[0]["SecurityID"]<<std::endl;
    std::cout<<"[Engine] "<<res[0]["ID"]<<std::endl;
    return res;
}
