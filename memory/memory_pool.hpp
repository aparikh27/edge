#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <mutex>
#include <new>

namespace ember::memory {

class MemoryPool {
public:
    MemoryPool(size_t chunk_size, size_t chunk_count)
        : m_chunk_size(align_up(chunk_size, alignof(std::max_align_t))),
          m_chunk_count(chunk_count),
          m_buffer(m_chunk_size * chunk_count) {
        
        // Build the embedded free list
        uint8_t* start = m_buffer.data();
        for (size_t i = 0; i < m_chunk_count - 1; ++i) {
            auto* current = reinterpret_cast<Node*>(start + (i * m_chunk_size));
            auto* next = reinterpret_cast<Node*>(start + ((i + 1) * m_chunk_size));
            current->next = next;
        }
        
        // Last node points to nullptr
        auto* last = reinterpret_cast<Node*>(start + ((m_chunk_count - 1) * m_chunk_size));
        last->next = nullptr;

        m_free_head = reinterpret_cast<Node*>(start);
    }

    ~MemoryPool() = default;

    // Prevent copies
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

        void* allocate() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_free_head) {
            return nullptr; // Pool exhausted
        }

        Node* chunk = m_free_head;
        m_free_head = m_free_head->next;
        ++m_allocated_chunks;
        return static_cast<void*>(chunk);
    }

    void deallocate(void* ptr) {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(m_mutex);
        auto* node = static_cast<Node*>(ptr);
        node->next = m_free_head;
        m_free_head = node;
        --m_allocated_chunks;
    }

    [[nodiscard]] size_t get_capacity() const { return m_chunk_count; }
    [[nodiscard]] size_t get_allocated_count() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_allocated_chunks;
    }
private:
    struct Node {
        Node* next{nullptr};
    };

    static constexpr size_t align_up(size_t size, size_t alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    size_t m_chunk_size;
    size_t m_chunk_count;
    std::vector<uint8_t> m_buffer; // Contiguous pool backing storage
    Node* m_free_head{nullptr};
    size_t m_allocated_chunks{0};
    mutable std::mutex m_mutex;
};

} // namespace ember::memory