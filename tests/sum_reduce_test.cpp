#include "domain/sum_reduce/sum_naive.hpp"
#include "domain/sum_reduce/sum_superscalar.hpp"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <vector>

using cpu_lab::domain::sum_reduce::sum_naive;
using cpu_lab::domain::sum_reduce::sum_superscalar;
using cpu_lab::domain::sum_reduce::SumNaive;
using cpu_lab::domain::sum_reduce::SumSuperscalar;

namespace
{
    /**
     * @brief 比较浮点是否近似相等（approx equal）；Check if two floating-point values are approximately equal.
     * @param lhs 左值（left value）。
     * @param rhs 右值（right value）。
     * @param eps 容差（tolerance）。
     * @return 近似相等返回 true（true if approximately equal）。
     */
    [[nodiscard]] bool approx_equal(const double lhs, const double rhs, const double eps = 1e-12)
    {
        return std::abs(lhs - rhs) <= eps;
    }
}

int main()
{
    const std::vector<double> values{1.0, 2.0, 3.5, 4.5, -2.0, 10.0};
    const double expected = 19.0;

    const double naive_sum = sum_naive(values);
    assert(approx_equal(naive_sum, expected));

    const double superscalar_default = sum_superscalar(values);
    const double superscalar_l2 = sum_superscalar(values, 2U);
    const double superscalar_l3 = sum_superscalar(values, 3U);

    assert(approx_equal(superscalar_default, expected));
    assert(approx_equal(superscalar_l2, expected));
    assert(approx_equal(superscalar_l3, expected));

    SumNaive naive_policy(values);
    assert(naive_policy.algorithm_name() == "sum_naive");
    naive_policy.run_once();
    assert(approx_equal(naive_policy.output(), expected));

    SumSuperscalar superscalar_policy(values, 4U);
    assert(superscalar_policy.algorithm_name() == "sum_superscalar");
    superscalar_policy.run_once();
    assert(approx_equal(superscalar_policy.output(), expected));

    SumSuperscalar superscalar_same(values, 4U);
    superscalar_same.run_once();
    assert(superscalar_same.output_fingerprint() == superscalar_policy.output_fingerprint());

    bool threw_zero_lane = false;
    try
    {
        (void)sum_superscalar(values, 0U);
    }
    catch (const std::invalid_argument &)
    {
        threw_zero_lane = true;
    }
    assert(threw_zero_lane);

    return 0;
}