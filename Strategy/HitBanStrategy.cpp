#pragma once
#include "StrategyBase.h"
#include "Lv2dataModel.hpp"
#include "OrderModels.hpp"
#include "json.hpp"

class Dispatcher;

class HitBanStrategy : public StrategyBase 
{
private:
    Dispatcher* m_dispatcher_ptr = nullptr;
    json m_stratParams;
}