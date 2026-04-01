#include "interfaces/cli/cli_parser.hpp"
#include "interfaces/cli/command_dispatcher.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using cpu_lab::interfaces::cli::CliParser;
using cpu_lab::interfaces::cli::CommandDispatcher;
using cpu_lab::interfaces::cli::ExitCode;
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

    void write_text_file(const std::filesystem::path &path, const std::string &content)
    {
        std::ofstream output(path, std::ios::out | std::ios::trunc);
        assert(output.is_open());
        output << content;
    }
}

int main()
{
    const std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    const std::filesystem::path sum_input_path = temp_dir / "cpu_lab_cli_integration_sum.csv";
    const std::filesystem::path sum_output_path = temp_dir / "cpu_lab_cli_integration_sum_out.csv";

    std::error_code ignore{};
    std::filesystem::remove(sum_input_path, ignore);
    std::filesystem::remove(sum_output_path, ignore);

    write_text_file(
        sum_input_path,
        "case_id,algorithm,length,value_base,value_step,superscalar_lanes,notes\n"
        "s1,naive,8,1.0,0.5,2,ok\n"
        "s2,superscalar,8,1.0,0.5,4,ok\n");

    {
        const std::vector<std::string> args{
            "cpu-lab",
            "bench",
            "sum",
            "--input",
            sum_input_path.string(),
            "--output",
            sum_output_path.string(),
            "--run-counts",
            "8,16,32"};

        const auto argv = to_argv(args);
        const auto parsed = CliParser::parse(static_cast<int>(argv.size()), argv.data());

        std::ostringstream out{};
        std::ostringstream err{};
        const ExitCode code = CommandDispatcher::dispatch(parsed, out, err);

        assert(code == ExitCode::Success);
        assert(err.str().empty());
        assert(std::filesystem::exists(sum_output_path));
    }

    {
        const std::vector<std::string> args{
            "cpu-lab",
            "validate",
            "sum",
            "--input",
            sum_input_path.string(),
            "--format",
            "json"};

        const auto argv = to_argv(args);
        const auto parsed = CliParser::parse(static_cast<int>(argv.size()), argv.data());

        std::ostringstream out{};
        std::ostringstream err{};
        const ExitCode code = CommandDispatcher::dispatch(parsed, out, err);

        assert(code == ExitCode::Success);
        assert(out.str().find("\"ok\":true") != std::string::npos);
    }

    {
        bool threw = false;
        try
        {
            const std::vector<std::string> args{"cpu-lab", "bench", "sum", "--run-counts", "8,0"};
            const auto argv = to_argv(args);
            (void)CliParser::parse(static_cast<int>(argv.size()), argv.data());
        }
        catch (const UsageError &)
        {
            threw = true;
        }

        assert(threw);
    }

    std::filesystem::remove(sum_input_path, ignore);
    std::filesystem::remove(sum_output_path, ignore);
    return 0;
}
