#include "domain/shared/algorithm_orchestrator.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <sstream>
#include <vector>

namespace cpu_lab::domain::shared
{

    bool AlgorithmOrchestrator::validate_config(
        const AlgorithmOrchestratorConfig &config) noexcept
    {
        if (config.benchmark_name.empty()) {
            return false;
        }

        if (config.problem_size == 0U) {
            return false;
        }

        if (config.run_counts.empty()) {
            return false;
        }

        for (const std::size_t run_count : config.run_counts) {
            if (run_count == 0U) {
                return false;
            }
        }

        return true;
    }

    std::vector<double> AlgorithmOrchestrator::to_ns_per_run(
        const std::vector<infrastructure::timing::TimingSample> &samples)
    {
        std::vector<double> per_run{};
        per_run.reserve(samples.size());

        for (const auto &sample : samples) {
            if (sample.run_count == 0U) {
                continue;
            }
            per_run.push_back(sample.elapsed_ns / static_cast<double>(sample.run_count));
        }

        return per_run;
    }

    double AlgorithmOrchestrator::compute_mean(const std::vector<double> &values)
    {
        if (values.empty()) {
            return 0.0;
        }

        const double sum = std::accumulate(values.begin(), values.end(), 0.0);
        return sum / static_cast<double>(values.size());
    }

    double AlgorithmOrchestrator::compute_median(std::vector<double> values)
    {
        if (values.empty()) {
            return 0.0;
        }

        std::sort(values.begin(), values.end());
        const std::size_t middle = values.size() / 2U;
        if ((values.size() % 2U) == 0U) {
            return (values[middle - 1U] + values[middle]) / 2.0;
        }
        return values[middle];
    }

    double AlgorithmOrchestrator::compute_stddev(
        const std::vector<double> &values,
        const double mean)
    {
        if (values.empty()) {
            return 0.0;
        }

        double squared_error_sum = 0.0;
        for (const double value : values) {
            const double delta = value - mean;
            squared_error_sum += delta * delta;
        }

        return std::sqrt(squared_error_sum / static_cast<double>(values.size()));
    }

    std::string AlgorithmOrchestrator::compose_notes(
        const std::string &notes,
        const std::uint64_t digest)
    {
        std::ostringstream builder{};
        if (!notes.empty()) {
            builder << notes << "; ";
        }
        builder << "digest=" << digest;
        return builder.str();
    }

} // namespace cpu_lab::domain::shared