#include "Porter.h"

/*

import ctypes
import json

# Example data
data = {'integer': 1, 'float': 3.14, 'char': 'c', 'string': 'mystring'}
json_str = json.dumps(data)

# Call C++ function

------------------------------------

#include <nlohmann/json.hpp>

extern "C" {
    void process_json(const char* json_str) {
        nlohmann::json j = nlohmann::json::parse(json_str);

        // Access and cast data
        int integer_value = j["integer"].get<int>();
        float float_value = j["float"].get<float>();
        char char_value = j["char"].get<std::string>()[0]; // Assuming single char in string
        std::string string_value = j["string"].get<std::string>();

        // Process the data as needed
        // ...
    }
}
*/

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

void AddStrategy(const char* json_str)
{
    getEngine().add_strategy(s_id);
}

void RemoveStrategy(int s_id)
{
    getEngine().remove_strategy(s_id);
}

//void pauseStrategy(){}

//void resumeStrategy(){}

//void updateStrategyDelay(){}

//void checkRunningStrategy(){}

//void checkRemovedStrategy(){}

//int getEventQueueSize(){}

const char* TestRtnJsonStr(const char* json_str)
{
    nlohmann::json j = nlohmann::json::parse(json_str);

    // Access and cast data
    int buy_trigger_volume = j["BuyTriggerVolume"].get<int>();
    int cancel_trigger_volume = j["CancelVolume"].get<int>();
    int max_trigger_times = j["MaxTriggerTimes"].get<int>();
    int position = j["Position"].get<int>();

    char strate_stock_code[31];
    std::string tempStr_securityID = j["SecurityID"].get<std::string>();
    std::strcpy(strate_stock_code, tempStr_securityID.c_str());

    char strate_SInfo[33];
    std::string tempStr_sinfo = j["ID"].get<std::string>();
    std::strcpy(strate_SInfo, tempStr_sinfo.c_str());

    char strate_stock_name[81];
    std::string tempStr_sname = j["SecurityName"].get<std::string>();
    std::strcpy(strate_SInfo, tempStr_sname.c_str());

    //float cancel_trigger_volume = j["cancel_trigger_volume"].get<float>();

    char strate_exchangeID = j["ExchangeID"].get<std::string>()[0]; // Assuming single char in string
    //std::string string_value = j["string"].get<std::string>();

    std::cout<<"strate_stock_code"<<strate_stock_code<<std::endl;
    std::cout<<"buy_trigger_volume"<<buy_trigger_volume<<std::endl;
    std::cout<<"cancel_trigger_volume"<<cancel_trigger_volume<<std::endl;
    std::cout<<"max_trigger_times"<<max_trigger_times<<std::endl;
    std::cout<<"position"<<position<<std::endl;
    std::cout<<"strate_SInfo"<<strate_SInfo<<std::endl;
    std::cout<<"strate_stock_name"<<strate_stock_name<<std::endl;
    std::cout<<"strate_exchangeID"<<strate_exchangeID<<std::endl;


    return json_str;

}

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