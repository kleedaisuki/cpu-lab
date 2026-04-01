#include "interfaces/cli/command_dispatcher.hpp"

#include "application/matrix_benchmark_pipeline.hpp"
#include "application/sum_reduce_benchmark_pipeline.hpp"
#include "interfaces/cli/cli_parser.hpp"
#include "interfaces/cli/validation_service.hpp"

#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

namespace cpu_lab::interfaces::cli
{
    namespace
    {
        void ensure_output_parent_exists(const std::string &output_csv_path)
        {
            const std::filesystem::path output_path{output_csv_path};
            if (!output_path.has_parent_path())
            {
                return;
            }

            std::filesystem::create_directories(output_path.parent_path());
        }
    } // namespace

    ExitCode CommandDispatcher::dispatch(
        const CliParseResult &parse_result,
        std::ostream &out,
        std::ostream &err)
    {
        switch (parse_result.command_kind)
        {
        case CliCommandKind::Help:
            out << CliParser::help_text();
            return ExitCode::Success;

        case CliCommandKind::Bench:
        {
            application::BenchmarkPipelineConfig common{};
            common.input_csv_path = parse_result.bench.input_csv_path;
            common.output_csv_path = parse_result.bench.output_csv_path;
            common.run_counts = parse_result.bench.run_counts;
            common.warmup_rounds = parse_result.bench.warmup_rounds;
            common.append_output = parse_result.bench.append_output;
            common.atomic_output = parse_result.bench.atomic_output;

            ensure_output_parent_exists(common.output_csv_path);

            if (parse_result.bench.target == BenchTarget::Matrix)
            {
                application::MatrixBenchmarkPipelineConfig config{};
                config.common = common;
                config.benchmark_name = parse_result.bench.benchmark_name;

                application::MatrixBenchmarkPipeline pipeline{config};
                const auto rows = pipeline.run();
                out << "bench matrix completed with " << rows.size() << " result rows.\n";
                return ExitCode::Success;
            }

            application::SumReduceBenchmarkPipelineConfig config{};
            config.common = common;
            config.benchmark_name = parse_result.bench.benchmark_name;

            application::SumReduceBenchmarkPipeline pipeline{config};
            const auto rows = pipeline.run();
            out << "bench sum completed with " << rows.size() << " result rows.\n";
            return ExitCode::Success;
        }

        case CliCommandKind::Validate:
        {
            ValidationReport report{};
            switch (parse_result.validate.target)
            {
            case ValidateTarget::Matrix:
                report = ValidationService::validate_matrix(parse_result.validate);
                break;
            case ValidateTarget::Sum:
                report = ValidationService::validate_sum(parse_result.validate);
                break;
            case ValidateTarget::All:
                report = ValidationService::validate_all(parse_result.validate);
                break;
            }

            const std::string rendered = ValidationService::render_report(report, parse_result.validate.format);
            if (report.ok())
            {
                out << rendered;
                return ExitCode::Success;
            }

            err << rendered;
            return ExitCode::ValidationFailed;
        }
        }

        err << "unknown CLI command kind.\n";
        return ExitCode::RuntimeError;
    }

} // namespace cpu_lab::interfaces::cli
