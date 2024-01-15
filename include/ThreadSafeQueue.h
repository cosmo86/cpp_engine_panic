#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <exception>
#include <iostream>

template<typename T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue() { std::cout << "[Queue] A thread-safe queue is created, address is :" << this << std::endl; }
	ThreadSafeQueue(const ThreadSafeQueue& other) = delete; // disable copying
	ThreadSafeQueue& operator=(const ThreadSafeQueue& other) = delete; // disable assignment

	void enqueue(T item) 
	{
		std::lock_guard<std::mutex> lock(_mtx);
		queue.push(std::move(item));
		condVar.notify_one();
	}

	T dequeue() 
	{
		std::unique_lock<std::mutex> lock(_mtx);
		condVar.wait(lock, [this] { return !queue.empty(); });
		T item = std::move(queue.front());
		queue.pop();
		return item;
	}

	bool is_empty() const 
	{
		std::lock_guard<std::mutex> lock(_mtx);
		//check if queue is empty
		return queue.empty();
	};

	size_t size() const 
	{
		std::lock_guard<std::mutex> lock(_mtx);
		//get the size
		return queue.size();
	};


private:
	std::queue<T> queue;
	mutable std::mutex _mtx;
	std::condition_variable condVar;

};