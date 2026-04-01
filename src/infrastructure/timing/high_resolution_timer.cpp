#include "infrastructure/timing/high_resolution_timer.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace cpu_lab::infrastructure::timing
{

    HighResolutionTimer::HighResolutionTimer(const std::size_t warmup_rounds) noexcept
        : warmup_rounds_(std::max<std::size_t>(1U, warmup_rounds))
    {
    }

    TimerReport HighResolutionTimer::measure(
        const std::function<void()> &task,
        const std::vector<std::size_t> &run_counts) const
    {
        if (!task)
        {
            throw std::invalid_argument("HighResolutionTimer::measure requires a valid task.");
        }

        if (run_counts.empty())
        {
            throw std::invalid_argument("HighResolutionTimer::measure requires non-empty run_counts.");
        }

        for (std::size_t i = 0; i < warmup_rounds_; ++i)
        {
            task();
        }

        TimerReport report{};
        report.samples.reserve(run_counts.size());

        for (const std::size_t run_count : run_counts)
        {
            if (run_count == 0U)
            {
                continue;
            }

            const double elapsed_ns = run_once(task, run_count);
            report.samples.push_back(TimingSample{
                run_count,
                elapsed_ns,
            });
        }

        if (report.samples.empty())
        {
            throw std::invalid_argument("HighResolutionTimer::measure got only zero run counts.");
        }

        report.fit = fit_linear_least_squares(report.samples);
        return report;
    }

    std::vector<std::size_t> HighResolutionTimer::build_default_run_counts(
        const std::size_t first,
        const std::size_t ratio,
        const std::size_t points)
    {
        if (first == 0U)
        {
            throw std::invalid_argument("build_default_run_counts requires first > 0.");
        }

        if (ratio < 2U)
        {
            throw std::invalid_argument("build_default_run_counts requires ratio >= 2.");
        }

        if (points == 0U)
        {
            throw std::invalid_argument("build_default_run_counts requires points > 0.");
        }

        std::vector<std::size_t> values{};
        values.reserve(points);

        std::size_t current = first;
        for (std::size_t i = 0; i < points; ++i)
        {
            values.push_back(current);

            if (i + 1U < points)
            {
                if (current > (std::numeric_limits<std::size_t>::max() / ratio))
                {
                    throw std::overflow_error("build_default_run_counts overflowed size_t.");
                }
                current *= ratio;
            }
        }

        return values;
    }

    LeastSquaresFit HighResolutionTimer::fit_linear_least_squares(
        const std::vector<TimingSample> &samples) noexcept
    {
        LeastSquaresFit fit{};

        if (samples.empty())
        {
            return fit;
        }

        const double n = static_cast<double>(samples.size());

        double sum_x = 0.0;
        double sum_y = 0.0;
        double sum_xx = 0.0;
        double sum_xy = 0.0;

        for (const TimingSample &sample : samples)
        {
            const double x = static_cast<double>(sample.run_count);
            const double y = sample.elapsed_ns;
            sum_x += x;
            sum_y += y;
            sum_xx += x * x;
            sum_xy += x * y;
        }

        const double denom = (n * sum_xx) - (sum_x * sum_x);
        if (denom <= std::numeric_limits<double>::epsilon())
        {
            fit.intercept_ns = sum_y / n;
            fit.r_squared = 0.0;
            return fit;
        }

        fit.slope_ns_per_run = ((n * sum_xy) - (sum_x * sum_y)) / denom;
        fit.intercept_ns = (sum_y - (fit.slope_ns_per_run * sum_x)) / n;

        const double mean_y = sum_y / n;
        double ss_tot = 0.0;
        double ss_res = 0.0;

        for (const TimingSample &sample : samples)
        {
            const double x = static_cast<double>(sample.run_count);
            const double pred = (fit.slope_ns_per_run * x) + fit.intercept_ns;
            const double dy = sample.elapsed_ns - mean_y;
            const double ry = sample.elapsed_ns - pred;
            ss_tot += dy * dy;
            ss_res += ry * ry;
        }

        if (ss_tot <= std::numeric_limits<double>::epsilon())
        {
            fit.r_squared = 1.0;
        }
        else
        {
            fit.r_squared = 1.0 - (ss_res / ss_tot);
        }

        return fit;
    }

    double HighResolutionTimer::run_once(
        const std::function<void()> &task,
        const std::size_t run_count)
    {
        const auto begin = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < run_count; ++i)
        {
            task();
        }
        const auto end = std::chrono::steady_clock::now();

        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
        return static_cast<double>(duration.count());
    }

} // namespace cpu_lab::infrastructure::timing
