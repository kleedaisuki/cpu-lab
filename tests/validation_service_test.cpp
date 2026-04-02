#include "interfaces/cli/cli_parser.hpp"
#include "interfaces/cli/validation_service.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

using cpu_lab::interfaces::cli::ValidateOptions;
using cpu_lab::interfaces::cli::ValidateTarget;
using cpu_lab::interfaces::cli::ValidationOutputFormat;
using cpu_lab::interfaces::cli::ValidationService;

namespace
{
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
    const std::filesystem::path matrix_valid_path = temp_dir / "cpu_lab_cli_matrix_valid.csv";
    const std::filesystem::path matrix_invalid_path = temp_dir / "cpu_lab_cli_matrix_invalid.csv";
    const std::filesystem::path sum_valid_path = temp_dir / "cpu_lab_cli_sum_valid.csv";

    std::error_code ignore{};
    std::filesystem::remove(matrix_valid_path, ignore);
    std::filesystem::remove(matrix_invalid_path, ignore);
    std::filesystem::remove(sum_valid_path, ignore);

    write_text_file(
        matrix_valid_path,
        "case_id,algorithm,rows,cols,matrix_base,matrix_row_step,matrix_col_step,vector_base,vector_step,cache_block_cols,notes\n"
        "m1,matrix_dot_cuda,3,4,1.0,0.1,0.01,0.5,0.02,8,ok\n");

    write_text_file(
        matrix_invalid_path,
        "case_id,algorithm,rows,cols,matrix_base,matrix_row_step,matrix_col_step,vector_base,vector_step,cache_block_cols,notes\n"
        "x,bad_algo,0,4,a,0.1,0.01,0.5,0.02,0,bad\n");

    write_text_file(
        sum_valid_path,
        "case_id,algorithm,length,value_base,value_step,superscalar_lanes,notes\n"
        "s1,superscalar,8,1.0,0.5,4,ok\n");

    {
        ValidateOptions options{};
        options.target = ValidateTarget::Matrix;
        options.input_csv_path = matrix_valid_path.string();
        options.run_counts = std::vector<std::size_t>{8U, 16U};
        options.warmup_rounds = 1U;
        options.strict_mode = true;
        options.format = ValidationOutputFormat::Text;

        const auto report = ValidationService::validate_matrix(options);
        assert(report.ok());
    }

    {
        ValidateOptions options{};
        options.target = ValidateTarget::Matrix;
        options.input_csv_path = matrix_invalid_path.string();
        options.run_counts = std::vector<std::size_t>{8U};
        options.warmup_rounds = 1U;
        options.strict_mode = true;
        options.format = ValidationOutputFormat::Json;

        const auto report = ValidationService::validate_matrix(options);
        assert(!report.ok());
        assert(!report.issues.empty());

        const std::string rendered = ValidationService::render_report(report, ValidationOutputFormat::Json);
        assert(rendered.find("\"target\":\"matrix\"") != std::string::npos);
        assert(rendered.find("\"issues\"") != std::string::npos);
    }

    {
        ValidateOptions options{};
        options.target = ValidateTarget::Sum;
        options.input_csv_path = sum_valid_path.string();
        options.run_counts = std::vector<std::size_t>{4U};
        options.warmup_rounds = 0U;
        options.strict_mode = true;

        const auto report = ValidationService::validate_sum(options);
        assert(report.ok());
    }

    std::filesystem::remove(matrix_valid_path, ignore);
    std::filesystem::remove(matrix_invalid_path, ignore);
    std::filesystem::remove(sum_valid_path, ignore);

    return 0;
}

