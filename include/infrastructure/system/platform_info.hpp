#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace cpu_lab::infrastructure::system
{

    /**
     * @brief 平台信息快照（platform snapshot）；Runtime and build-time platform profile.
     */
    struct PlatformSnapshot
    {
        /** @brief 操作系统名称（OS name）；Detected operating system family. */
        std::string os_name{};

        /** @brief 操作系统版本（OS version）；Detected operating system version text. */
        std::string os_version{};

        /** @brief CPU 架构（CPU architecture）；Detected architecture label. */
        std::string architecture{};

        /** @brief CPU 型号（CPU brand）；Best-effort CPU brand/model string. */
        std::string cpu_brand{};

        /** @brief 编译器标识（compiler id）；Compiler family and version. */
        std::string compiler_id{};

        /** @brief C++ 标准（C++ standard）；Value of __cplusplus used for this build. */
        std::string cpp_standard{};

        /** @brief 逻辑核心数（logical cores）；Online hardware thread count. */
        std::size_t logical_cpu_cores{0U};

        /** @brief 指针位宽（pointer width）；Pointer width in bits. */
        std::size_t pointer_width_bits{0U};

        /** @brief 页大小（page size）；Memory page size in bytes. */
        std::size_t page_size_bytes{0U};

        /** @brief 缓存行大小（cache line size）；Cache line size in bytes if available. */
        std::size_t cache_line_bytes{0U};

        /** @brief L1 数据缓存大小（L1 data cache）；L1D cache size in bytes if available. */
        std::size_t l1_data_cache_bytes{0U};

        /** @brief 物理内存容量（physical memory）；Total physical memory in bytes if available. */
        std::uint64_t total_physical_memory_bytes{0ULL};

        /** @brief CUDA 构建开关（CUDA build flag）；Whether matrix-dot CUDA policy is compiled in. */
        bool cuda_matrix_dot_build_enabled{false};

        /** @brief CUDA Runtime 可达性（CUDA runtime reachable）；Whether CUDA Runtime API probe succeeded. */
        bool cuda_runtime_available{false};

        /** @brief CUDA 设备可用性（CUDA device available）；Whether at least one CUDA device is visible. */
        bool cuda_device_available{false};

        /** @brief CUDA 可见设备数（CUDA device count）；Number of visible CUDA devices, if probe succeeded. */
        std::size_t cuda_device_count{0U};

        /** @brief CUDA Runtime 版本（CUDA runtime version）；Semantic version if known, otherwise fallback text. */
        std::string cuda_runtime_version{"n/a"};

        /** @brief CUDA Driver 版本（CUDA driver version）；Semantic version if known, otherwise fallback text. */
        std::string cuda_driver_version{"n/a"};

        /** @brief CUDA 状态摘要（CUDA status）；Human-readable probe result and fallback reason. */
        std::string cuda_status{"not probed"};
    };

    /**
     * @brief 平台信息工具（platform info utilities）；Collect and derive platform-aware hints.
     */
    class PlatformInfo final
    {
    public:
        /**
         * @brief 采集平台快照（collect snapshot）；Collect runtime/build platform information.
         * @return 平台快照（platform snapshot）。
         */
        [[nodiscard]] static PlatformSnapshot collect();

        /**
         * @brief 推荐并行工作线程数（recommended workers）；Compute practical worker count.
         * @param snapshot 平台快照（platform snapshot）。
         * @param reserve_cores 预留核心数（reserved cores）用于系统响应。
         * @return 建议线程数（recommended worker threads）。
         */
        [[nodiscard]] static std::size_t recommend_parallel_workers(
            const PlatformSnapshot &snapshot,
            std::size_t reserve_cores = 1U) noexcept;

        /**
         * @brief 推荐流式分块元素数（streaming chunk elements）；Estimate cache-friendly chunk length.
         * @param snapshot 平台快照（platform snapshot）。
         * @param element_size_bytes 单元素大小（element size in bytes），需大于 0。
         * @param concurrent_arrays 同时驻留数组数（concurrent arrays），例如矩阵列+向量约为 2。
         * @return 建议分块元素数（recommended elements per chunk）。
         * @note 若缓存参数未知，使用保守默认值（conservative fallback）。
         */
        [[nodiscard]] static std::size_t recommend_streaming_chunk_elements(
            const PlatformSnapshot &snapshot,
            std::size_t element_size_bytes,
            std::size_t concurrent_arrays = 2U);

        /**
         * @brief 转换为键值对（key-value pairs）；Export snapshot as display-friendly pairs.
         * @param snapshot 平台快照（platform snapshot）。
         * @return 键值对数组（ordered key-value pairs）。
         */
        [[nodiscard]] static std::vector<std::pair<std::string, std::string>> to_key_value_pairs(
            const PlatformSnapshot &snapshot);

        /**
         * @brief 转换为摘要文本（summary text）；Build a human-readable multiline summary.
         * @param snapshot 平台快照（platform snapshot）。
         * @return 摘要字符串（summary text）。
         */
        [[nodiscard]] static std::string to_summary(const PlatformSnapshot &snapshot);
    };

} // namespace cpu_lab::infrastructure::system
