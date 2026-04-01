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
    const std::filesystem::path input_path = temp_dir / "cpu_lab_cli_dispatch_matrix.csv";
    const std::filesystem::path output_path = temp_dir / "cpu_lab_cli_dispatch_matrix_out.csv";

    std::error_code ignore{};
    std::filesystem::remove(input_path, ignore);
    std::filesystem::remove(output_path, ignore);

    write_text_file(
        input_path,
        "case_id,algorithm,rows,cols,matrix_base,matrix_row_step,matrix_col_step,vector_base,vector_step,cache_block_cols,notes\n"
        "m1,naive,3,4,1.0,0.1,0.01,0.5,0.02,8,ok\n"
        "m2,cache,3,4,1.0,0.1,0.01,0.5,0.02,2,ok\n");

    {
        const std::vector<std::string> args{
            "cpu-lab",
            "bench",
            "matrix",
            "--input",
            input_path.string(),
            "--output",
            output_path.string(),
            "--run-counts",
            "8,16",
            "--warmup",
            "1"};

        const auto argv = to_argv(args);
        const auto parsed = CliParser::parse(static_cast<int>(argv.size()), argv.data());

        std::ostringstream out{};
        std::ostringstream err{};
        const ExitCode code = CommandDispatcher::dispatch(parsed, out, err);

        assert(code == ExitCode::Success);
        assert(err.str().empty());
        assert(std::filesystem::exists(output_path));
    }

    {
        const std::vector<std::string> args{
            "cpu-lab",
            "validate",
            "matrix",
            "--input",
            input_path.string(),
            "--format",
            "text"};

        const auto argv = to_argv(args);
        const auto parsed = CliParser::parse(static_cast<int>(argv.size()), argv.data());

        std::ostringstream out{};
        std::ostringstream err{};
        const ExitCode code = CommandDispatcher::dispatch(parsed, out, err);

        assert(code == ExitCode::Success);
        assert(out.str().find("status=ok") != std::string::npos);
        assert(err.str().empty());
    }

    {
        const std::vector<std::string> args{
            "cpu-lab",
            "validate",
            "matrix",
            "--input",
            "definitely_not_existing.csv"};

        const auto argv = to_argv(args);
        const auto parsed = CliParser::parse(static_cast<int>(argv.size()), argv.data());

        std::ostringstream out{};
        std::ostringstream err{};
        const ExitCode code = CommandDispatcher::dispatch(parsed, out, err);

        assert(code == ExitCode::ValidationFailed);
        assert(!err.str().empty());
    }

    std::filesystem::remove(input_path, ignore);
    std::filesystem::remove(output_path, ignore);
    return 0;
}
