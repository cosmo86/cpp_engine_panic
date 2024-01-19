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