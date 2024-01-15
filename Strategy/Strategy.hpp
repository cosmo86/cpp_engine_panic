#pragma once
#include "StrategyBase.h"
#include "Lv2dataModel.hpp"



class Strategy : public StrategyBase 
{
public:
	Strategy(int num) : id(num) {}

	void on_tick(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_Lev2MarketDataField> temp = std::static_pointer_cast<SE_Lev2MarketDataField>(e);
		std::cout << "[Strategy] "<< id <<" on_tick triggered, value: " << temp->SecurityID << std::endl;
	}

	void on_orderDetial(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_Lev2OrderDetailField> temp = std::static_pointer_cast<SE_Lev2OrderDetailField>(e);
		std::cout << "[Strategy] " << id <<" on_order triggered, value: " << temp->SecurityID << std::endl;
	}

	void on_transac(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_Lev2TransactionStruct> temp = std::static_pointer_cast<SE_Lev2TransactionStruct>(e);
		std::cout << "[Strategy] " << id <<" on_transac triggered, value: " << temp->SecurityID << std::endl;
	}
	/*
	void on_cus_event(std::shared_ptr<SEObject> e) 
	{
		std::shared_ptr<SE_Lev2TransactionStruct> temp = std::static_pointer_cast<SE_Lev2TransactionStruct>(e);
		std::cout << "[Strategy] " << id << " on_cus_event triggered, value: " << temp->SecurityID << std::endl;
	}
	*/


private:
	int id;

};