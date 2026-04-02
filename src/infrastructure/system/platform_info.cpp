#include "infrastructure/system/platform_info.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <limits>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <thread>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__) || defined(__unix__)
#include <sys/utsname.h>
#include <unistd.h>
#endif

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#if defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#endif
#endif

#if defined(CPU_LAB_ENABLE_CUDA_MATRIX_DOT)
extern "C" int cudaRuntimeGetVersion(int *runtimeVersion);
extern "C" int cudaDriverGetVersion(int *driverVersion);
extern "C" int cudaGetDeviceCount(int *count);
#endif

namespace cpu_lab::infrastructure::system
{
    namespace
    {
        /** @brief 默认缓存行大小（default cache line）；Conservative cache line fallback bytes. */
        constexpr std::size_t kDefaultCacheLineBytes = 64U;

        /** @brief 默认 L1 数据缓存（default L1D）；Conservative L1 data cache fallback bytes. */
        constexpr std::size_t kDefaultL1DataCacheBytes = 32U * 1024U;

#if defined(CPU_LAB_ENABLE_CUDA_MATRIX_DOT)
        /** @brief CUDA 成功码（CUDA success code）；Numeric value of cudaSuccess. */
        constexpr int kCudaSuccess = 0;
#endif

        /**
         * @brief CUDA 探测结果（CUDA probe result）；Internal CUDA runtime probe payload.
         */
        struct CudaProbeResult final
        {
            /** @brief 构建启用（build enabled）；Whether CUDA policy is compiled. */
            bool build_enabled{false};

            /** @brief Runtime 可达（runtime reachable）；Whether CUDA Runtime API probe succeeded. */
            bool runtime_available{false};

            /** @brief 设备可见（device visible）；Whether at least one CUDA device is available. */
            bool device_available{false};

            /** @brief 设备数量（device count）；Visible CUDA device count. */
            std::size_t device_count{0U};

            /** @brief Runtime 版本（runtime version）；Formatted CUDA Runtime version text. */
            std::string runtime_version{"n/a"};

            /** @brief Driver 版本（driver version）；Formatted CUDA Driver version text. */
            std::string driver_version{"n/a"};

            /** @brief 状态摘要（status）；Human-readable status with fallback rationale. */
            std::string status{"not probed"};
        };

        /**
         * @brief 布尔值转文本（bool to text）；Convert bool to stable lowercase text.
         * @param value 布尔值（boolean value）。
         * @return 文本（text），"true" 或 "false"。
         */
        [[nodiscard]] std::string bool_to_text(const bool value)
        {
            return value ? "true" : "false";
        }

        /**
         * @brief 格式化 CUDA 版本号（format CUDA version）；Convert packed CUDA version integer to semantic string.
         * @param packed_version 打包版本（packed version），例如 12040。
         * @return 语义版本文本（semantic version text）。
         */
        [[nodiscard]] std::string format_cuda_version(const int packed_version)
        {
            if (packed_version <= 0)
            {
                return "n/a";
            }

            const int major = packed_version / 1000;
            const int minor = (packed_version % 1000) / 10;
            return std::to_string(major) + "." + std::to_string(minor);
        }

