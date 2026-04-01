#include "interfaces/cli/cli_parser.hpp"

#include <cassert>
#include <string>
#include <vector>

using cpu_lab::interfaces::cli::BenchTarget;
using cpu_lab::interfaces::cli::CliCommandKind;
using cpu_lab::interfaces::cli::CliParser;
using cpu_lab::interfaces::cli::ValidateTarget;
using cpu_lab::interfaces::cli::ValidationOutputFormat;
using cpu_lab::interfaces::cli::UsageError;

namespace
{
    [[nodiscard]] std::vector<const char *> to_argv(const std::vector<std::string> &args)
    {
        std::vector<const char *> argv{};
        argv.reserve(args.size());
        for (const std::string &arg : args)
        {
            argv.push_back(arg.c_str());
        }
        return argv;
    }
}

int main()
{
    {
        const std::vector<std::string> args{"cpu-lab", "bench", "matrix"};
        const auto argv = to_argv(args);
        const auto parsed = CliParser::parse(static_cast<int>(argv.size()), argv.data());

        assert(parsed.command_kind == CliCommandKind::Bench);
        assert(parsed.bench.target == BenchTarget::Matrix);
        assert(parsed.bench.input_csv_path == "config/benchmark_matrix.csv");
        assert(parsed.bench.output_csv_path == "data/raw/matrix_dot/benchmark_results.csv");
        assert(parsed.bench.run_counts.size() == 3U);
        assert(parsed.bench.run_counts[0] == 16U);
        assert(parsed.bench.warmup_rounds == 1U);
        assert(parsed.bench.atomic_output);
        assert(!parsed.bench.append_output);
        assert(parsed.bench.benchmark_name == "matrix_dot");
    }

    {
        const std::vector<std::string> args{
            "cpu-lab",
            "bench",
            "sum",
            "--input",
            "input.csv",
            "--output",
            "output.csv",
            "--run-counts",
            "8,16",
            "--warmup",
            "3",
            "--append",
            "--no-atomic",
            "--benchmark-name",
            "sum_custom"};
        const auto argv = to_argv(args);
        const auto parsed = CliParser::parse(static_cast<int>(argv.size()), argv.data());

        assert(parsed.command_kind == CliCommandKind::Bench);
        assert(parsed.bench.target == BenchTarget::Sum);
        assert(parsed.bench.input_csv_path == "input.csv");
        assert(parsed.bench.output_csv_path == "output.csv");
        assert(parsed.bench.run_counts.size() == 2U);
        assert(parsed.bench.run_counts[0] == 8U);
        assert(parsed.bench.run_counts[1] == 16U);
        assert(parsed.bench.warmup_rounds == 3U);
        assert(parsed.bench.append_output);
        assert(!parsed.bench.atomic_output);
        assert(parsed.bench.benchmark_name == "sum_custom");
    }

    {
        const std::vector<std::string> args{
            "cpu-lab",
            "validate",
            "all",
            "--run-counts",
            "4,8",
            "--warmup",
            "2",
            "--format",
            "json"};
        const auto argv = to_argv(args);
        const auto parsed = CliParser::parse(static_cast<int>(argv.size()), argv.data());

        assert(parsed.command_kind == CliCommandKind::Validate);
        assert(parsed.validate.target == ValidateTarget::All);
        assert(parsed.validate.run_counts.size() == 2U);
        assert(parsed.validate.run_counts[0] == 4U);
        assert(parsed.validate.run_counts[1] == 8U);
        assert(parsed.validate.warmup_rounds == 2U);
        assert(parsed.validate.format == ValidationOutputFormat::Json);
        assert(parsed.validate.strict_mode);
    }

    {
        bool threw = false;
        try
        {
            const std::vector<std::string> args{"cpu-lab", "validate", "all", "--input", "x.csv"};
            const auto argv = to_argv(args);
            (void)CliParser::parse(static_cast<int>(argv.size()), argv.data());
        }
        catch (const UsageError &)
        {
            threw = true;
        }

        assert(threw);
    }

    {
        bool threw = false;
        try
        {
            const std::vector<std::string> args{"cpu-lab", "unknown"};
            const auto argv = to_argv(args);
            (void)CliParser::parse(static_cast<int>(argv.size()), argv.data());
        }
        catch (const UsageError &)
        {
            threw = true;
        }

        assert(threw);
    }

    return 0;
}
