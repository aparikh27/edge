// Memory Management Benchmarks
//
// Compares ember::memory::MemoryPool against the standard heap allocator
// (::operator new / ::operator delete) for:
//   1. Steady-state allocate/deallocate latency + throughput.
//   2. Behavior under simulated heap fragmentation.
//
// Run: bench_memory[.exe] [--markdown=path/to/file.md]

#include "bench_framework.hpp"
#include "memory/memory_pool.hpp"

#include <cstdlib>
#include <memory>
#include <random>
#include <vector>

using namespace ember::bench;

namespace {

constexpr std::size_t kIterations = 100'000;
constexpr std::size_t kWarmup = 5'000;
constexpr std::size_t kChunkSize = 64; // bytes; representative of a small telemetry/message struct

BenchRow make_row(const std::string& name, const LatencyStats& stats,
                   double elapsed_ns, std::size_t ops, const std::string& notes) {
    BenchRow row;
    row.suite = "Memory Management";
    row.name = name;
    row.latency = stats;
    row.throughput_value = elapsed_ns > 0.0
        ? static_cast<double>(ops) / (elapsed_ns / 1e9)
        : 0.0;
    row.throughput_unit = " ops/sec";
    row.notes = notes;
    return row;
}

// ---------------------------------------------------------------------------
// Scenario 1: steady-state round-trip allocate() + deallocate()
// ---------------------------------------------------------------------------

BenchRow bench_pool_steady_state() {
    ember::memory::MemoryPool pool(kChunkSize, /*chunk_count=*/1024);

    double elapsed_ns = 0.0;
    LatencyStats stats;
    elapsed_ns = time_wall_ns([&] {
        stats = time_each_call(kIterations, kWarmup, [&] {
            void* p = pool.allocate();
            pool.deallocate(p);
        });
    });

    return make_row("MemoryPool: alloc+dealloc", stats, elapsed_ns, kIterations,
                     std::to_string(kIterations) + " iters, " + std::to_string(kChunkSize) + "B chunks");
}

BenchRow bench_heap_steady_state() {
    double elapsed_ns = 0.0;
    LatencyStats stats;
    elapsed_ns = time_wall_ns([&] {
        stats = time_each_call(kIterations, kWarmup, [&] {
            void* p = ::operator new(kChunkSize);
            ::operator delete(p);
        });
    });

    return make_row("Heap (::operator new/delete): alloc+dealloc", stats, elapsed_ns, kIterations,
                     std::to_string(kIterations) + " iters, " + std::to_string(kChunkSize) + "B");
}

// ---------------------------------------------------------------------------
// Scenario 2: simulated heap fragmentation
//
// A background arena is churned with randomly-sized allocations that are
// freed out of order, leaving the general-purpose heap fragmented. We then
// measure allocation latency for our target chunk size against that
// fragmented backdrop. MemoryPool draws from its own pre-reserved,
// contiguous arena, so it is unaffected; the raw heap allocator has to work
// harder to satisfy same-size requests once free lists are fragmented,
// which typically shows up as a widened tail (P99/Max).
// ---------------------------------------------------------------------------

class HeapFragmenter {
public:
    explicit HeapFragmenter(std::size_t block_count, unsigned seed = 1337)
        : rng_(seed), size_dist_(16, 512) {
        blocks_.reserve(block_count);
        for (std::size_t i = 0; i < block_count; ++i) {
            blocks_.push_back(::operator new(size_dist_(rng_)));
        }
        // Free every other block to punch holes through the arena.
        for (std::size_t i = 0; i < blocks_.size(); i += 2) {
            ::operator delete(blocks_[i]);
            blocks_[i] = nullptr;
        }
    }

    ~HeapFragmenter() {
        for (void* p : blocks_) {
            if (p) ::operator delete(p);
        }
    }

private:
    std::mt19937 rng_;
    std::uniform_int_distribution<std::size_t> size_dist_;
    std::vector<void*> blocks_;
};

BenchRow bench_pool_fragmented() {
    HeapFragmenter fragmenter(20'000); // fragments the *general* heap arena
    ember::memory::MemoryPool pool(kChunkSize, /*chunk_count=*/1024);

    double elapsed_ns = 0.0;
    LatencyStats stats;
    elapsed_ns = time_wall_ns([&] {
        stats = time_each_call(kIterations, kWarmup, [&] {
            void* p = pool.allocate();
            pool.deallocate(p);
        });
    });

    return make_row("MemoryPool: alloc+dealloc (heap fragmented)", stats, elapsed_ns, kIterations,
                     "own arena is unaffected by external fragmentation");
}

BenchRow bench_heap_fragmented() {
    HeapFragmenter fragmenter(20'000);

    double elapsed_ns = 0.0;
    LatencyStats stats;
    elapsed_ns = time_wall_ns([&] {
        stats = time_each_call(kIterations, kWarmup, [&] {
            void* p = ::operator new(kChunkSize);
            ::operator delete(p);
        });
    });

    return make_row("Heap: alloc+dealloc (fragmented)", stats, elapsed_ns, kIterations,
                     "20k random-sized blocks pre-freed in alternating pattern");
}

} // namespace

int main(int argc, char** argv) {
    CliOptions cli = parse_cli(argc, argv);

    Reporter reporter;
    std::cout << "Running memory benchmarks (" << kIterations << " iterations, "
               << kWarmup << " warm-up)...\n";

    reporter.add(bench_pool_steady_state());
    reporter.add(bench_heap_steady_state());
    reporter.add(bench_pool_fragmented());
    reporter.add(bench_heap_fragmented());

    reporter.print_console("EMBER Memory Management Benchmarks");

    if (cli.has_markdown) {
        reporter.write_markdown(cli.markdown_path, "Memory Management Benchmarks");
    }

    return 0;
}
