#include "domain/shared/algorithm_orchestrator.hpp"
#include "domain/sum_reduce/sum_generator.hpp"
#include "domain/sum_reduce/sum_naive.hpp"
#include "domain/sum_reduce/sum_superscalar.hpp"
#include "infrastructure/csv/csv_reader.hpp"
#include "infrastructure/csv/row.hpp"

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using cpu_lab::domain::shared::AlgorithmOrchestrator;
using cpu_lab::domain::shared::AlgorithmOrchestratorConfig;
using cpu_lab::domain::sum_reduce::SumReduceAlgorithm;
using cpu_lab::domain::sum_reduce::SumReduceCtorArgs;
using cpu_lab::domain::sum_reduce::SumReduceCtorArgsGenerator;
using cpu_lab::domain::sum_reduce::SumReduceTestCaseRow;
using cpu_lab::domain::sum_reduce::SumNaive;
using cpu_lab::domain::sum_reduce::SumSuperscalar;
using cpu_lab::infrastructure::csv::CsvReadOptions;
using cpu_lab::infrastructure::csv::CsvReader;
using cpu_lab::infrastructure::csv::row_field_count;
using cpu_lab::infrastructure::csv::row_header;

namespace
{
    /**
     * @brief 读取测试 CSV（read test csv）；Read SumReduceTestCaseRow list from inline CSV text.
     * @return 行对象数组（row object array）。
     */
    [[nodiscard]] std::vector<SumReduceTestCaseRow> read_rows_from_inline_csv()
    {
        const std::string csv_text =
            "case_id,algorithm,length,value_base,value_step,superscalar_lanes,notes\n"
            "sum_naive_small,naive,8,1.0,0.5,4,baseline\n"
            "sum_super_small,superscalar,8,1.0,0.5,3,multi_lane\n";

        CsvReadOptions options{};
        options.has_header = true;
        CsvReader reader(options);

        std::istringstream input(csv_text);
        return reader.read_rows<SumReduceTestCaseRow>(input);
    }
}

int main()
{
    const auto header = row_header<SumReduceTestCaseRow>();
    assert(header.size() == row_field_count<SumReduceTestCaseRow>());
    assert(header.front() == "case_id");
    assert(header.back() == "notes");

    const std::vector<SumReduceTestCaseRow> rows = read_rows_from_inline_csv();
    assert(rows.size() == 2U);

    assert(SumReduceCtorArgsGenerator::validate_row(rows[0]));
    assert(SumReduceCtorArgsGenerator::validate_row(rows[1]));

    std::vector<SumReduceCtorArgs> args_list = SumReduceCtorArgsGenerator::make_ctor_args_list(rows);
    assert(args_list.size() == 2U);

    AlgorithmOrchestrator orchestrator{};

    for (const SumReduceCtorArgs &args : args_list)
    {
        AlgorithmOrchestratorConfig config{};
        config.benchmark_name = "sum_reduce";
        config.problem_size = args.problem_size();
        config.run_counts = std::vector<std::size_t>{16U, 32U, 64U};
        config.warmup_rounds = 1U;
        config.notes = args.case_id + ":" + args.notes;

        switch (args.algorithm)
        {
        case SumReduceAlgorithm::Naive:
        {
            const auto result = orchestrator.operator()<SumNaive>(
                config,
                args.values);
            assert(result.is_valid());
            assert(result.algorithm == "sum_naive");
            break;
        }

        case SumReduceAlgorithm::Superscalar:
        {
            const auto result = orchestrator.operator()<SumSuperscalar>(
                config,
                args.values,
                args.superscalar_lanes);
            assert(result.is_valid());
            assert(result.algorithm == "sum_superscalar");
            break;
        }
        }
    }

    bool threw_bad_algorithm = false;
    try
    {
        SumReduceTestCaseRow bad = rows[0];
        bad.algorithm = "unknown_algo";
        (void)SumReduceCtorArgsGenerator::make_ctor_args(bad);
    }
    catch (const std::invalid_argument &)
    {
        threw_bad_algorithm = true;
    }
    assert(threw_bad_algorithm);

    bool threw_bad_lane = false;
    try
    {
        SumReduceTestCaseRow bad = rows[1];
        bad.superscalar_lanes = 0U;
        (void)SumReduceCtorArgsGenerator::make_ctor_args(bad);
    }
    catch (const std::invalid_argument &)
    {
        threw_bad_lane = true;
    }
    assert(threw_bad_lane);

    return 0;
}