#pragma once
#include <mutex>
#include <queue>

template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    // Delete copy constructor and assignment operator
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // Enqueue an element into the queue
    bool enqueue(const T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(value);
        return true;
    }

    // Try to dequeue an element from the queue. Returns true if successful.
    bool try_dequeue(T& outValue) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return false; // Queue is empty, return false
        }

        outValue = m_queue.front();
        m_queue.pop();
        return true;
    }

    // Check if the queue is empty
    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    // Get the size of the queue
    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

private:
    mutable std::mutex m_mutex;
    std::queue<T> m_queue;
};