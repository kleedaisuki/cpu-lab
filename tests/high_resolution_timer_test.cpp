#include "infrastructure/timing/high_resolution_timer.hpp"

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>

using cpu_lab::infrastructure::timing::HighResolutionTimer;
using cpu_lab::infrastructure::timing::LeastSquaresFit;
using cpu_lab::infrastructure::timing::TimerReport;

/**
 * @brief 验证最小二乘参数是否合理；Validate least-squares output sanity.
 * @param fit 拟合结果（fit result）。
 * @return 无返回值；No return value.
 */
static void assert_fit_sane(const LeastSquaresFit& fit) {
    assert(fit.slope_ns_per_run >= 0.0);
    assert(fit.r_squared <= 1.0 + 1e-9);
}

int main() {
    std::atomic<std::size_t> sink {0U};

    HighResolutionTimer timer {8U};
    const auto run_counts = HighResolutionTimer::build_default_run_counts(128U, 2U, 7U);

    const TimerReport report = timer.measure(
        [&sink]() {
            std::size_t local = 0U;
            for (std::size_t i = 0U; i < 256U; ++i) {
                local += (i * 3U) ^ (i >> 1U);
            }
            sink.fetch_add(local, std::memory_order_relaxed);
        },
        run_counts);

    assert(!report.samples.empty());
    assert(report.samples.size() == run_counts.size());
    assert_fit_sane(report.fit);

    std::cout << "samples=" << report.samples.size()
              << ", slope_ns_per_run=" << report.fit.slope_ns_per_run
              << ", intercept_ns=" << report.fit.intercept_ns
              << ", r2=" << report.fit.r_squared << '\n';

    return 0;
}
