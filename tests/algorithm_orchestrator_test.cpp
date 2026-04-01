#include "domain/shared/algorithm_orchestrator.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using cpu_lab::domain::shared::AlgorithmOrchestrator;
using cpu_lab::domain::shared::AlgorithmOrchestratorConfig;

namespace
{
    std::size_t g_construct_count = 0U;
    std::size_t g_destruct_count = 0U;
}

/**
 * @brief 测试策略实现（test policy implementation）；Simple deterministic policy for orchestrator tests.
 */
class DeterministicPolicy final
{
public:
    /**
     * @brief 构造函数（constructor）；Initialize policy state.
     * @param seed 初始种子（initial seed）。
     */
    explicit DeterministicPolicy(const std::uint64_t seed)
        : state_(seed)
    {
        ++g_construct_count;
    }

    /**
     * @brief 析构函数（destructor）；Record lifecycle teardown.
     */
    ~DeterministicPolicy()
    {
        ++g_destruct_count;
    }

    /**
     * @brief 获取算法名称（algorithm name）。
     * @return 算法名称（algorithm name）。
     */
    [[nodiscard]] std::string_view algorithm_name() const noexcept
    {
        return "deterministic_policy";
    }

    /**
     * @brief 执行一次算法（run once）；Perform one deterministic arithmetic workload.
     * @return 无返回值；No return value.
     */
    void run_once()
    {
        std::uint64_t local = state_;
        for (std::size_t i = 0U; i < 256U; ++i) {
            local ^= (local << 7U) + (i * 1315423911ULL);
            local += (i << 3U) ^ (local >> 11U);
        }
        state_ = local;
    }

    /**
     * @brief 获取输出指纹（output fingerprint）；Return latest digest.
     * @return 指纹值（digest value）。
     */
    [[nodiscard]] std::uint64_t output_fingerprint() const noexcept
    {
        return state_;
    }

private:
    /** @brief 内部状态（internal state）；Digest accumulator. */
    std::uint64_t state_{0x12345678ABCDEF00ULL};
};

int main()
{
    g_construct_count = 0U;
    g_destruct_count = 0U;

    AlgorithmOrchestrator orchestrator{};

    AlgorithmOrchestratorConfig config{};
    config.benchmark_name = "sum_reduce";
    config.problem_size = 4096U;
    config.run_counts = std::vector<std::size_t>{64U, 128U, 256U, 512U};
    config.warmup_rounds = 2U;
    config.notes = "policy_contract_test";

    const auto result = orchestrator.operator()<DeterministicPolicy>(
        config,
        0x12345678ABCDEF00ULL);

    assert(result.is_valid());
    assert(result.benchmark_name == config.benchmark_name);
    assert(result.algorithm == "deterministic_policy");
    assert(result.problem_size == config.problem_size);
    assert(result.sample_count == config.run_counts.size());
    assert(result.runs_per_sample == config.run_counts.front());
    assert(result.notes.find("digest=") != std::string::npos);

    assert(g_construct_count == 1U);
    assert(g_destruct_count == 1U);

    AlgorithmOrchestratorConfig invalid = config;
    invalid.run_counts.clear();

    bool threw = false;
    try {
        (void)orchestrator.operator()<DeterministicPolicy>(
            invalid,
            0xABCDEF1234567890ULL);
    }
    catch (const std::invalid_argument &) {
        threw = true;
    }

    assert(threw);
    return 0;
}