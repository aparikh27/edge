#pragma once

// EMBER Benchmarking Framework
//
// Lightweight, dependency-free statistics/reporting layer shared by every
// benchmarks/bench_*.cpp executable. Uses std::chrono::high_resolution_clock
// for timing (Google Benchmark is not vendored into this project, so this
// header fills that role while keeping the suite dependency-free).

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace ember::bench {

using Clock = std::chrono::high_resolution_clock;

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

struct LatencyStats {
    double min_ns{0.0};
    double max_ns{0.0};
    double mean_ns{0.0};
    double p50_ns{0.0};
    double p95_ns{0.0};
    double p99_ns{0.0};
    double stddev_ns{0.0};
    std::size_t sample_count{0};
};

// Linear-interpolated percentile over a *sorted* sample set (matches the
// common "R7" definition used by numpy/most benchmarking tools).
inline double percentile(const std::vector<double>& sorted_samples, double pct) {
    if (sorted_samples.empty()) return 0.0;
    if (sorted_samples.size() == 1) return sorted_samples.front();

    const double rank = (pct / 100.0) * static_cast<double>(sorted_samples.size() - 1);
    const auto lo = static_cast<std::size_t>(std::floor(rank));
    const auto hi = static_cast<std::size_t>(std::ceil(rank));
    if (lo == hi) return sorted_samples[lo];

    const double frac = rank - static_cast<double>(lo);
    return sorted_samples[lo] + (sorted_samples[hi] - sorted_samples[lo]) * frac;
}

inline LatencyStats compute_stats(std::vector<double> samples_ns) {
    LatencyStats s;
    s.sample_count = samples_ns.size();
    if (samples_ns.empty()) return s;

    std::sort(samples_ns.begin(), samples_ns.end());

    s.min_ns = samples_ns.front();
    s.max_ns = samples_ns.back();

    const double sum = std::accumulate(samples_ns.begin(), samples_ns.end(), 0.0);
    s.mean_ns = sum / static_cast<double>(samples_ns.size());

    double sq_diff_sum = 0.0;
    for (double v : samples_ns) {
        const double d = v - s.mean_ns;
        sq_diff_sum += d * d;
    }
    s.stddev_ns = samples_ns.size() > 1
        ? std::sqrt(sq_diff_sum / static_cast<double>(samples_ns.size() - 1))
        : 0.0;

    s.p50_ns = percentile(samples_ns, 50.0);
    s.p95_ns = percentile(samples_ns, 95.0);
    s.p99_ns = percentile(samples_ns, 99.0);
    return s;
}

// ---------------------------------------------------------------------------
// Timing helpers
// ---------------------------------------------------------------------------

// Times `iterations` individual invocations of fn() (each call is one
// sample), after first running `warmup` untimed invocations to flush cache
// effects / page faults / lazy allocator setup out of the timed region.
template <typename Fn>
LatencyStats time_each_call(std::size_t iterations, std::size_t warmup, Fn&& fn) {
    for (std::size_t i = 0; i < warmup; ++i) {
        fn();
    }

    std::vector<double> samples_ns;
    samples_ns.reserve(iterations);
    for (std::size_t i = 0; i < iterations; ++i) {
        const auto t0 = Clock::now();
        fn();
        const auto t1 = Clock::now();
        samples_ns.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
    }
    return compute_stats(std::move(samples_ns));
}

// Wall-clock duration (nanoseconds) of a block that may internally perform
// many operations / spawn threads. Used for throughput-oriented benchmarks
// where per-call timing would perturb contention behavior.
template <typename Fn>
double time_wall_ns(Fn&& fn) {
    const auto t0 = Clock::now();
    fn();
    const auto t1 = Clock::now();
    return std::chrono::duration<double, std::nano>(t1 - t0).count();
}

// ---------------------------------------------------------------------------
// Formatting
// ---------------------------------------------------------------------------

inline std::string format_ns_as_us(double ns) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << (ns / 1000.0);
    return oss.str();
}

