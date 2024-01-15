#pragma once
#include <string>
#include <iostream>
enum Eventtype
{
    TICK,
    ORDER,
    ORDER_SUCCESS,
    ORDER_ERROR,
    CANCEL_SUCCESS,
    CANCEL_ERROR,
    TRADE,
    L2TICK,
    ORDER_DETIAL,
	TRANSACTION,
    NGTSTICK
};

class Event
{
public:
	Event() {}
	Event(Eventtype e_type, std::string payload) :e_type(e_type), payload(payload) { std::cout << "[Event] event created" << std::endl; }
	~Event() {}


	// the following three methods are only used in ThreadSafeQueue class
	// enqueue() and dequeue() methods which are already thread-safe.
	// to avoid performance overhead, no need to locks here

	Eventtype e_type;
	std::string payload;
private:

};