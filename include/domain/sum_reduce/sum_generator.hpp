#pragma once

#include "infrastructure/csv/row.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace cpu_lab::domain::sum_reduce
{

    /**
     * @brief sum_reduce 算法类型（sum-reduce algorithm kind）；Supported sum-reduce algorithm tags.
     */
    enum class SumReduceAlgorithm : std::uint8_t
    {
        /** @brief 朴素算法（naive）；Single scalar accumulation chain. */
        Naive = 0U,

        /** @brief 超标量算法（superscalar）；Multiple independent accumulation chains. */
        Superscalar = 1U,
    };

    /**
     * @brief sum_reduce 测试用例行（test case row）；RowLike schema for CSV-driven case definition.
     */
    struct SumReduceTestCaseRow final
    {
        /** @brief 用例标识（case id）；Stable identifier for one test case. */
        std::string case_id{};

        /** @brief 算法名称（algorithm name）；Algorithm selector text, e.g. naive/superscalar. */
        std::string algorithm{};

        /** @brief 数组长度（value count）；Number of generated scalar values. */
        std::size_t length{0U};

        /** @brief 基值（base value）；Base value used by synthetic input generator. */
        double value_base{0.0};

        /** @brief 步长（value step）；Increment per index in generated input. */
        double value_step{0.0};

        /** @brief 超标量链路数（superscalar lane count）；Lane count used by superscalar policy, ignored by naive. */
        std::size_t superscalar_lanes{4U};

        /** @brief 备注（notes）；Free-form notes for this case row. */
        std::string notes{};

        /**
         * @brief 字段元信息（metadata）；Reflected schema for CSV row mapping.
         * @return 字段描述元组（field descriptor tuple）。
         */
        [[nodiscard]] static auto meta() noexcept
            -> std::tuple<
                infrastructure::csv::RowField<SumReduceTestCaseRow, std::string>,
                infrastructure::csv::RowField<SumReduceTestCaseRow, std::string>,
                infrastructure::csv::RowField<SumReduceTestCaseRow, std::size_t>,
                infrastructure::csv::RowField<SumReduceTestCaseRow, double>,
                infrastructure::csv::RowField<SumReduceTestCaseRow, double>,
                infrastructure::csv::RowField<SumReduceTestCaseRow, std::size_t>,
                infrastructure::csv::RowField<SumReduceTestCaseRow, std::string>>;
    };

    /**
     * @brief sum_reduce 构造参数包（constructor args package）；Generated ctor-args payload for orchestrator call-site.
     */
    struct SumReduceCtorArgs final
    {
        /** @brief 用例标识（case id）；Source test-case identifier. */
        std::string case_id{};

        /** @brief 算法类型（algorithm kind）；Parsed algorithm tag. */
        SumReduceAlgorithm algorithm{SumReduceAlgorithm::Naive};

        /** @brief 输入数组（input values）；Owned values argument. */
        std::vector<double> values{};

        /** @brief 超标量链路数（superscalar lane count）；Second ctor arg for superscalar policy. */
        std::size_t superscalar_lanes{4U};

        /** @brief 备注（notes）；Notes passed through from CSV row. */
        std::string notes{};

        /**
         * @brief 问题规模（problem size）；Compute scalar problem size for benchmark metadata.
         * @return 问题规模（problem size）。
         */
        [[nodiscard]] std::size_t problem_size() const noexcept;
    };

    /**
     * @brief sum_reduce 构造参数生成器（ctor-args generator）；Generate ctor-args payloads from RowLike test-case rows.
     */
    class SumReduceCtorArgsGenerator final
    {
    public:
        /**
         * @brief 校验用例行（validate row）；Validate one test-case row before generation.
         * @param row 用例行（test-case row）。
         * @return 合法返回 true（true if valid）。
         */
        [[nodiscard]] static bool validate_row(const SumReduceTestCaseRow &row) noexcept;

        /**
         * @brief 解析算法名称（parse algorithm）；Parse algorithm tag from text.
         * @param algorithm 算法文本（algorithm text）。
         * @return 算法枚举（algorithm enum）。
         * @note 未知算法抛出 std::invalid_argument（throws std::invalid_argument on unknown algorithm）。
         */
        [[nodiscard]] static SumReduceAlgorithm parse_algorithm(std::string_view algorithm);

        /**
         * @brief 从用例行生成构造参数（generate ctor-args from row）；Create one ctor-args package from one row.
         * @param row 用例行（test-case row）。
         * @return 构造参数包（ctor-args package）。
         * @note 行非法时抛出 std::invalid_argument（throws std::invalid_argument on invalid row）。
         */
        [[nodiscard]] static SumReduceCtorArgs make_ctor_args(const SumReduceTestCaseRow &row);

        /**
         * @brief 批量生成构造参数（generate ctor-args list）；Create ctor-args packages from multiple rows.
         * @param rows 用例行数组（test-case row array）。
         * @return 构造参数包数组（ctor-args package array）。
         */
        [[nodiscard]] static std::vector<SumReduceCtorArgs> make_ctor_args_list(
            const std::vector<SumReduceTestCaseRow> &rows);

    private:
        /**
         * @brief 构造测试数组（build values）；Build deterministic values from row parameters.
         * @param row 用例行（test-case row）。
         * @return 生成数组（generated values）。
         */
        [[nodiscard]] static std::vector<double> build_values(const SumReduceTestCaseRow &row);
    };

} // namespace cpu_lab::domain::sum_reduce