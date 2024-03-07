#pragma once
#include <iostream>
#include "engine.h"
#include "SEObject.hpp"
#include "json.hpp"

#ifndef EXPORT_FLAG
#ifdef _MSC_VER
#	define EXPORT_FLAG __declspec(dllexport)
#else
#	define EXPORT_FLAG __attribute__((__visibility__("default")))
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    EXPORT_FLAG const char* testreturnstr();

    EXPORT_FLAG const char* TestRtnJsonStr(const char* json_str);

    EXPORT_FLAG int testreturnint();

    EXPORT_FLAG void testtakestr(const char* str);

    EXPORT_FLAG void startEngine(const char* mode);

    EXPORT_FLAG void stopEngine();

    EXPORT_FLAG void initEngine();

    EXPORT_FLAG void AddStrategy(const char* json_str);

    EXPORT_FLAG void RemoveStrategy(int s_id, const char* str , char eid);

    EXPORT_FLAG int GetEventQSize();

    EXPORT_FLAG const char* CheckRunningStrategy();

    EXPORT_FLAG const char* CheckRemovedStrategy();

    EXPORT_FLAG void free_string(char* str);

    EXPORT_FLAG void UpdateDelayDuration(int s_id, int new_delay_duration);

    EXPORT_FLAG void manual_Send_Order_LimitPrice( const char exchange_id, const int volume, const double price, const char* stock_id , 
								const char* req_sinfo, const int order_ref, const int req_iinfo = 0);

    EXPORT_FLAG void manual_Send_Cancle_Order_OrderActionRef( const char exchange_id ,const char* req_sinfo, const int order_ref ,const int order_action_ref , const int req_iinfo = 0 );
    

#ifdef __cplusplus
}
#endif