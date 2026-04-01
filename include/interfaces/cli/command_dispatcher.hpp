#pragma once

#include "interfaces/cli/cli_parser.hpp"

#include <iosfwd>

namespace cpu_lab::interfaces::cli
{

    /**
     * @brief CLI process exit codes.
     */
    enum class ExitCode : int
    {
        /** @brief Successful completion. */
        Success = 0,

        /** @brief CLI usage/parsing failure. */
        UsageError = 1,

        /** @brief Strict validation failure. */
        ValidationFailed = 2,

        /** @brief Runtime execution failure. */
        RuntimeError = 3,
    };

    /**
     * @brief Dispatch parsed command into concrete runtime behavior.
     */
    class CommandDispatcher final
    {
    public:
        /**
         * @brief Execute one parsed command.
         * @param parse_result Parsed CLI result.
         * @param out Standard output stream.
         * @param err Standard error stream.
         * @return Exit code.
         */
        [[nodiscard]] static ExitCode dispatch(
            const CliParseResult &parse_result,
            std::ostream &out,
            std::ostream &err);
    };

} // namespace cpu_lab::interfaces::cli
