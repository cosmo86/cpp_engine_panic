#pragma once
#include <iostream>
#include "engine.h"
#include "SEObject.hpp"

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

    EXPORT_FLAG int testreturnint();

    EXPORT_FLAG void testtakestr(const char* str);

    EXPORT_FLAG void startEngine();

    EXPORT_FLAG void stopEngine();

    EXPORT_FLAG void initEngine();

    EXPORT_FLAG void addStrategy(int s_id);

    EXPORT_FLAG void removeStrategy(int s_id);

#ifdef __cplusplus
}
#endif