// Adds thousands separators and, above 1e6/1e9, a K/M/G suffix for readable
// throughput figures (e.g. "3.42 M").
inline std::string format_scaled(double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    if (value >= 1e9) {
        oss << (value / 1e9) << " G";
    } else if (value >= 1e6) {
        oss << (value / 1e6) << " M";
    } else if (value >= 1e3) {
        oss << (value / 1e3) << " K";
    } else {
        oss << value << " ";
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
// Result rows + reporters
// ---------------------------------------------------------------------------

struct BenchRow {
    std::string suite;             // e.g. "Memory Management"
    std::string name;              // e.g. "MemoryPool alloc/dealloc"
    LatencyStats latency;          // per-op latency, in nanoseconds
    double throughput_value{-1.0}; // -1 => not applicable / not printed
    std::string throughput_unit;   // e.g. "ops/sec", "bytes/sec", "msgs/sec"
    std::string notes;             // free-form context (iteration count, thread count, ...)
};

class Reporter {
public:
    void add(BenchRow row) { rows_.push_back(std::move(row)); }

    void print_console(const std::string& title) const {
        std::cout << "\n=== " << title << " ===\n";

        std::string current_suite;
        for (const auto& r : rows_) {
            if (r.suite != current_suite) {
                current_suite = r.suite;
                std::cout << "\n-- " << current_suite << " --\n";
                print_header();
            }
            print_row(r);
        }
        std::cout << std::endl;
    }

    // Appends (or creates) a GitHub-flavored Markdown table, ready to paste
    // straight into a README.
    void write_markdown(const std::string& path, const std::string& title) const {
        std::ofstream out(path, std::ios::app);
        if (!out) {
            std::cerr << "warning: could not open markdown output file: " << path << "\n";
            return;
        }

        out << "\n### " << title << "\n\n";
        out << "| Suite | Benchmark | Min (µs) | P50 (µs) | Mean (µs) | P95 (µs) | P99 (µs) | Max (µs) | Throughput | Notes |\n";
        out << "|---|---|---:|---:|---:|---:|---:|---:|---:|---|\n";

        for (const auto& r : rows_) {
            out << "| " << r.suite
                << " | " << r.name
                << " | " << format_ns_as_us(r.latency.min_ns)
                << " | " << format_ns_as_us(r.latency.p50_ns)
                << " | " << format_ns_as_us(r.latency.mean_ns)
                << " | " << format_ns_as_us(r.latency.p95_ns)
                << " | " << format_ns_as_us(r.latency.p99_ns)
                << " | " << format_ns_as_us(r.latency.max_ns)
                << " | " << throughput_cell(r)
                << " | " << r.notes
                << " |\n";
        }

        std::cout << "Markdown table appended to: " << path << "\n";
    }

    [[nodiscard]] const std::vector<BenchRow>& rows() const { return rows_; }

private:
    static void print_header() {
        std::cout << std::left
                   << std::setw(34) << "Benchmark"
                   << std::right
                   << std::setw(10) << "Min(us)"
                   << std::setw(10) << "P50(us)"
                   << std::setw(10) << "Mean(us)"
                   << std::setw(10) << "P95(us)"
                   << std::setw(10) << "P99(us)"
                   << std::setw(10) << "Max(us)"
                   << "   " << std::left << "Throughput"
                   << "\n";
        std::cout << std::string(120, '-') << "\n";
    }

    static std::string throughput_cell(const BenchRow& r) {
        if (r.throughput_value < 0.0) return "-";
        std::ostringstream oss;
        oss << format_scaled(r.throughput_value) << r.throughput_unit;
        return oss.str();
    }

    static void print_row(const BenchRow& r) {
        std::cout << std::left << std::setw(34) << r.name
                   << std::right
                   << std::setw(10) << format_ns_as_us(r.latency.min_ns)
                   << std::setw(10) << format_ns_as_us(r.latency.p50_ns)
                   << std::setw(10) << format_ns_as_us(r.latency.mean_ns)
                   << std::setw(10) << format_ns_as_us(r.latency.p95_ns)
                   << std::setw(10) << format_ns_as_us(r.latency.p99_ns)
                   << std::setw(10) << format_ns_as_us(r.latency.max_ns)
                   << "   " << std::left << throughput_cell(r)
                   << "\n";
    }

    std::vector<BenchRow> rows_;
};

// ---------------------------------------------------------------------------
// Minimal CLI parsing shared by every bench_*.cpp
// ---------------------------------------------------------------------------

struct CliOptions {
    std::string markdown_path;   // empty => console-only
    bool has_markdown{false};
};

inline CliOptions parse_cli(int argc, char** argv) {
    CliOptions opts;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        constexpr const char* kPrefix = "--markdown=";
        if (arg.rfind(kPrefix, 0) == 0) {
            opts.markdown_path = arg.substr(std::string(kPrefix).size());
            opts.has_markdown = true;
        } else if (arg == "--markdown" && i + 1 < argc) {
            opts.markdown_path = argv[++i];
            opts.has_markdown = true;
        }
    }
    return opts;
}

} // namespace ember::bench
