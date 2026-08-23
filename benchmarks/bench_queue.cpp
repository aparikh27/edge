// ThreadSafeQueue & Concurrency Benchmarks
//
// Measures ember::messaging::ThreadSafeQueue<T> under two access patterns:
//   1. SPSC (single-producer / single-consumer)
//   2. MPMC (multi-producer / multi-consumer, high contention)
//
// For each pattern we report combined push+pop throughput (ops/sec) and the
// end-to-end enqueue-to-dequeue latency distribution.
//
// Run: bench_queue[.exe] [--markdown=path/to/file.md]

#include "bench_framework.hpp"
#include "messages/threads.hpp"

#include <atomic>
#include <thread>
#include <vector>

using namespace ember::bench;
using ember::messaging::ThreadSafeQueue;

namespace {

struct QueueItem {
    Clock::time_point t_enqueue;
    std::uint64_t seq{0};
};

constexpr std::size_t kWarmupItems = 2'000;

BenchRow make_row(const std::string& name, const LatencyStats& stats,
                   double elapsed_ns, std::size_t total_ops, const std::string& notes) {
    BenchRow row;
    row.suite = "ThreadSafeQueue / Concurrency";
    row.name = name;
    row.latency = stats;
    row.throughput_value = elapsed_ns > 0.0
        ? static_cast<double>(total_ops) / (elapsed_ns / 1e9)
        : 0.0;
    row.throughput_unit = " ops/sec";
    row.notes = notes;
    return row;
}

void warm_up_queue(ThreadSafeQueue<QueueItem>& queue) {
    for (std::size_t i = 0; i < kWarmupItems; ++i) {
        queue.push(QueueItem{Clock::now(), i});
        QueueItem item;
        queue.try_pop(item);
    }
}

// ---------------------------------------------------------------------------
// SPSC
// ---------------------------------------------------------------------------

BenchRow bench_spsc(std::size_t total_items) {
    ThreadSafeQueue<QueueItem> queue;
    warm_up_queue(queue);

    std::vector<double> latencies_ns;
    latencies_ns.reserve(total_items);

    const double elapsed_ns = time_wall_ns([&] {
        std::thread producer([&] {
            for (std::uint64_t i = 0; i < total_items; ++i) {
                queue.push(QueueItem{Clock::now(), i});
            }
        });

        std::thread consumer([&] {
            QueueItem item;
            for (std::size_t i = 0; i < total_items; ++i) {
                queue.wait_and_pop(item);
                const double lat_ns =
                    std::chrono::duration<double, std::nano>(Clock::now() - item.t_enqueue).count();
                latencies_ns.push_back(lat_ns);
            }
        });

        producer.join();
        consumer.join();
    });

    LatencyStats stats = compute_stats(std::move(latencies_ns));
    // Throughput counts each push and each pop as one op.
    return make_row("SPSC push+pop", stats, elapsed_ns, total_items * 2,
                     std::to_string(total_items) +
                         " messages, 1P/1C, unthrottled producer (latency reflects queue backlog, not per-op cost)");
}

// ---------------------------------------------------------------------------
// MPMC
// ---------------------------------------------------------------------------

BenchRow bench_mpmc(std::size_t producers, std::size_t consumers, std::size_t items_per_producer) {
    ThreadSafeQueue<QueueItem> queue;
    warm_up_queue(queue);

    const std::size_t total_items = producers * items_per_producer;
    std::atomic<std::size_t> consumed_count{0};

    // Each consumer thread accumulates into its own vector to avoid the
    // measurement itself becoming a contention point; merged after join.
    std::vector<std::vector<double>> per_consumer_latencies(consumers);

    const double elapsed_ns = time_wall_ns([&] {
        std::vector<std::thread> producer_threads;
        producer_threads.reserve(producers);
        for (std::size_t p = 0; p < producers; ++p) {
            producer_threads.emplace_back([&, p] {
                for (std::size_t i = 0; i < items_per_producer; ++i) {
                    queue.push(QueueItem{Clock::now(), p * items_per_producer + i});
                }
            });
        }

        std::vector<std::thread> consumer_threads;
        consumer_threads.reserve(consumers);
        for (std::size_t c = 0; c < consumers; ++c) {
            consumer_threads.emplace_back([&, c] {
                auto& local = per_consumer_latencies[c];
                QueueItem item;
                while (consumed_count.load(std::memory_order_relaxed) < total_items) {
                    if (queue.try_pop(item)) {
                        const double lat_ns =
                            std::chrono::duration<double, std::nano>(Clock::now() - item.t_enqueue).count();
                        local.push_back(lat_ns);
                        consumed_count.fetch_add(1, std::memory_order_relaxed);
                    } else {
                        std::this_thread::yield();
                    }
                }
            });
        }

        for (auto& t : producer_threads) t.join();
        for (auto& t : consumer_threads) t.join();
    });

    std::vector<double> merged;
    merged.reserve(total_items);
    for (auto& v : per_consumer_latencies) {
        merged.insert(merged.end(), v.begin(), v.end());
    }

    LatencyStats stats = compute_stats(std::move(merged));
    return make_row("MPMC push+pop", stats, elapsed_ns, total_items * 2,
                     std::to_string(producers) + "P/" + std::to_string(consumers) + "C, " +
                         std::to_string(total_items) + " messages, unthrottled producers (latency reflects backlog)");
}

} // namespace

int main(int argc, char** argv) {
    CliOptions cli = parse_cli(argc, argv);

    constexpr std::size_t kSpscItems = 200'000;
    constexpr std::size_t kMpmcProducers = 4;
    constexpr std::size_t kMpmcConsumers = 4;
    constexpr std::size_t kMpmcItemsPerProducer = 50'000;

    Reporter reporter;
    std::cout << "Running queue/concurrency benchmarks...\n";

    reporter.add(bench_spsc(kSpscItems));
    reporter.add(bench_mpmc(kMpmcProducers, kMpmcConsumers, kMpmcItemsPerProducer));

    reporter.print_console("EMBER ThreadSafeQueue Benchmarks");

    if (cli.has_markdown) {
        reporter.write_markdown(cli.markdown_path, "ThreadSafeQueue / Concurrency Benchmarks");
    }

    return 0;
}