        /**
         * @brief 探测 CUDA 状态（probe CUDA）；Collect CUDA build/runtime/device status with fallback reason.
         * @return CUDA 探测结果（CUDA probe result）。
         */
        [[nodiscard]] CudaProbeResult probe_cuda_support()
        {
            CudaProbeResult probe{};

#if defined(CPU_LAB_ENABLE_CUDA_MATRIX_DOT)
            probe.build_enabled = true;

            int runtime_version = 0;
            const int runtime_status = cudaRuntimeGetVersion(&runtime_version);
            if (runtime_status != kCudaSuccess)
            {
                probe.status =
                    "CUDA runtime API is unreachable (cudaRuntimeGetVersion failed); fallback to CPU policies.";
                return probe;
            }

            probe.runtime_available = true;
            probe.runtime_version = format_cuda_version(runtime_version);

            int driver_version = 0;
            const int driver_status = cudaDriverGetVersion(&driver_version);
            if (driver_status == kCudaSuccess)
            {
                probe.driver_version = format_cuda_version(driver_version);
            }

            int device_count = 0;
            const int device_status = cudaGetDeviceCount(&device_count);
            if (device_status != kCudaSuccess)
            {
                probe.status =
                    "CUDA runtime is present but device enumeration failed (cudaGetDeviceCount); fallback to CPU policies.";
                return probe;
            }

            if (device_count < 0)
            {
                device_count = 0;
            }

            probe.device_count = static_cast<std::size_t>(device_count);
            probe.device_available = (probe.device_count > 0U);

            if (probe.device_available)
            {
                probe.status = "CUDA runtime ready with visible device(s).";
            }
            else
            {
                probe.status = "CUDA runtime reachable but no visible CUDA device; fallback to CPU policies.";
            }
#else
            probe.status = "matrix_dot_cuda policy is not built; fallback to CPU policies.";
#endif

            return probe;
        }

        /**
         * @brief 清理首尾空白（trim）；Trim leading/trailing ASCII spaces.
         * @param text 输入文本（input text）。
         * @return 清理后文本（trimmed text）。
         */
        [[nodiscard]] std::string trim_ascii(std::string text)
        {
            const auto not_space = [](const unsigned char ch)
            {
                return !std::isspace(ch);
            };

            auto begin = std::find_if(text.begin(), text.end(), not_space);
            auto end = std::find_if(text.rbegin(), text.rend(), not_space).base();

            if (begin >= end)
            {
                return {};
            }

            return std::string(begin, end);
        }

        /**
         * @brief 检测操作系统名称（OS name）；Detect OS family from compile-time macros.
         * @return 操作系统名称（OS family name）。
         */
        [[nodiscard]] std::string detect_os_name()
        {
#if defined(_WIN32)
            return "Windows";
#elif defined(__APPLE__)
            return "macOS";
#elif defined(__linux__)
            return "Linux";
#elif defined(__unix__)
            return "Unix";
#else
            return "Unknown";
#endif
        }

        /**
         * @brief 检测 CPU 架构（CPU architecture）；Detect architecture from macros.
         * @return 架构字符串（architecture label）。
         */
        [[nodiscard]] std::string detect_architecture()
        {
#if defined(__x86_64__) || defined(_M_X64)
            return "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
            return "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
            return "arm64";
#elif defined(__arm__) || defined(_M_ARM)
            return "arm";
#elif defined(__riscv)
            return "riscv";
#else
            return "unknown";
#endif
        }

        /**
         * @brief 获取编译器标识（compiler id）；Build compiler family/version string.
         * @return 编译器文本（compiler text）。
         */
        [[nodiscard]] std::string detect_compiler_id()
        {
#if defined(__clang__)
            return "Clang " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__)
            return "GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
            return "MSVC " + std::to_string(_MSC_VER);
#else
            return "Unknown";
#endif
        }

        /**
         * @brief 获取 C++ 标准标识（C++ standard）；Serialize __cplusplus value.
         * @return C++ 标准文本（C++ standard string）。
         */
        [[nodiscard]] std::string detect_cpp_standard()
        {
            return std::to_string(static_cast<long long>(__cplusplus));
        }

        /**
         * @brief 获取逻辑核心数（logical cores）；Detect online hardware concurrency.
         * @return 逻辑核心数（logical core count）。
         */
        [[nodiscard]] std::size_t detect_logical_cores() noexcept
        {
            const unsigned int value = std::thread::hardware_concurrency();
            return (value == 0U) ? 1U : static_cast<std::size_t>(value);
        }

