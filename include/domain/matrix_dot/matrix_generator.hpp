#pragma once

#include "domain/matrix_dot/matrix.hpp"
#include "domain/matrix_dot/vector.hpp"
#include "infrastructure/csv/row.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace cpu_lab::domain::matrix_dot
{

    /**
     * @brief 矩阵点积算法类型（matrix-dot algorithm kind）；Supported matrix-dot algorithm tags.
     */
    enum class MatrixDotAlgorithm : std::uint8_t
    {
        /** @brief 朴素算法（naive）；Column-wise baseline traversal. */
        Naive = 0U,

        /** @brief cache 优化算法（cache）；Row-wise cache-friendly traversal. */
        Cache = 1U,

        /** @brief CUDA 算法（CUDA）；GPU-accelerated policy when CUDA path is available. */
        Cuda = 2U,
    };

    /**
     * @brief matrix_dot 测试用例行（test case row）；RowLike schema for CSV-driven case definition.
     */
    struct MatrixDotTestCaseRow final
    {
        /** @brief 用例标识（case id）；Stable identifier for one test case. */
        std::string case_id{};

        /** @brief 算法名称（algorithm name）；Algorithm selector text, e.g. naive/cache/cuda. */
        std::string algorithm{};

        /** @brief 矩阵行数（matrix rows）；Number of matrix rows (M). */
        std::size_t rows{0U};

        /** @brief 矩阵列数（matrix cols）；Number of matrix columns (N). */
        std::size_t cols{0U};

        /** @brief 矩阵基值（matrix base）；Base value used by synthetic matrix generator. */
        double matrix_base{0.0};

        /** @brief 矩阵行步长（matrix row step）；Increment per row in generated matrix. */
        double matrix_row_step{0.0};

        /** @brief 矩阵列步长（matrix col step）；Increment per column in generated matrix. */
        double matrix_col_step{0.0};

        /** @brief 向量基值（vector base）；Base value used by synthetic vector generator. */
        double vector_base{0.0};

        /** @brief 向量步长（vector step）；Increment per index in generated vector. */
        double vector_step{0.0};

        /** @brief cache 列分块（cache block cols）；Tile width used by cache policy, ignored by naive. */
        std::size_t cache_block_cols{64U};

        /** @brief 备注（notes）；Free-form notes for this case row. */
        std::string notes{};

        /**
         * @brief 字段元信息（metadata）；Reflected schema for CSV row mapping.
         * @return 字段描述元组（field descriptor tuple）。
         */
        [[nodiscard]] static auto meta() noexcept
            -> std::tuple<
                infrastructure::csv::RowField<MatrixDotTestCaseRow, std::string>,
                infrastructure::csv::RowField<MatrixDotTestCaseRow, std::string>,
                infrastructure::csv::RowField<MatrixDotTestCaseRow, std::size_t>,
                infrastructure::csv::RowField<MatrixDotTestCaseRow, std::size_t>,
                infrastructure::csv::RowField<MatrixDotTestCaseRow, double>,
                infrastructure::csv::RowField<MatrixDotTestCaseRow, double>,
                infrastructure::csv::RowField<MatrixDotTestCaseRow, double>,
                infrastructure::csv::RowField<MatrixDotTestCaseRow, double>,
                infrastructure::csv::RowField<MatrixDotTestCaseRow, double>,
                infrastructure::csv::RowField<MatrixDotTestCaseRow, std::size_t>,
                infrastructure::csv::RowField<MatrixDotTestCaseRow, std::string>>;
    };

    /**
     * @brief matrix_dot 构造参数包（constructor args package）；Generated ctor-args payload for orchestrator call-site.
     */
    struct MatrixDotCtorArgs final
    {
        /** @brief 用例标识（case id）；Source test-case identifier. */
        std::string case_id{};

        /** @brief 算法类型（algorithm kind）；Parsed algorithm tag. */
        MatrixDotAlgorithm algorithm{MatrixDotAlgorithm::Naive};

        /** @brief 输入矩阵（input matrix）；Owned matrix argument. */
        Matrix matrix{};

        /** @brief 输入向量（input vector）；Owned vector argument. */
        Vector vector{};

        /** @brief cache 列分块（cache block cols）；Third ctor arg for cache policy. */
        std::size_t cache_block_cols{64U};

        /** @brief 备注（notes）；Notes passed through from CSV row. */
        std::string notes{};

        /**
         * @brief 问题规模（problem size）；Compute scalar problem size for benchmark metadata.
         * @return 问题规模（problem size）。
         */
        [[nodiscard]] std::size_t problem_size() const noexcept;
    };

    /**
     * @brief matrix_dot 构造参数生成器（ctor-args generator）；Generate ctor-args payloads from RowLike test-case rows.
     */
    class MatrixDotCtorArgsGenerator final
    {
    public:
        /**
         * @brief 校验用例行（validate row）；Validate one test-case row before generation.
         * @param row 用例行（test-case row）。
         * @return 合法返回 true（true if valid）。
         */
        [[nodiscard]] static bool validate_row(const MatrixDotTestCaseRow &row) noexcept;

        /**
         * @brief 解析算法名称（parse algorithm）；Parse algorithm tag from text.
         * @param algorithm 算法文本（algorithm text）。
         * @return 算法枚举（algorithm enum）。
         * @note 未知算法抛出 std::invalid_argument（throws std::invalid_argument on unknown algorithm）。
         */
        [[nodiscard]] static MatrixDotAlgorithm parse_algorithm(std::string_view algorithm);

        /**
         * @brief 从用例行生成构造参数（generate ctor-args from row）；Create one ctor-args package from one row.
         * @param row 用例行（test-case row）。
         * @return 构造参数包（ctor-args package）。
         * @note 行非法时抛出 std::invalid_argument（throws std::invalid_argument on invalid row）。
         */
        [[nodiscard]] static MatrixDotCtorArgs make_ctor_args(const MatrixDotTestCaseRow &row);

        /**
         * @brief 批量生成构造参数（generate ctor-args list）；Create ctor-args packages from multiple rows.
         * @param rows 用例行数组（test-case row array）。
         * @return 构造参数包数组（ctor-args package array）。
         */
        [[nodiscard]] static std::vector<MatrixDotCtorArgs> make_ctor_args_list(
            const std::vector<MatrixDotTestCaseRow> &rows);

    private:
        /**
         * @brief 构造测试矩阵（build matrix）；Build deterministic matrix from row parameters.
         * @param row 用例行（test-case row）。
         * @return 生成矩阵（generated matrix）。
         */
        [[nodiscard]] static Matrix build_matrix(const MatrixDotTestCaseRow &row);

        /**
         * @brief 构造测试向量（build vector）；Build deterministic vector from row parameters.
         * @param row 用例行（test-case row）。
         * @return 生成向量（generated vector）。
         */
        [[nodiscard]] static Vector build_vector(const MatrixDotTestCaseRow &row);
    };

} // namespace cpu_lab::domain::matrix_dot

