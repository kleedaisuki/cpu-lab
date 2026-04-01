#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace cpu_lab::interfaces::cli
{

    /**
     * @brief Top-level CLI command kind.
     */
    enum class CliCommandKind : std::uint8_t
    {
        /** @brief Render usage/help text. */
        Help = 0U,

        /** @brief Run benchmark pipelines. */
        Bench = 1U,

        /** @brief Run strict static validations. */
        Validate = 2U,
    };

    /**
     * @brief Benchmark subcommand target.
     */
    enum class BenchTarget : std::uint8_t
    {
        /** @brief Matrix-dot benchmark profile. */
        Matrix = 0U,

        /** @brief Sum-reduce benchmark profile. */
        Sum = 1U,
    };

    /**
     * @brief Validation subcommand target.
     */
    enum class ValidateTarget : std::uint8_t
    {
        /** @brief Matrix-dot validation profile. */
        Matrix = 0U,

        /** @brief Sum-reduce validation profile. */
        Sum = 1U,

        /** @brief Validate both matrix and sum profiles. */
        All = 2U,
    };

    /**
     * @brief Validation report output format.
     */
    enum class ValidationOutputFormat : std::uint8_t
    {
        /** @brief Human-readable text report. */
        Text = 0U,

        /** @brief Machine-readable JSON report. */
        Json = 1U,
    };

    /**
     * @brief Resolved options for bench subcommands.
     */
    struct BenchOptions final
    {
        /** @brief Selected benchmark profile target. */
        BenchTarget target{BenchTarget::Matrix};

        /** @brief Path to benchmark input CSV file. */
        std::string input_csv_path{};

        /** @brief Path to benchmark output CSV file. */
        std::string output_csv_path{};

        /** @brief Timer sample run-count list. */
        std::vector<std::size_t> run_counts{};

        /** @brief Warmup rounds before timed runs. */
        std::size_t warmup_rounds{1U};

        /** @brief Append rows to existing output file. */
        bool append_output{false};

        /** @brief Use atomic write when not in append mode. */
        bool atomic_output{true};

        /** @brief Benchmark name emitted into result rows. */
        std::string benchmark_name{};
    };

    /**
     * @brief Resolved options for validate subcommands.
     */
    struct ValidateOptions final
    {
        /** @brief Selected validation profile target. */
        ValidateTarget target{ValidateTarget::Matrix};

        /** @brief Path to test-case CSV file. */
        std::string input_csv_path{};

        /** @brief Run-count list checked by config validation. */
        std::vector<std::size_t> run_counts{};

        /** @brief Warmup rounds checked by config validation. */
        std::size_t warmup_rounds{1U};

        /** @brief Always true in v1 strict validation mode. */
        bool strict_mode{true};

        /** @brief Report rendering format. */
        ValidationOutputFormat format{ValidationOutputFormat::Text};
    };

    /**
     * @brief Normalized parse result for dispatcher input.
     */
    struct CliParseResult final
    {
        /** @brief Top-level parsed command kind. */
        CliCommandKind command_kind{CliCommandKind::Help};

        /** @brief Payload when command kind is Bench. */
        BenchOptions bench{};

        /** @brief Payload when command kind is Validate. */
        ValidateOptions validate{};
    };

    /**
     * @brief Exception for CLI usage/parse failures.
     */
    class UsageError : public std::runtime_error
    {
    public:
        /**
         * @brief Create usage error with detail message.
         * @param message Error details.
         */
        explicit UsageError(const std::string &message);
    };

    /**
     * @brief Parse argv into normalized command payload.
     */
    class CliParser final
    {
    public:
        /**
         * @brief Parse argv into CliParseResult.
         * @param argc Argument count.
         * @param argv Argument vector.
         * @return Parsed result.
         * @note Throws UsageError on invalid syntax/options.
         */
        [[nodiscard]] static CliParseResult parse(int argc, const char *const argv[]);

        /**
         * @brief Return CLI usage and option reference text.
         * @return Help text.
         */
        [[nodiscard]] static std::string help_text();
    };

} // namespace cpu_lab::interfaces::cli
