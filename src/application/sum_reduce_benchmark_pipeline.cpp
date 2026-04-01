#include "application/sum_reduce_benchmark_pipeline.hpp"

#include "domain/shared/algorithm_orchestrator.hpp"
#include "domain/sum_reduce/sum_generator.hpp"
#include "domain/sum_reduce/sum_naive.hpp"
#include "domain/sum_reduce/sum_superscalar.hpp"
#include "infrastructure/csv/csv_reader.hpp"
#include "infrastructure/csv/csv_writer.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace cpu_lab::application
{

    SumReduceBenchmarkPipeline::SumReduceBenchmarkPipeline(SumReduceBenchmarkPipelineConfig config)
        : config_(std::move(config))
    {
    }

    bool SumReduceBenchmarkPipeline::validate_config(const SumReduceBenchmarkPipelineConfig &config) noexcept
    {
        if (config.benchmark_name.empty())
        {
            return false;
        }

        if (config.common.input_csv_path.empty() || config.common.output_csv_path.empty())
        {
            return false;
        }

        if (config.common.run_counts.empty())
        {
            return false;
        }

        for (const std::size_t run_count : config.common.run_counts)
        {
            if (run_count == 0U)
            {
                return false;
            }
        }

        return true;
    }

    std::vector<domain::shared::BenchmarkResult> SumReduceBenchmarkPipeline::run() const
    {
        if (!validate_config(config_))
        {
            throw std::invalid_argument("SumReduceBenchmarkPipelineConfig is invalid.");
        }

        infrastructure::csv::CsvReader reader{};
        const std::vector<domain::sum_reduce::SumReduceTestCaseRow> rows =
            reader.read_rows_from_file<domain::sum_reduce::SumReduceTestCaseRow>(
                config_.common.input_csv_path);

        const std::vector<domain::sum_reduce::SumReduceCtorArgs> args_list =
            domain::sum_reduce::SumReduceCtorArgsGenerator::make_ctor_args_list(rows);

        domain::shared::AlgorithmOrchestrator orchestrator{};
        std::vector<domain::shared::BenchmarkResult> results{};
        results.reserve(args_list.size());

        for (const domain::sum_reduce::SumReduceCtorArgs &args : args_list)
        {
            domain::shared::AlgorithmOrchestratorConfig run_config{};
            run_config.benchmark_name = config_.benchmark_name;
            run_config.problem_size = args.problem_size();
            run_config.run_counts = config_.common.run_counts;
            run_config.warmup_rounds = config_.common.warmup_rounds;
            run_config.notes = args.case_id + ":" + args.notes;

            switch (args.algorithm)
            {
            case domain::sum_reduce::SumReduceAlgorithm::Naive:
            {
                results.push_back(orchestrator.operator()<domain::sum_reduce::SumNaive>(
                    run_config,
                    args.values));
                break;
            }

            case domain::sum_reduce::SumReduceAlgorithm::Superscalar:
            {
                results.push_back(orchestrator.operator()<domain::sum_reduce::SumSuperscalar>(
                    run_config,
                    args.values,
                    args.superscalar_lanes));
                break;
            }
            }
        }

        infrastructure::csv::CsvWriteOptions writer_options{};
        writer_options.append = config_.common.append_output;
        writer_options.atomic_write = config_.common.atomic_output;

        infrastructure::csv::CsvWriter writer{writer_options};
        writer.write_rows_to_file(config_.common.output_csv_path, results);

        return results;
    }

} // namespace cpu_lab::application
