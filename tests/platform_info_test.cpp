#include "infrastructure/system/platform_info.hpp"

#include <cassert>
#include <cstddef>
#include <string>

using cpu_lab::infrastructure::system::PlatformInfo;
using cpu_lab::infrastructure::system::PlatformSnapshot;

int main()
{
    const PlatformSnapshot snapshot = PlatformInfo::collect();

    assert(!snapshot.os_name.empty());
    assert(!snapshot.os_version.empty());
    assert(!snapshot.architecture.empty());
    assert(!snapshot.compiler_id.empty());
    assert(!snapshot.cpp_standard.empty());

    assert(snapshot.logical_cpu_cores >= 1U);
    assert(snapshot.pointer_width_bits == (sizeof(void *) * 8U));

    const std::size_t workers = PlatformInfo::recommend_parallel_workers(snapshot, 1U);
    assert(workers >= 1U);
    assert(workers <= snapshot.logical_cpu_cores);

    const std::size_t chunk_elements = PlatformInfo::recommend_streaming_chunk_elements(snapshot, sizeof(double), 2U);
    assert(chunk_elements >= 1U);

    const auto kv = PlatformInfo::to_key_value_pairs(snapshot);
    assert(!kv.empty());
    assert(kv.front().first == "os.name");

    const std::string summary = PlatformInfo::to_summary(snapshot);
    assert(!summary.empty());

    return 0;
}
