// Serialization Engine Benchmarks
//
// Benchmarks ember::serialization::Serializer::pack() (structured payload ->
// framed, Fletcher-16 checksummed binary packet) plus the receive-side path:
// byte scanning, checksum verification and unpacking.
//
// NOTE: Serializer currently only exposes pack(). To benchmark the
// symmetric decode path we implement a small `unpack_and_verify()` helper
// below that mirrors pack()'s wire format exactly (magic "EM" + type + BE
// length + payload + BE Fletcher-16 checksum). If/when Serializer grows a
// real unpack() API, swap the call in bench_unpack() for it — the
// measurement harness and reported numbers stay valid either way.
//
// Run: bench_serialization[.exe] [--markdown=path/to/file.md]

#include "bench_framework.hpp"
#include "serialization/checksum.hpp"
#include "serialization/endian.hpp"
#include "serialization/serializer.hpp"

#include <array>
#include <cstring>
#include <optional>
#include <random>
#include <span>
#include <vector>

using namespace ember::bench;
using ember::serialization::calculate_fletcher16;
using ember::serialization::read_u16_be;
using ember::serialization::Serializer;

namespace {

constexpr std::size_t kIterations = 50'000;
constexpr std::size_t kWarmup = 2'000;

// A representative structured telemetry message, packed as the payload.
struct TelemetryMessage {
    std::uint32_t sensor_id;
    float temperature_c;
    float voltage;
    std::uint64_t timestamp_us;
};

struct UnpackResult {
    bool checksum_ok{false};
    std::uint8_t msg_type{0};
    std::span<const std::uint8_t> payload;
};

// Mirrors Serializer::pack()'s framing (see file header note above).
UnpackResult unpack_and_verify(std::span<const std::uint8_t> packet) {
    UnpackResult result;
    if (packet.size() < 7 || packet[0] != 'E' || packet[1] != 'M') {
        return result;
    }

    result.msg_type = packet[2];
    const std::uint16_t len = read_u16_be(&packet[3]);
    if (packet.size() != 5u + len + 2u) {
        return result;
    }

    const auto check_region = packet.subspan(0, 5 + len);
    const std::uint16_t expected_checksum = calculate_fletcher16(check_region);
    const std::uint16_t actual_checksum = read_u16_be(&packet[5 + len]);

    result.checksum_ok = (expected_checksum == actual_checksum);
    result.payload = packet.subspan(5, len);
    return result;
}

std::vector<std::uint8_t> make_payload(std::size_t size) {
    std::vector<std::uint8_t> payload(size);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& b : payload) b = static_cast<std::uint8_t>(dist(rng));
    return payload;
}

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

// Prevents the optimizer from eliding pack() entirely; cheap no-op touch.
void asm_volatile_hint(const std::vector<std::uint8_t>& packet) {
    volatile std::uint8_t sink = packet.empty() ? 0 : packet[0];
    (void)sink;
}

// ---------------------------------------------------------------------------
// pack()
// ---------------------------------------------------------------------------

BenchRow bench_pack(std::size_t payload_size) {
    const std::vector<std::uint8_t> payload = make_payload(payload_size);

    double elapsed_ns = 0.0;
    LatencyStats stats;
    elapsed_ns = time_wall_ns([&] {
        stats = time_each_call(kIterations, kWarmup, [&] {
            auto packet = Serializer::pack(0x01, payload);
            asm_volatile_hint(packet);
        });
    });

    const double bytes_per_sec = (static_cast<double>(payload_size) * kIterations) / (elapsed_ns / 1e9);
    return make_row("Serialization Engine", "pack() " + std::to_string(payload_size) + "B payload",
                     stats, bytes_per_sec, " bytes/sec (payload)",
                     std::to_string(kIterations) + " iters");
}

// ---------------------------------------------------------------------------
// unpack + checksum verify
// ---------------------------------------------------------------------------

BenchRow bench_unpack(std::size_t payload_size) {
    const std::vector<std::uint8_t> payload = make_payload(payload_size);
    const std::vector<std::uint8_t> packet = Serializer::pack(0x01, payload);

    double elapsed_ns = 0.0;
    LatencyStats stats;
    bool sanity_ok = true;
    elapsed_ns = time_wall_ns([&] {
        stats = time_each_call(kIterations, kWarmup, [&] {
            UnpackResult r = unpack_and_verify(packet);
            sanity_ok &= r.checksum_ok;
        });
    });

    if (!sanity_ok) {
        std::cerr << "warning: unpack_and_verify() reported a checksum failure during benchmarking\n";
    }

    const double bytes_per_sec = (static_cast<double>(packet.size()) * kIterations) / (elapsed_ns / 1e9);
    return make_row("Serialization Engine", "unpack+verify() " + std::to_string(payload_size) + "B payload",
                     stats, bytes_per_sec, " bytes/sec (wire)",
                     std::to_string(kIterations) + " iters");
}

// ---------------------------------------------------------------------------
// Raw Fletcher-16 checksum throughput (isolated from framing overhead)
// ---------------------------------------------------------------------------

BenchRow bench_checksum(std::size_t buffer_size) {
    const std::vector<std::uint8_t> buffer = make_payload(buffer_size);

    double elapsed_ns = 0.0;
    LatencyStats stats;
    elapsed_ns = time_wall_ns([&] {
        stats = time_each_call(kIterations, kWarmup, [&] {
            volatile std::uint16_t checksum = calculate_fletcher16(buffer);
            (void)checksum;
        });
    });

    const double bytes_per_sec = (static_cast<double>(buffer_size) * kIterations) / (elapsed_ns / 1e9);
    return make_row("Serialization Engine", "calculate_fletcher16() " + std::to_string(buffer_size) + "B",
                     stats, bytes_per_sec, " bytes/sec", std::to_string(kIterations) + " iters");
}

} // namespace

int main(int argc, char** argv) {
    CliOptions cli = parse_cli(argc, argv);

    Reporter reporter;
    std::cout << "Running serialization benchmarks (" << kIterations << " iterations per case)...\n";

    for (std::size_t size : {16u, 64u, 256u, 1024u}) {
        reporter.add(bench_pack(size));
    }
    for (std::size_t size : {16u, 64u, 256u, 1024u}) {
        reporter.add(bench_unpack(size));
    }
    for (std::size_t size : {1024u, 4096u, 65536u}) {
        reporter.add(bench_checksum(size));
    }

    reporter.print_console("EMBER Serialization Engine Benchmarks");

    if (cli.has_markdown) {
        reporter.write_markdown(cli.markdown_path, "Serialization Engine Benchmarks");
    }

    return 0;
}
