#pragma once

#include "memory_pool.hpp"
#include <utility>

namespace ember::memory {

template <typename T, size_t Capacity>
class ObjectPool {
public:
    ObjectPool() : m_pool(sizeof(T), Capacity) {}

    template <typename... Args>
    T* acquire(Args&&... args) {
        void* mem = m_pool.allocate();
        if (!mem) return nullptr;
        
        // Placement new constructs T in the allocated memory chunk
        return ::new (mem) T(std::forward<Args>(args)...);
    }

    void release(T* obj) {
        if (!obj) return;
        
        // Explicitly invoke destructor
        obj->~T();
        
        // Return memory chunk to pool
        m_pool.deallocate(static_cast<void*>(obj));
    }

private:
    MemoryPool m_pool;
};

} // namespace ember::memory