#include "engine.h"

void Engine::Init()
{
    m_logger = GetLogger();
    
}

void Engine::Start()
{
    dispatcher.init();
    dispatcher.Start();
    std::cout<<"[Engine] dispatcher started"<< std::endl;
    m_logger->info("[Engine] dispatcher started");
    char LEV2MD_TCP_FrontAddress[64];
    char UserID[21];
    char Password[41];
    char mode[21];
    std::cout<<"[Engine] user ,passwork inited"<< std::endl;
    m_logger->info("[Engine] user ,passwork inited");
    strcpy(LEV2MD_TCP_FrontAddress,"tcp://210.14.72.17:6900");
    strcpy(UserID,"00032129");
    strcpy(Password,"19359120");
    strcpy(mode,"test");
    std::cout<<"[Engine] user ,passwork copied"<< std::endl;
    m_L2_quoter.init_queue(&dispatcher._event_q);
    m_L2_quoter.connect( UserID, Password,LEV2MD_TCP_FrontAddress,mode);
    m_logger->info("Engine started");
}

void Engine::Stop()
{
    m_L2_quoter.Stop();
    dispatcher.Stop();
    m_logger->info("Engine stopped");
}


void Engine::add_strategy(int id)
{
    std::shared_ptr<Strategy> temp_strategy = std::make_shared<Strategy>(id);
    dispatcher.add_strategy(id,temp_strategy);

}

void Engine:: remove_strategy(int id)
{
    std::shared_ptr<Strategy> temp_strategy = std::make_shared<Strategy>(id);
    dispatcher.remove_strategy(id);

}