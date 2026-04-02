#include "application/matrix_benchmark_pipeline.hpp"

#include "domain/matrix_dot/matrix_dot_cache.hpp"
#include "domain/matrix_dot/matrix_dot_cuda.hpp"
#include "domain/matrix_dot/matrix_dot_naive.hpp"
#include "domain/matrix_dot/matrix_generator.hpp"
#include "domain/shared/algorithm_orchestrator.hpp"
#include "infrastructure/csv/csv_reader.hpp"
#include "infrastructure/csv/csv_writer.hpp"

#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace cpu_lab::application
{

    MatrixBenchmarkPipeline::MatrixBenchmarkPipeline(MatrixBenchmarkPipelineConfig config)
        : config_(std::move(config))
    {
    }

    bool MatrixBenchmarkPipeline::validate_config(const MatrixBenchmarkPipelineConfig &config) noexcept
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

    std::vector<domain::shared::BenchmarkResult> MatrixBenchmarkPipeline::run() const
    {
        if (!validate_config(config_))
        {
            throw std::invalid_argument("MatrixBenchmarkPipelineConfig is invalid.");
        }

        infrastructure::csv::CsvReader reader{};
        const std::vector<domain::matrix_dot::MatrixDotTestCaseRow> rows =
            reader.read_rows_from_file<domain::matrix_dot::MatrixDotTestCaseRow>(
                config_.common.input_csv_path);

        const std::vector<domain::matrix_dot::MatrixDotCtorArgs> args_list =
            domain::matrix_dot::MatrixDotCtorArgsGenerator::make_ctor_args_list(rows);

        domain::shared::AlgorithmOrchestrator orchestrator{};
        std::vector<domain::shared::BenchmarkResult> results{};
        results.reserve(args_list.size());

        for (const domain::matrix_dot::MatrixDotCtorArgs &args : args_list)
        {
            domain::shared::AlgorithmOrchestratorConfig run_config{};
            run_config.benchmark_name = config_.benchmark_name;
            run_config.problem_size = args.problem_size();
            run_config.run_counts = config_.common.run_counts;
            run_config.warmup_rounds = config_.common.warmup_rounds;
            run_config.notes = args.case_id + ":" + args.notes;

            switch (args.algorithm)
            {
            case domain::matrix_dot::MatrixDotAlgorithm::Naive:
            {
                results.push_back(orchestrator.operator()<domain::matrix_dot::MatrixDotNaive>(
                    run_config,
                    args.matrix,
                    args.vector));
                break;
            }

            case domain::matrix_dot::MatrixDotAlgorithm::Cache:
            {
                results.push_back(orchestrator.operator()<domain::matrix_dot::MatrixDotCache>(
                    run_config,
                    args.matrix,
                    args.vector,
                    args.cache_block_cols));
                break;
            }

            case domain::matrix_dot::MatrixDotAlgorithm::Cuda:
            {
                try
                {
                    results.push_back(orchestrator.operator()<domain::matrix_dot::MatrixDotCuda>(
                        run_config,
                        args.matrix,
                        args.vector));
                }
                catch (const std::exception &error)
                {
                    domain::shared::AlgorithmOrchestratorConfig fallback_config = run_config;
                    fallback_config.notes += "; cuda_fallback=" + std::string(error.what());

                    results.push_back(orchestrator.operator()<domain::matrix_dot::MatrixDotCache>(
                        fallback_config,
                        args.matrix,
                        args.vector,
                        args.cache_block_cols));
                }
                break;
            }

            default:
            {
                throw std::invalid_argument("unsupported MatrixDotAlgorithm enum value.");
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