        /**
         * @brief 获取页大小（page size）；Detect memory page size.
         * @return 页大小字节（page bytes），未知时返回 0。
         */
        [[nodiscard]] std::size_t detect_page_size_bytes() noexcept
        {
#if defined(_WIN32)
            SYSTEM_INFO info{};
            GetSystemInfo(&info);
            return static_cast<std::size_t>(info.dwPageSize);
#elif defined(_SC_PAGESIZE)
            const long value = ::sysconf(_SC_PAGESIZE);
            return (value > 0L) ? static_cast<std::size_t>(value) : 0U;
#else
            return 0U;
#endif
        }

        /**
         * @brief 获取缓存行大小（cache line）；Detect cache line bytes when available.
         * @return 缓存行字节数（cache line bytes），未知时返回 0。
         */
        [[nodiscard]] std::size_t detect_cache_line_bytes() noexcept
        {
#if defined(__cpp_lib_hardware_interference_size)
            if (std::hardware_destructive_interference_size > 0U)
            {
                return std::hardware_destructive_interference_size;
            }
#endif
#if defined(_SC_LEVEL1_DCACHE_LINESIZE)
            const long value = ::sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
            if (value > 0L)
            {
                return static_cast<std::size_t>(value);
            }
#endif
            return 0U;
        }

        /**
         * @brief 获取 L1 数据缓存大小（L1D cache）；Detect L1D size when available.
         * @return L1D 字节数（L1D bytes），未知时返回 0。
         */
        [[nodiscard]] std::size_t detect_l1_data_cache_bytes() noexcept
        {
#if defined(_SC_LEVEL1_DCACHE_SIZE)
            const long value = ::sysconf(_SC_LEVEL1_DCACHE_SIZE);
            if (value > 0L)
            {
                return static_cast<std::size_t>(value);
            }
#endif
            return 0U;
        }

        /**
         * @brief 获取物理内存大小（physical memory）；Detect total physical memory bytes.
         * @return 物理内存字节（physical memory bytes），未知时返回 0。
         */
        [[nodiscard]] std::uint64_t detect_total_physical_memory_bytes() noexcept
        {
#if defined(_WIN32)
            MEMORYSTATUSEX status{};
            status.dwLength = sizeof(status);
            if (GlobalMemoryStatusEx(&status) == 0)
            {
                return 0ULL;
            }
            return static_cast<std::uint64_t>(status.ullTotalPhys);
#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
            const long pages = ::sysconf(_SC_PHYS_PAGES);
            const long page_size = ::sysconf(_SC_PAGESIZE);
            if (pages <= 0L || page_size <= 0L)
            {
                return 0ULL;
            }

            const auto product = static_cast<unsigned long long>(pages) * static_cast<unsigned long long>(page_size);
            return static_cast<std::uint64_t>(product);
#else
            return 0ULL;
#endif
        }

        /**
         * @brief 获取操作系统版本（OS version）；Detect OS version text.
         * @return 版本文本（version string）。
         */
        [[nodiscard]] std::string detect_os_version()
        {
#if defined(_WIN32)
            OSVERSIONINFOW version{};
            version.dwOSVersionInfoSize = sizeof(version);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            const BOOL ok = ::GetVersionExW(&version);
#pragma GCC diagnostic pop

            if (ok != 0)
            {
                return std::to_string(static_cast<unsigned int>(version.dwMajorVersion)) +
                       "." + std::to_string(static_cast<unsigned int>(version.dwMinorVersion)) +
                       "." + std::to_string(static_cast<unsigned int>(version.dwBuildNumber));
            }

            return "unknown";
#elif defined(__linux__) || defined(__APPLE__) || defined(__unix__)
            struct utsname name_info{};
            if (::uname(&name_info) == 0)
            {
                std::string value = name_info.release;
                value = trim_ascii(std::move(value));
                return value.empty() ? std::string{"unknown"} : value;
            }
            return "unknown";
#else
            return "unknown";
#endif
        }

