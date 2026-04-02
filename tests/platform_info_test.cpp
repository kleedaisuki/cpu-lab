#include "infrastructure/system/platform_info.hpp"

#include <algorithm>
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

    const auto has_cuda_status = std::any_of(
        kv.begin(),
        kv.end(),
        [](const auto &entry)
        {
            return entry.first == "cuda.status";
        });
    assert(has_cuda_status);

    const std::string summary = PlatformInfo::to_summary(snapshot);
    assert(!summary.empty());
    assert(summary.find("cuda.status=") != std::string::npos);

    if (snapshot.cuda_matrix_dot_build_enabled)
    {
        assert(snapshot.cuda_runtime_available || snapshot.cuda_status.find("fallback") != std::string::npos);
    }
    else
    {
        assert(!snapshot.cuda_runtime_available);
        assert(!snapshot.cuda_device_available);
        assert(snapshot.cuda_device_count == 0U);
    }

    return 0;
}
