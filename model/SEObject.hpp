/***********************************************************************
*	@history	2024-01-01
*	@author		CosmoW
*   @contact    Earth
***********************************************************************/


#pragma once
#include <jemalloc/jemalloc.h>
#include <new>
#include <memory>
//#include "StrategyBase.h"
//#include "Strategy.hpp"
#include "Event.h"


class StrategyBase;

class Strategy;

// Custom allocator using je_malloc
template <typename T>
T* custom_allocate() {
    return static_cast<T*>(je_malloc(sizeof(T)));
}

// Custom deleter using je_free
template <typename T>
void custom_deleter(T* ptr) {
    if (ptr) {
        ptr->~T();  // Explicitly call the destructor for the object
        je_free(ptr);
    }
}

class SEObject {
public:
    // Updated Create method
    template <typename T>
    static std::shared_ptr<T> Create() {
        T* mem = custom_allocate<T>();
        if (!mem) {
            throw std::bad_alloc();
        }
        new (mem) T(); // Placement new
        return std::shared_ptr<T>(mem, custom_deleter<T>);
    }
};

class SEEvent
{
public:
    Eventtype e_type;
    std::shared_ptr<SEObject> event;
    char S_id[31] = {};
};

class SETask
{
    typedef void (StrategyBase::* FuncPtr)(const std::shared_ptr<SEObject> e);
public:
    SETask(){}
    SETask(std::shared_ptr<SEObject> evt, FuncPtr cbFuncPtr, std::shared_ptr<StrategyBase> strat)
        : event(evt), _cb_funcPtr(cbFuncPtr), s(strat) { }

    std::shared_ptr<SEObject> event;
    FuncPtr _cb_funcPtr;
    //Strategy* s;
    std::shared_ptr<StrategyBase> s;

};
//l2 quoter ->  SE_task<tick,order,transac>