        /**
         * @brief 读取 CPU 品牌字符串（CPU brand）；Best-effort x86 CPUID brand extraction.
         * @return CPU 品牌文本（CPU brand string）。
         */
        [[nodiscard]] std::string detect_cpu_brand()
        {
#if (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)) && (defined(__GNUC__) || defined(__clang__))
            unsigned int max_extended = __get_cpuid_max(0x80000000U, nullptr);
            if (max_extended < 0x80000004U)
            {
                return "unknown";
            }

            std::array<unsigned int, 12U> cpu_data{};
            for (unsigned int leaf = 0U; leaf < 3U; ++leaf)
            {
                unsigned int eax = 0U;
                unsigned int ebx = 0U;
                unsigned int ecx = 0U;
                unsigned int edx = 0U;
                if (__get_cpuid(0x80000002U + leaf, &eax, &ebx, &ecx, &edx) == 0)
                {
                    return "unknown";
                }
                cpu_data[(leaf * 4U) + 0U] = eax;
                cpu_data[(leaf * 4U) + 1U] = ebx;
                cpu_data[(leaf * 4U) + 2U] = ecx;
                cpu_data[(leaf * 4U) + 3U] = edx;
            }

            std::string brand(reinterpret_cast<const char *>(cpu_data.data()), cpu_data.size() * sizeof(unsigned int));
            brand = trim_ascii(std::move(brand));
            return brand.empty() ? std::string{"unknown"} : brand;
#else
            return "unknown";
#endif
        }

