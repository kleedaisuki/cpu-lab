#pragma once

#include "interfaces/cli/cli_parser.hpp"

#include <string>
#include <vector>

namespace cpu_lab::interfaces::cli
{

    /**
     * @brief One validation failure item.
     */
    struct ValidationIssue final
    {
        /** @brief Logical scope label, e.g. matrix:row[1]. */
        std::string scope{};

        /** @brief Error message text. */
        std::string message{};
    };

    /**
     * @brief Validation report payload.
     */
    struct ValidationReport final
    {
        /** @brief Target label for this report. */
        std::string target{};

        /** @brief Aggregated validation issues. */
        std::vector<ValidationIssue> issues{};

        /**
         * @brief Return true when report has zero issues.
         * @return True if valid.
         */
        [[nodiscard]] bool ok() const noexcept;
    };

    /**
     * @brief Strict validation service for CLI commands.
     */
    class ValidationService final
    {
    public:
        /**
         * @brief Validate one matrix profile input and options.
         * @param options Validate command options.
         * @return Validation report.
         */
        [[nodiscard]] static ValidationReport validate_matrix(const ValidateOptions &options);

        /**
         * @brief Validate one sum profile input and options.
         * @param options Validate command options.
         * @return Validation report.
         */
        [[nodiscard]] static ValidationReport validate_sum(const ValidateOptions &options);

        /**
         * @brief Validate both matrix and sum profiles.
         * @param options Validate command options.
         * @return Aggregated validation report.
         */
        [[nodiscard]] static ValidationReport validate_all(const ValidateOptions &options);

        /**
         * @brief Render report by requested output format.
         * @param report Validation report.
         * @param format Output format.
         * @return Rendered report text.
         */
        [[nodiscard]] static std::string render_report(
            const ValidationReport &report,
            ValidationOutputFormat format);
    };

} // namespace cpu_lab::interfaces::cli
