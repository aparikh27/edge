#include <gtest/gtest.h>

#include "memory/memory_pool.hpp"
#include "memory/object_pool.hpp"

#include <string>

namespace {

TEST(MemoryPoolTest, RejectsInvalidConfiguration) {
    EXPECT_THROW({
        ember::memory::MemoryPool pool(0, 8);
    }, std::invalid_argument);

    EXPECT_THROW({
        ember::memory::MemoryPool pool(64, 0);
    }, std::invalid_argument);
}

TEST(MemoryPoolTest, AllocatesUntilCapacityIsReached) {
    ember::memory::MemoryPool pool(64, 3);

    void* a = pool.allocate();
    void* b = pool.allocate();
    void* c = pool.allocate();

    EXPECT_NE(a, nullptr);
    EXPECT_NE(b, nullptr);
    EXPECT_NE(c, nullptr);
    EXPECT_EQ(pool.get_allocated_count(), 3u);
    EXPECT_EQ(pool.allocate(), nullptr);

    pool.deallocate(c);
    pool.deallocate(b);
    EXPECT_EQ(pool.get_allocated_count(), 1u);

    void* d = pool.allocate();
    EXPECT_EQ(d, b);
    EXPECT_EQ(pool.get_allocated_count(), 2u);

    pool.deallocate(a);
    pool.deallocate(d);
    EXPECT_EQ(pool.get_allocated_count(), 0u);
}

TEST(MemoryPoolTest, ReusesFreedBlocksInLifoOrder) {
    ember::memory::MemoryPool pool(32, 4);

    void* a = pool.allocate();
    void* b = pool.allocate();
    void* c = pool.allocate();

    pool.deallocate(c);
    pool.deallocate(b);

    void* reused = pool.allocate();
    EXPECT_EQ(reused, b);

    pool.deallocate(reused);
    pool.deallocate(a);

    void* next = pool.allocate();
    EXPECT_NE(next, nullptr);
}

struct TrackedObject {
    static int live_count;

    explicit TrackedObject(int value = 0) : value(value) {
        ++live_count;
    }

    TrackedObject(const TrackedObject&) = delete;
    TrackedObject& operator=(const TrackedObject&) = delete;

    ~TrackedObject() {
        --live_count;
    }

    int value;
};

int TrackedObject::live_count = 0;

TEST(ObjectPoolTest, ConstructsAndReleasesTypedObjects) {
    ember::memory::ObjectPool<TrackedObject, 2> pool;

    TrackedObject* first = pool.acquire(7);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->value, 7);
    EXPECT_EQ(TrackedObject::live_count, 1);

    TrackedObject* second = pool.acquire(9);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->value, 9);
    EXPECT_EQ(TrackedObject::live_count, 2);

    pool.release(first);
    EXPECT_EQ(TrackedObject::live_count, 1);

    TrackedObject* third = pool.acquire(11);
    ASSERT_NE(third, nullptr);
    EXPECT_EQ(third->value, 11);
    EXPECT_EQ(TrackedObject::live_count, 2);

    pool.release(third);
    pool.release(second);
    EXPECT_EQ(TrackedObject::live_count, 0);
}

TEST(ObjectPoolTest, ReacquiresReleasedStorage) {
    ember::memory::ObjectPool<std::string, 2> pool;

    std::string* first = pool.acquire("alpha");
    std::string* second = pool.acquire("beta");
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);

    EXPECT_EQ(*first, "alpha");
    EXPECT_EQ(*second, "beta");

    pool.release(first);
    std::string* replacement = pool.acquire("gamma");
    ASSERT_NE(replacement, nullptr);
    EXPECT_EQ(*replacement, "gamma");

    pool.release(replacement);
    pool.release(second);
    EXPECT_EQ(pool.acquire("done") != nullptr, true);
}

} // namespace
