#include <gtest/gtest.h>
#include "messages/threads.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <numeric>

using namespace ember::messaging;

TEST(ThreadSafeQueueTest, PushPopSingleThreaded) {
    ThreadSafeQueue<int> queue;

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);

    queue.push(42);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);

    int val = 0;
    EXPECT_TRUE(queue.try_pop(val));
    EXPECT_EQ(val, 42);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);

    queue.push(100);
    int wait_val = 0;
    EXPECT_TRUE(queue.wait_and_pop(wait_val));
    EXPECT_EQ(wait_val, 100);
    EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeQueueTest, EmptyStateBehavior) {
    ThreadSafeQueue<std::string> queue;

    std::string val;
    EXPECT_FALSE(queue.try_pop(val));
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST(ThreadSafeQueueTest, MultiProducerMultiConsumer) {
    ThreadSafeQueue<int> queue;
    constexpr int kProducers = 4;
    constexpr int kConsumers = 4;
    constexpr int kItemsPerProducer = 500;
    constexpr int kTotalItems = kProducers * kItemsPerProducer;

    std::atomic<int> consumed_count{0};
    std::atomic<long long> consumed_sum{0};

    // Expected sum: kProducers * sum(0..499)
    long long expected_sum = 0;
    for (int p = 0; p < kProducers; ++p) {
        for (int i = 0; i < kItemsPerProducer; ++i) {
            expected_sum += i;
        }
    }

    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&queue]() {
            for (int i = 0; i < kItemsPerProducer; ++i) {
                queue.push(i);
            }
        });
    }

    std::vector<std::thread> consumers;
    for (int c = 0; c < kConsumers; ++c) {
        consumers.emplace_back([&queue, &consumed_count, &consumed_sum]() {
            int val = 0;
            while (consumed_count.load() < kTotalItems) {
                if (queue.try_pop(val)) {
                    consumed_sum.fetch_add(val, std::memory_order_relaxed);
                    consumed_count.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }
    for (auto& t : consumers) {
        t.join();
    }

    EXPECT_EQ(consumed_count.load(), kTotalItems);
    EXPECT_EQ(consumed_sum.load(), expected_sum);
    EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeQueueTest, GracefulShutdownUnblocksWaitAndPop) {
    ThreadSafeQueue<int> queue;
    std::atomic<bool> thread_exited{false};
    std::atomic<bool> pop_result{true};

    std::thread worker([&queue, &thread_exited, &pop_result]() {
        int val = 0;
        pop_result = queue.wait_and_pop(val);
        thread_exited = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(thread_exited.load());

    queue.shutdown();
    worker.join();

    EXPECT_TRUE(thread_exited.load());
    EXPECT_FALSE(pop_result.load());
}
