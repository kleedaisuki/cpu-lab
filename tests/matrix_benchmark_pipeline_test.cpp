#include "application/matrix_benchmark_pipeline.hpp"
#include "infrastructure/csv/csv_reader.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using cpu_lab::application::BenchmarkPipelineConfig;
using cpu_lab::application::MatrixBenchmarkPipeline;
using cpu_lab::application::MatrixBenchmarkPipelineConfig;
using cpu_lab::domain::shared::BenchmarkResult;
using cpu_lab::infrastructure::csv::CsvReader;
using cpu_lab::infrastructure::csv::CsvReadOptions;

namespace
{
    /**
     * @brief 写入 matrix_dot 输入 CSV（write matrix-dot input CSV）；Create deterministic matrix test-case CSV file.
     * @param file_path 文件路径（file path）。
     * @return 无返回值（no return value）。
     */
    void write_input_csv(const std::filesystem::path &file_path)
    {
        std::ofstream output(file_path, std::ios::out | std::ios::trunc);
        assert(output.is_open());
        output
            << "case_id,algorithm,rows,cols,matrix_base,matrix_row_step,matrix_col_step,vector_base,vector_step,cache_block_cols,notes\n"
            << "m1,naive,3,4,1.0,10.0,1.0,1.0,1.0,8,baseline\n"
            << "m2,cache,3,4,1.0,10.0,1.0,1.0,1.0,2,cache_tile\n";
    }
}

int main()
{
    const std::filesystem::path input_path = std::filesystem::temp_directory_path() / "cpu_lab_matrix_pipeline_in.csv";
    const std::filesystem::path output_path = std::filesystem::temp_directory_path() / "cpu_lab_matrix_pipeline_out.csv";

    std::error_code ignore{};
    std::filesystem::remove(input_path, ignore);
    std::filesystem::remove(output_path, ignore);

    write_input_csv(input_path);

    BenchmarkPipelineConfig common{};
    common.input_csv_path = input_path.string();
    common.output_csv_path = output_path.string();
    common.run_counts = std::vector<std::size_t>{16U, 32U, 64U};
    common.warmup_rounds = 1U;

    MatrixBenchmarkPipelineConfig config{};
    config.common = common;

    MatrixBenchmarkPipeline pipeline{config};
    const std::vector<BenchmarkResult> results = pipeline.run();

    assert(results.size() == 2U);
    assert(results[0].benchmark_name == "matrix_dot");
    assert(results[0].algorithm == "matrix_dot_naive");
    assert(results[1].algorithm == "matrix_dot_cache");

    CsvReader reader{CsvReadOptions{}};
    const auto loaded = reader.read_rows_from_file<BenchmarkResult>(output_path.string());
    assert(loaded.size() == 2U);
    assert(loaded[0].algorithm == "matrix_dot_naive");
    assert(loaded[1].algorithm == "matrix_dot_cache");

    std::filesystem::remove(input_path, ignore);
    std::filesystem::remove(output_path, ignore);
    return 0;
}
