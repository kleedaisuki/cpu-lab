#include "domain/matrix_dot/matrix.hpp"
#include "domain/matrix_dot/matrix_dot_naive.hpp"
#include "domain/matrix_dot/vector.hpp"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <vector>

using cpu_lab::domain::matrix_dot::Matrix;
using cpu_lab::domain::matrix_dot::MatrixDotNaive;
using cpu_lab::domain::matrix_dot::Vector;
using cpu_lab::domain::matrix_dot::matrix_dot_naive;

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
    Vector vector(3U, 1.0);
    assert(vector.size() == 3U);
    assert(!vector.empty());

    vector[0] = 1.0;
    vector[1] = 2.0;
    vector[2] = 3.0;
    assert(approx_equal(vector.at(1U), 2.0));

    bool at_threw = false;
    try {
        (void)vector.at(3U);
    }
    catch (const std::out_of_range &) {
        at_threw = true;
    }
    assert(at_threw);

    Vector other(std::vector<double>{4.0, 5.0, 6.0});
    const double dot_result = vector.dot(other);
    assert(approx_equal(dot_result, 32.0));

    Matrix matrix(3U, 3U, std::vector<double>{
                              1.0,
                              2.0,
                              3.0,
                              4.0,
                              5.0,
                              6.0,
                              7.0,
                              8.0,
                              9.0,
                          });
    assert(matrix.rows() == 3U);
    assert(matrix.cols() == 3U);
    assert(approx_equal(matrix.at(2U, 1U), 8.0));

    const Vector output = matrix_dot_naive(matrix, vector);
    assert(output.size() == 3U);
    assert(approx_equal(output[0], 30.0));
    assert(approx_equal(output[1], 36.0));
    assert(approx_equal(output[2], 42.0));

    bool shape_threw = false;
    try {
        const Vector bad_vector(2U, 1.0);
        (void)matrix_dot_naive(matrix, bad_vector);
    }
    catch (const std::invalid_argument &) {
        shape_threw = true;
    }
    assert(shape_threw);

    MatrixDotNaive policy(matrix, vector);
    assert(policy.algorithm_name() == "matrix_dot_naive");
    policy.run_once();
    const Vector &policy_output = policy.output();
    assert(approx_equal(policy_output[0], 30.0));
    assert(approx_equal(policy_output[1], 36.0));
    assert(approx_equal(policy_output[2], 42.0));
    assert(policy.output_fingerprint() == policy_output.fingerprint());

    return 0;
}
