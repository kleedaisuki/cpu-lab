#include "domain/shared/benchmark_result.hpp"

#include "infrastructure/csv/row.hpp"

#include <cmath>

namespace cpu_lab::domain::shared
{

    auto BenchmarkResult::meta() noexcept
        -> std::tuple<
            infrastructure::csv::RowField<BenchmarkResult, std::string>,
            infrastructure::csv::RowField<BenchmarkResult, std::string>,
            infrastructure::csv::RowField<BenchmarkResult, std::size_t>,
            infrastructure::csv::RowField<BenchmarkResult, std::size_t>,
            infrastructure::csv::RowField<BenchmarkResult, std::size_t>,
            infrastructure::csv::RowField<BenchmarkResult, double>,
            infrastructure::csv::RowField<BenchmarkResult, double>,
            infrastructure::csv::RowField<BenchmarkResult, double>,
            infrastructure::csv::RowField<BenchmarkResult, double>,
            infrastructure::csv::RowField<BenchmarkResult, double>,
            infrastructure::csv::RowField<BenchmarkResult, double>,
            infrastructure::csv::RowField<BenchmarkResult, double>,
            infrastructure::csv::RowField<BenchmarkResult, std::string>>
    {
        using infrastructure::csv::make_row_field;

        return std::make_tuple(
            make_row_field("benchmark_name", &BenchmarkResult::benchmark_name),
            make_row_field("algorithm", &BenchmarkResult::algorithm),
            make_row_field("problem_size", &BenchmarkResult::problem_size),
            make_row_field("runs_per_sample", &BenchmarkResult::runs_per_sample),
            make_row_field("sample_count", &BenchmarkResult::sample_count),
            make_row_field("best_ns_per_run", &BenchmarkResult::best_ns_per_run),
            make_row_field("mean_ns_per_run", &BenchmarkResult::mean_ns_per_run),
            make_row_field("median_ns_per_run", &BenchmarkResult::median_ns_per_run),
            make_row_field("stddev_ns", &BenchmarkResult::stddev_ns),
            make_row_field("fit_slope_ns_per_run", &BenchmarkResult::fit_slope_ns_per_run),
            make_row_field("fit_intercept_ns", &BenchmarkResult::fit_intercept_ns),
            make_row_field("fit_r_squared", &BenchmarkResult::fit_r_squared),
            make_row_field("notes", &BenchmarkResult::notes));
    }

    bool BenchmarkResult::is_valid() const noexcept
    {
        const auto finite_non_negative = [](const double value) noexcept -> bool
        {
            return std::isfinite(value) && (value >= 0.0);
        };

        if (benchmark_name.empty() || algorithm.empty()) {
            return false;
        }

        if (problem_size == 0U || runs_per_sample == 0U || sample_count == 0U) {
            return false;
        }

        if (!finite_non_negative(best_ns_per_run) ||
            !finite_non_negative(mean_ns_per_run) ||
            !finite_non_negative(median_ns_per_run) ||
            !finite_non_negative(stddev_ns) ||
            !finite_non_negative(fit_slope_ns_per_run) ||
            !finite_non_negative(fit_intercept_ns)) {
            return false;
        }

        if (!std::isfinite(fit_r_squared) || fit_r_squared < 0.0 || fit_r_squared > 1.0) {
            return false;
        }

        return true;
    }

} // namespace cpu_lab::domain::shared