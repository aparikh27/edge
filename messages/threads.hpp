#pragma once 

#include <queue>
#include <mutex>
#include <condition_variable>

namespace ember::messaging {

template <typename T> 
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    // Do not allow copies
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    void push(T value);
    bool try_pop(T& value);
    bool wait_and_pop(T& value);
    void shutdown();

    [[nodiscard]] bool empty() const;
    [[nodiscard]] int size() const;

private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex; 
    std::condition_variable m_cv;
    bool m_shutdown{false};
};

template<typename T>
bool ThreadSafeQueue<T>::empty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

template<typename T>
int ThreadSafeQueue<T>::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int>(m_queue.size());
}

template<typename T>
void ThreadSafeQueue<T>::push(T value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_shutdown) return;
    m_queue.push(std::move(value));
    m_cv.notify_one(); 
}

template<typename T>
bool ThreadSafeQueue<T>::try_pop(T& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty()) {
        return false;
    }
    value = std::move(m_queue.front());
    m_queue.pop();
    return true; 
}

template<typename T>
bool ThreadSafeQueue<T>::wait_and_pop(T& value) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this] { return !m_queue.empty() || m_shutdown; });
    
    if (m_queue.empty() && m_shutdown) {
        return false;
    }

    value = std::move(m_queue.front());
    m_queue.pop();
    return true;
}

template<typename T>
void ThreadSafeQueue<T>::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shutdown = true;
    }
    m_cv.notify_all();
}

} // namespace ember::messaging