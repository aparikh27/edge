// Pub/Sub Message Bus Benchmarks
//
// EMBER has two publish/subscribe primitives and this suite covers both:
//   1. ember::events::EventBus     — synchronous, in-process typed dispatch.
//      publish() calls every handler inline, so "dispatch latency" here is
//      the time from entering publish() to a handler actually running.
//   2. ember::messaging::Coordinator/Publisher/Subscriber — the topic-based
//      asynchronous message bus, where publish() fans a message out to each
//      matching subscriber's queue and a worker thread pops it. Message
//      already carries a timestamp, so end-to-end latency is publish-call
//      to worker-thread pop.
//
// Both are measured across 1, 5, and 10 active subscriber handlers.
//
// Run: bench_pubsub[.exe] [--markdown=path/to/file.md]

#include "bench_framework.hpp"
#include "../events/EventBus.hpp"
#include "../messages/coordinator.hpp"
#include "../messages/publisher.hpp"
#include "../messages/subscriber.hpp"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

using namespace ember::bench;

namespace {

constexpr std::size_t kWarmup = 1'000;
constexpr std::size_t kPublishCount = 20'000;

BenchRow make_row(const std::string& suite, const std::string& name, const LatencyStats& stats,
                   double throughput_value, const std::string& unit, const std::string& notes) {
    BenchRow row;
    row.suite = suite;
    row.name = name;
    row.latency = stats;
    row.throughput_value = throughput_value;
    row.throughput_unit = unit;
    row.notes = notes;
    return row;
}

// ---------------------------------------------------------------------------
// 1. EventBus — synchronous dispatch
// ---------------------------------------------------------------------------

struct BenchEvent {
    std::uint64_t seq{0};
    Clock::time_point publish_time{};
};

BenchRow bench_eventbus(std::size_t subscriber_count) {
    ember::events::EventBus bus;
    std::vector<double> latencies_ns;
    latencies_ns.reserve(kPublishCount * subscriber_count);

    // Every handler records the time from publish() to its own execution;
    // with N handlers each publish() yields N latency samples.
    for (std::size_t i = 0; i < subscriber_count; ++i) {
        bus.subscribe<BenchEvent>([&latencies_ns](const BenchEvent& e) {
            const double lat_ns =
                std::chrono::duration<double, std::nano>(Clock::now() - e.publish_time).count();
            latencies_ns.push_back(lat_ns);
        });
    }

    for (std::size_t i = 0; i < kWarmup; ++i) {
        bus.publish(BenchEvent{i, Clock::now()});
    }
    latencies_ns.clear();

    const double elapsed_ns = time_wall_ns([&] {
        for (std::size_t i = 0; i < kPublishCount; ++i) {
            bus.publish(BenchEvent{i, Clock::now()});
        }
    });

    LatencyStats stats = compute_stats(std::move(latencies_ns));
    const double handler_invocations = static_cast<double>(kPublishCount * subscriber_count);
    const double msgs_per_sec = handler_invocations / (elapsed_ns / 1e9);

    return make_row("Pub/Sub EventBus (sync)",
                     std::to_string(subscriber_count) + " subscriber(s)",
                     stats, msgs_per_sec, " handler-calls/sec",
                     std::to_string(kPublishCount) + " publishes");
}

// ---------------------------------------------------------------------------
// 2. Coordinator/Publisher/Subscriber — async topic-based message bus
// ---------------------------------------------------------------------------

BenchRow bench_coordinator(std::size_t subscriber_count) {
    using ember::messaging::Coordinator;
    using ember::messaging::Message;
    using ember::messaging::Publisher;
    using ember::messaging::Subscriber;

    Coordinator coordinator;
    Publisher publisher(&coordinator);
    const std::string topic = "bench/topic";

    std::vector<std::shared_ptr<Subscriber>> subscribers;
    subscribers.reserve(subscriber_count);
    for (std::size_t i = 0; i < subscriber_count; ++i) {
        auto sub = std::make_shared<Subscriber>(topic);
        coordinator.subscribe(sub);
        subscribers.push_back(sub);
    }

    // Warm-up: push and drain a few messages so lazy setup (allocator pages,
    // mutex contention, thread scheduling) doesn't pollute timed samples.
    for (std::size_t i = 0; i < kWarmup; ++i) {
        publisher.push(Message{topic, std::chrono::steady_clock::now(), ""});
    }
    for (auto& sub : subscribers) {
        Message m;
        while (sub->pop(m)) { /* drain */ }
    }

    std::vector<std::vector<double>> per_subscriber_latencies(subscriber_count);
    std::atomic<std::size_t> consumed_total{0};
    const std::size_t expected_total = kPublishCount * subscriber_count;

    const double elapsed_ns = time_wall_ns([&] {
        std::vector<std::thread> consumer_threads;
        consumer_threads.reserve(subscriber_count);
        for (std::size_t i = 0; i < subscriber_count; ++i) {
            consumer_threads.emplace_back([&, i] {
                auto& local = per_subscriber_latencies[i];
                Message msg;
                for (std::size_t received = 0; received < kPublishCount; ++received) {
                    if (subscribers[i]->wait_and_pop(msg)) {
                        const double lat_ns =
                            std::chrono::duration<double, std::nano>(
                                std::chrono::steady_clock::now() - msg.timestamp).count();
                        local.push_back(lat_ns);
                        consumed_total.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }

        std::thread producer([&] {
            for (std::size_t i = 0; i < kPublishCount; ++i) {
                publisher.push(Message{topic, std::chrono::steady_clock::now(), ""});
            }
        });

        producer.join();
        for (auto& t : consumer_threads) t.join();
    });

    (void)expected_total;
    std::vector<double> merged;
    merged.reserve(consumed_total.load());
    for (auto& v : per_subscriber_latencies) {
        merged.insert(merged.end(), v.begin(), v.end());
    }

    LatencyStats stats = compute_stats(std::move(merged));
    const double msgs_per_sec = static_cast<double>(consumed_total.load()) / (elapsed_ns / 1e9);

    for (auto& sub : subscribers) sub->stop();

    return make_row("Pub/Sub Coordinator (async)",
                     std::to_string(subscriber_count) + " subscriber(s)",
                     stats, msgs_per_sec, " deliveries/sec",
                     std::to_string(kPublishCount) + " publishes, fan-out to " +
                         std::to_string(subscriber_count));
}

} // namespace

int main(int argc, char** argv) {
    CliOptions cli = parse_cli(argc, argv);

    Reporter reporter;
    std::cout << "Running pub/sub message bus benchmarks (" << kPublishCount
               << " publishes per configuration)...\n";

    for (std::size_t n : {1u, 5u, 10u}) {
        reporter.add(bench_eventbus(n));
    }
    for (std::size_t n : {1u, 5u, 10u}) {
        reporter.add(bench_coordinator(n));
    }

    reporter.print_console("EMBER Pub/Sub Message Bus Benchmarks");

    if (cli.has_markdown) {
        reporter.write_markdown(cli.markdown_path, "Pub/Sub Message Bus Benchmarks");
    }

    return 0;
}
