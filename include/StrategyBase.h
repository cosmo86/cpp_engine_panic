#pragma once
//#include "SEObject.hpp"


class SEObject;

class StrategyBase
{
public:
	StrategyBase() = default;

	virtual void on_tick(std::shared_ptr<SEObject> e) = 0;
	virtual void on_orderDetial(std::shared_ptr<SEObject> e) = 0;
	virtual void on_transac(std::shared_ptr<SEObject> e) = 0;
	//tual void on_cus_event(std::shared_ptr<SEObject> e) {};

private:


};