        /**
         * @brief 字节数格式化（byte formatting）；Convert byte count to readable IEC string.
         * @param bytes 字节数量（byte count）。
         * @return 格式化文本（formatted text）。
         */
        [[nodiscard]] std::string format_bytes(const std::uint64_t bytes)
        {
            static constexpr std::array<const char *, 5U> units{"B", "KiB", "MiB", "GiB", "TiB"};

            double value = static_cast<double>(bytes);
            std::size_t unit_index = 0U;

            while (value >= 1024.0 && (unit_index + 1U) < units.size())
            {
                value /= 1024.0;
                ++unit_index;
            }

            std::ostringstream stream{};
            stream.setf(std::ios::fixed);
            stream.precision((unit_index == 0U) ? 0 : 2);
            stream << value << ' ' << units[unit_index];
            return stream.str();
        }
    } // namespace

    PlatformSnapshot PlatformInfo::collect()
    {
        PlatformSnapshot snapshot{};
        snapshot.os_name = detect_os_name();
        snapshot.os_version = detect_os_version();
        snapshot.architecture = detect_architecture();
        snapshot.cpu_brand = detect_cpu_brand();
        snapshot.compiler_id = detect_compiler_id();
        snapshot.cpp_standard = detect_cpp_standard();
        snapshot.logical_cpu_cores = detect_logical_cores();
        snapshot.pointer_width_bits = sizeof(void *) * 8U;
        snapshot.page_size_bytes = detect_page_size_bytes();
        snapshot.cache_line_bytes = detect_cache_line_bytes();
        snapshot.l1_data_cache_bytes = detect_l1_data_cache_bytes();
        snapshot.total_physical_memory_bytes = detect_total_physical_memory_bytes();

        const CudaProbeResult cuda_probe = probe_cuda_support();
        snapshot.cuda_matrix_dot_build_enabled = cuda_probe.build_enabled;
        snapshot.cuda_runtime_available = cuda_probe.runtime_available;
        snapshot.cuda_device_available = cuda_probe.device_available;
        snapshot.cuda_device_count = cuda_probe.device_count;
        snapshot.cuda_runtime_version = std::move(cuda_probe.runtime_version);
        snapshot.cuda_driver_version = std::move(cuda_probe.driver_version);
        snapshot.cuda_status = std::move(cuda_probe.status);

        return snapshot;
    }

    std::size_t PlatformInfo::recommend_parallel_workers(
        const PlatformSnapshot &snapshot,
        const std::size_t reserve_cores) noexcept
    {
        const std::size_t cores = (snapshot.logical_cpu_cores == 0U) ? 1U : snapshot.logical_cpu_cores;
        if (cores <= reserve_cores)
        {
            return 1U;
        }
        return cores - reserve_cores;
    }

    std::size_t PlatformInfo::recommend_streaming_chunk_elements(
        const PlatformSnapshot &snapshot,
        const std::size_t element_size_bytes,
        const std::size_t concurrent_arrays)
    {
        if (element_size_bytes == 0U)
        {
            throw std::invalid_argument("recommend_streaming_chunk_elements requires element_size_bytes > 0.");
        }

        const std::size_t active_arrays = std::max<std::size_t>(1U, concurrent_arrays);

        const std::size_t l1_bytes = (snapshot.l1_data_cache_bytes == 0U)
                                         ? kDefaultL1DataCacheBytes
                                         : snapshot.l1_data_cache_bytes;

        const std::size_t cache_line_bytes = (snapshot.cache_line_bytes == 0U)
                                                 ? kDefaultCacheLineBytes
                                                 : snapshot.cache_line_bytes;

        const std::size_t effective_budget = (l1_bytes * 3U) / 4U;
        const std::size_t denominator = active_arrays * element_size_bytes;

        if (denominator == 0U)
        {
            return 1U;
        }

        std::size_t raw_chunk = effective_budget / denominator;
        if (raw_chunk == 0U)
        {
            raw_chunk = 1U;
        }

        const std::size_t alignment = std::max<std::size_t>(1U, cache_line_bytes / element_size_bytes);
        const std::size_t aligned_chunk = (raw_chunk / alignment) * alignment;

        if (aligned_chunk == 0U)
        {
            return alignment;
        }

        return aligned_chunk;
    }

    std::vector<std::pair<std::string, std::string>> PlatformInfo::to_key_value_pairs(
        const PlatformSnapshot &snapshot)
    {
        std::vector<std::pair<std::string, std::string>> pairs{};
        pairs.reserve(19U);

        pairs.emplace_back("os.name", snapshot.os_name);
        pairs.emplace_back("os.version", snapshot.os_version);
        pairs.emplace_back("cpu.architecture", snapshot.architecture);
        pairs.emplace_back("cpu.brand", snapshot.cpu_brand);
        pairs.emplace_back("build.compiler", snapshot.compiler_id);
        pairs.emplace_back("build.cpp_standard", snapshot.cpp_standard);
        pairs.emplace_back("cpu.logical_cores", std::to_string(snapshot.logical_cpu_cores));
        pairs.emplace_back("cpu.pointer_bits", std::to_string(snapshot.pointer_width_bits));
        pairs.emplace_back("memory.page_size_bytes", std::to_string(snapshot.page_size_bytes));
        pairs.emplace_back("memory.cache_line_bytes", std::to_string(snapshot.cache_line_bytes));
        pairs.emplace_back("memory.l1_data_cache_bytes", std::to_string(snapshot.l1_data_cache_bytes));
        pairs.emplace_back("memory.total_physical", format_bytes(snapshot.total_physical_memory_bytes));
        pairs.emplace_back("cuda.matrix_dot_build_enabled", bool_to_text(snapshot.cuda_matrix_dot_build_enabled));
        pairs.emplace_back("cuda.runtime_available", bool_to_text(snapshot.cuda_runtime_available));
        pairs.emplace_back("cuda.device_available", bool_to_text(snapshot.cuda_device_available));
        pairs.emplace_back("cuda.device_count", std::to_string(snapshot.cuda_device_count));
        pairs.emplace_back("cuda.runtime_version", snapshot.cuda_runtime_version);
        pairs.emplace_back("cuda.driver_version", snapshot.cuda_driver_version);
        pairs.emplace_back("cuda.status", snapshot.cuda_status);

        return pairs;
    }

    std::string PlatformInfo::to_summary(const PlatformSnapshot &snapshot)
    {
        const auto pairs = to_key_value_pairs(snapshot);
        std::ostringstream stream{};

        for (const auto &entry : pairs)
        {
            stream << entry.first << "=" << entry.second << '\n';
        }

        return stream.str();
    }

} // namespace cpu_lab::infrastructure::system
