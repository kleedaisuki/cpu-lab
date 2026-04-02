#include "domain/matrix_dot/matrix_dot_cache.hpp"
#include "domain/matrix_dot/matrix_dot_cuda.hpp"
#include "domain/matrix_dot/matrix_dot_naive.hpp"
#include "domain/matrix_dot/matrix_generator.hpp"
#include "domain/shared/algorithm_orchestrator.hpp"
#include "infrastructure/csv/csv_reader.hpp"
#include "infrastructure/csv/row.hpp"

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using cpu_lab::domain::matrix_dot::MatrixDotAlgorithm;
using cpu_lab::domain::matrix_dot::MatrixDotCache;
using cpu_lab::domain::matrix_dot::MatrixDotCtorArgs;
using cpu_lab::domain::matrix_dot::MatrixDotCtorArgsGenerator;
using cpu_lab::domain::matrix_dot::MatrixDotCuda;
using cpu_lab::domain::matrix_dot::MatrixDotNaive;
using cpu_lab::domain::matrix_dot::MatrixDotTestCaseRow;
using cpu_lab::domain::shared::AlgorithmOrchestrator;
using cpu_lab::domain::shared::AlgorithmOrchestratorConfig;
using cpu_lab::infrastructure::csv::CsvReadOptions;
using cpu_lab::infrastructure::csv::CsvReader;
using cpu_lab::infrastructure::csv::row_field_count;
using cpu_lab::infrastructure::csv::row_header;

namespace
{
    /**
     * @brief 读取测试 CSV（read test csv）；Read MatrixDotTestCaseRow list from inline CSV text.
     * @return 行对象数组（row object array）。
     */
    [[nodiscard]] std::vector<MatrixDotTestCaseRow> read_rows_from_inline_csv()
    {
        const std::string csv_text =
            "case_id,algorithm,rows,cols,matrix_base,matrix_row_step,matrix_col_step,vector_base,vector_step,cache_block_cols,notes\n"
            "case_naive_small,naive,3,4,1.0,10.0,1.0,1.0,1.0,8,baseline\n"
            "case_cache_small,cache,3,4,1.0,10.0,1.0,1.0,1.0,2,cache_tile\n";

        CsvReadOptions options{};
        options.has_header = true;
        CsvReader reader(options);

        std::istringstream input(csv_text);
        return reader.read_rows<MatrixDotTestCaseRow>(input);
    }
}

int main()
{
    const auto header = row_header<MatrixDotTestCaseRow>();
    assert(header.size() == row_field_count<MatrixDotTestCaseRow>());
    assert(header.front() == "case_id");
    assert(header.back() == "notes");

    const std::vector<MatrixDotTestCaseRow> rows = read_rows_from_inline_csv();
    assert(rows.size() == 2U);

    assert(MatrixDotCtorArgsGenerator::validate_row(rows[0]));
    assert(MatrixDotCtorArgsGenerator::validate_row(rows[1]));

    std::vector<MatrixDotCtorArgs> args_list = MatrixDotCtorArgsGenerator::make_ctor_args_list(rows);
    assert(args_list.size() == 2U);

    AlgorithmOrchestrator orchestrator{};

    for (const MatrixDotCtorArgs &args : args_list)
    {
        AlgorithmOrchestratorConfig config{};
        config.benchmark_name = "matrix_dot";
        config.problem_size = args.problem_size();
        config.run_counts = std::vector<std::size_t>{16U, 32U, 64U};
        config.warmup_rounds = 1U;
        config.notes = args.case_id + ":" + args.notes;

        switch (args.algorithm)
        {
        case MatrixDotAlgorithm::Naive:
        {
            const auto result = orchestrator.operator()<MatrixDotNaive>(
                config,
                args.matrix,
                args.vector);
            assert(result.is_valid());
            assert(result.algorithm == "matrix_dot_naive");
            break;
        }

        case MatrixDotAlgorithm::Cache:
        {
            const auto result = orchestrator.operator()<MatrixDotCache>(
                config,
                args.matrix,
                args.vector,
                args.cache_block_cols);
            assert(result.is_valid());
            assert(result.algorithm == "matrix_dot_cache");
            break;
        }

        case MatrixDotAlgorithm::Cuda:
        {
            bool cuda_ran = false;
            try
            {
                const auto result = orchestrator.operator()<MatrixDotCuda>(
                    config,
                    args.matrix,
                    args.vector);
                assert(result.is_valid());
                assert(result.algorithm == "matrix_dot_cuda");
                cuda_ran = true;
            }
            catch (const std::exception &)
            {
                const auto fallback = orchestrator.operator()<MatrixDotCache>(
                    config,
                    args.matrix,
                    args.vector,
                    args.cache_block_cols);
                assert(fallback.is_valid());
                assert(fallback.algorithm == "matrix_dot_cache");
            }

            (void)cuda_ran;
            break;
        }

        default:
            assert(false);
            break;
        }
    }

    {
        MatrixDotTestCaseRow cuda_row = rows[0];
        cuda_row.algorithm = "cuda";
        const MatrixDotCtorArgs cuda_args = MatrixDotCtorArgsGenerator::make_ctor_args(cuda_row);
        assert(cuda_args.algorithm == MatrixDotAlgorithm::Cuda);
    }

    {
        MatrixDotTestCaseRow cuda_row = rows[0];
        cuda_row.algorithm = "matrix_dot_cuda";
        const MatrixDotCtorArgs cuda_args = MatrixDotCtorArgsGenerator::make_ctor_args(cuda_row);
        assert(cuda_args.algorithm == MatrixDotAlgorithm::Cuda);
    }

    bool threw_bad_algorithm = false;
    try
    {
        MatrixDotTestCaseRow bad = rows[0];
        bad.algorithm = "unknown_algo";
        (void)MatrixDotCtorArgsGenerator::make_ctor_args(bad);
    }
    catch (const std::invalid_argument &)
    {
        threw_bad_algorithm = true;
    }
    assert(threw_bad_algorithm);

    bool threw_bad_block = false;
    try
    {
        MatrixDotTestCaseRow bad = rows[1];
        bad.cache_block_cols = 0U;
        (void)MatrixDotCtorArgsGenerator::make_ctor_args(bad);
    }
    catch (const std::invalid_argument &)
    {
        threw_bad_block = true;
    }
    assert(threw_bad_block);

    return 0;
}
