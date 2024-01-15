#include "Porter.h"



Engine& getEngine()
{
    static Engine engine; 
    return engine;
}

void startEngine()
{
    getEngine().Start();
}

void stopEngine()
{
    getEngine().Stop();
}

void initEngine() // this should handle register logger
{
    getEngine().Init();
}

void addStrategy(int s_id)
{
    getEngine().add_strategy(s_id);
}

void removeStrategy(int s_id)
{
    getEngine().remove_strategy(s_id);
}

//void pauseStrategy(){}

//void resumeStrategy(){}

//void updateStrategyDelay(){}

//void checkRunningStrategy(){}

//void checkRemovedStrategy(){}

//int getEventQueueSize(){}


const char*  testreturnstr()
{
    return "test";
}

int testreturnint()
{
    int a = 1;
    return a;
}

void testtakestr(const char* str)
{
    std::cout<<"take string is "<< str <<std::endl;
}