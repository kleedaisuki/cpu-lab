#include "interfaces/cli/cli_parser.hpp"
#include "interfaces/cli/command_dispatcher.hpp"

#include <exception>
#include <iostream>

int main(const int argc, const char *const argv[])
{
    using cpu_lab::interfaces::cli::CliParser;
    using cpu_lab::interfaces::cli::CommandDispatcher;
    using cpu_lab::interfaces::cli::ExitCode;
    using cpu_lab::interfaces::cli::UsageError;

    try
    {
        const auto parse_result = CliParser::parse(argc, argv);
        const ExitCode code = CommandDispatcher::dispatch(parse_result, std::cout, std::cerr);
        return static_cast<int>(code);
    }
    catch (const UsageError &error)
    {
        std::cerr << "CLI usage error: " << error.what() << "\n\n";
        std::cerr << CliParser::help_text();
        return static_cast<int>(ExitCode::UsageError);
    }
    catch (const std::exception &error)
    {
        std::cerr << "Runtime error: " << error.what() << "\n";
        return static_cast<int>(ExitCode::RuntimeError);
    }
}
