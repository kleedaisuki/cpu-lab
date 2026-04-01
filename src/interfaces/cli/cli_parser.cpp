#include "interfaces/cli/cli_parser.hpp"

#include <charconv>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cpu_lab::interfaces::cli
{
    namespace
    {
        [[nodiscard]] std::vector<std::size_t> default_run_counts()
        {
            return std::vector<std::size_t>{16U, 32U, 64U};
        }

        [[nodiscard]] std::string default_matrix_input_path()
        {
            return "config/benchmark_matrix.csv";
        }

        [[nodiscard]] std::string default_sum_input_path()
        {
            return "config/benchmark_sum.csv";
        }

        [[nodiscard]] std::string default_matrix_output_path()
        {
            return "data/raw/matrix_dot/benchmark_results.csv";
        }

        [[nodiscard]] std::string default_sum_output_path()
        {
            return "data/raw/sum_reduce/benchmark_results.csv";
        }

        [[nodiscard]] std::string default_matrix_benchmark_name()
        {
            return "matrix_dot";
        }

        [[nodiscard]] std::string default_sum_benchmark_name()
        {
            return "sum_reduce";
        }

        [[nodiscard]] std::size_t parse_size_t(std::string_view text, const std::string &flag_name)
        {
            if (text.empty())
            {
                throw UsageError("missing value for " + flag_name + ".");
            }

            std::size_t value{0U};
            const char *const begin = text.data();
            const char *const end = text.data() + text.size();
            const auto result = std::from_chars(begin, end, value);
            if (result.ec != std::errc{} || result.ptr != end)
            {
                throw UsageError("invalid integer value for " + flag_name + ": '" + std::string(text) + "'.");
            }

            return value;
        }

        [[nodiscard]] std::vector<std::size_t> parse_run_counts(std::string_view text)
        {
            std::vector<std::size_t> values{};
            std::size_t token_begin{0U};

            while (token_begin <= text.size())
            {
                const std::size_t comma_index = text.find(',', token_begin);
                const std::size_t token_end = (comma_index == std::string_view::npos) ? text.size() : comma_index;
                const std::string_view token = text.substr(token_begin, token_end - token_begin);
                if (token.empty())
                {
                    throw UsageError("run-count list has empty item.");
                }

                const std::size_t value = parse_size_t(token, "--run-counts");
                if (value == 0U)
                {
                    throw UsageError("run-count list must contain only positive integers.");
                }
                values.push_back(value);

                if (comma_index == std::string_view::npos)
                {
                    break;
                }
                token_begin = comma_index + 1U;
            }

            if (values.empty())
            {
                throw UsageError("run-count list must not be empty.");
            }

            return values;
        }

        [[nodiscard]] bool is_help_token(std::string_view token) noexcept
        {
            return token == "-h" || token == "--help" || token == "help";
        }

        [[nodiscard]] const char *require_next_value(
            const int argc,
            const char *const argv[],
            int &index,
            const std::string &flag_name)
        {
            const int next_index = index + 1;
            if (next_index >= argc)
            {
                throw UsageError("missing value for " + flag_name + ".");
            }

            index = next_index;
            return argv[index];
        }

        [[nodiscard]] BenchOptions default_bench_options(const BenchTarget target)
        {
            BenchOptions options{};
            options.target = target;
            options.run_counts = default_run_counts();
            options.warmup_rounds = 1U;
            options.append_output = false;
            options.atomic_output = true;

            if (target == BenchTarget::Matrix)
            {
                options.input_csv_path = default_matrix_input_path();
                options.output_csv_path = default_matrix_output_path();
                options.benchmark_name = default_matrix_benchmark_name();
            }
            else
            {
                options.input_csv_path = default_sum_input_path();
                options.output_csv_path = default_sum_output_path();
                options.benchmark_name = default_sum_benchmark_name();
            }

            return options;
        }

        [[nodiscard]] ValidateOptions default_validate_options(const ValidateTarget target)
        {
            ValidateOptions options{};
            options.target = target;
            options.run_counts = default_run_counts();
            options.warmup_rounds = 1U;
            options.strict_mode = true;
            options.format = ValidationOutputFormat::Text;

            if (target == ValidateTarget::Matrix)
            {
                options.input_csv_path = default_matrix_input_path();
            }
            else if (target == ValidateTarget::Sum)
            {
                options.input_csv_path = default_sum_input_path();
            }

            return options;
        }

        [[nodiscard]] BenchTarget parse_bench_target(std::string_view token)
        {
            if (token == "matrix")
            {
                return BenchTarget::Matrix;
            }
            if (token == "sum")
            {
                return BenchTarget::Sum;
            }

            throw UsageError("unknown bench target: '" + std::string(token) + "'.");
        }

        [[nodiscard]] ValidateTarget parse_validate_target(std::string_view token)
        {
            if (token == "matrix")
            {
                return ValidateTarget::Matrix;
            }
            if (token == "sum")
            {
                return ValidateTarget::Sum;
            }
            if (token == "all")
            {
                return ValidateTarget::All;
            }

            throw UsageError("unknown validate target: '" + std::string(token) + "'.");
        }

        void parse_bench_flags(BenchOptions &options, const int argc, const char *const argv[], int start_index)
        {
            for (int index = start_index; index < argc; ++index)
            {
                const std::string_view token = argv[index];

                if (is_help_token(token))
                {
                    throw UsageError("help requested");
                }
                if (token == "--append")
                {
                    options.append_output = true;
                    continue;
                }
                if (token == "--no-atomic")
                {
                    options.atomic_output = false;
                    continue;
                }
                if (token == "--input")
                {
                    options.input_csv_path = require_next_value(argc, argv, index, "--input");
                    continue;
                }
                if (token == "--output")
                {
                    options.output_csv_path = require_next_value(argc, argv, index, "--output");
                    continue;
                }
                if (token == "--run-counts")
                {
                    options.run_counts = parse_run_counts(require_next_value(argc, argv, index, "--run-counts"));
                    continue;
                }
                if (token == "--warmup")
                {
                    options.warmup_rounds = parse_size_t(
                        require_next_value(argc, argv, index, "--warmup"),
                        "--warmup");
                    continue;
                }
                if (token == "--benchmark-name")
                {
                    options.benchmark_name = require_next_value(argc, argv, index, "--benchmark-name");
                    continue;
                }

                throw UsageError("unknown bench option: '" + std::string(token) + "'.");
            }
        }

        void parse_validate_flags(ValidateOptions &options, const int argc, const char *const argv[], int start_index)
        {
            for (int index = start_index; index < argc; ++index)
            {
                const std::string_view token = argv[index];

                if (is_help_token(token))
                {
                    throw UsageError("help requested");
                }
                if (token == "--strict")
                {
                    options.strict_mode = true;
                    continue;
                }
                if (token == "--input")
                {
                    if (options.target == ValidateTarget::All)
                    {
                        throw UsageError("--input is not allowed for 'validate all'.");
                    }
                    options.input_csv_path = require_next_value(argc, argv, index, "--input");
                    continue;
                }
                if (token == "--run-counts")
                {
                    options.run_counts = parse_run_counts(require_next_value(argc, argv, index, "--run-counts"));
                    continue;
                }
                if (token == "--warmup")
                {
                    options.warmup_rounds = parse_size_t(
                        require_next_value(argc, argv, index, "--warmup"),
                        "--warmup");
                    continue;
                }
                if (token == "--format")
                {
                    const std::string_view value = require_next_value(argc, argv, index, "--format");
                    if (value == "text")
                    {
                        options.format = ValidationOutputFormat::Text;
                        continue;
                    }
                    if (value == "json")
                    {
                        options.format = ValidationOutputFormat::Json;
                        continue;
                    }
                    throw UsageError("unknown --format value: '" + std::string(value) + "'.");
                }

                throw UsageError("unknown validate option: '" + std::string(token) + "'.");
            }
        }
    } // namespace

    UsageError::UsageError(const std::string &message)
        : std::runtime_error(message)
    {
    }

    CliParseResult CliParser::parse(const int argc, const char *const argv[])
    {
        CliParseResult result{};

        if (argc <= 1)
        {
            result.command_kind = CliCommandKind::Help;
            return result;
        }

        const std::string_view command = argv[1];
        if (is_help_token(command))
        {
            result.command_kind = CliCommandKind::Help;
            return result;
        }

        if (command == "bench")
        {
            if (argc <= 2)
            {
                throw UsageError("missing bench target (expected: matrix|sum).");
            }

            const BenchTarget target = parse_bench_target(argv[2]);
            result.command_kind = CliCommandKind::Bench;
            result.bench = default_bench_options(target);
            parse_bench_flags(result.bench, argc, argv, 3);
            return result;
        }

        if (command == "validate")
        {
            if (argc <= 2)
            {
                throw UsageError("missing validate target (expected: matrix|sum|all).");
            }

            const ValidateTarget target = parse_validate_target(argv[2]);
            result.command_kind = CliCommandKind::Validate;
            result.validate = default_validate_options(target);
            parse_validate_flags(result.validate, argc, argv, 3);
            return result;
        }

        throw UsageError("unknown command: '" + std::string(command) + "'.");
    }

    std::string CliParser::help_text()
    {
        std::ostringstream stream{};
        stream
            << "cpu-lab CLI\n"
            << "\n"
            << "Commands:\n"
            << "  cpu-lab bench matrix [options]\n"
            << "  cpu-lab bench sum [options]\n"
            << "  cpu-lab validate matrix [options]\n"
            << "  cpu-lab validate sum [options]\n"
            << "  cpu-lab validate all [options]\n"
            << "\n"
            << "bench options:\n"
            << "  --input <path>\n"
            << "  --output <path>\n"
            << "  --run-counts <csv>     e.g. 16,32,64\n"
            << "  --warmup <n>\n"
            << "  --append\n"
            << "  --no-atomic\n"
            << "  --benchmark-name <name>\n"
            << "\n"
            << "validate options:\n"
            << "  --input <path>         (not allowed for validate all)\n"
            << "  --run-counts <csv>\n"
            << "  --warmup <n>\n"
            << "  --strict\n"
            << "  --format <text|json>\n"
            << "\n"
            << "Defaults:\n"
            << "  matrix input : config/benchmark_matrix.csv\n"
            << "  sum input    : config/benchmark_sum.csv\n"
            << "  matrix output: data/raw/matrix_dot/benchmark_results.csv\n"
            << "  sum output   : data/raw/sum_reduce/benchmark_results.csv\n"
            << "  run-counts   : 16,32,64\n"
            << "  warmup       : 1\n";

        return stream.str();
    }

} // namespace cpu_lab::interfaces::cli
