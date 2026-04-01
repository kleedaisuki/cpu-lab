#include "application/sum_reduce_benchmark_pipeline.hpp"
#include "infrastructure/csv/csv_reader.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using cpu_lab::application::BenchmarkPipelineConfig;
using cpu_lab::application::SumReduceBenchmarkPipeline;
using cpu_lab::application::SumReduceBenchmarkPipelineConfig;
using cpu_lab::domain::shared::BenchmarkResult;
using cpu_lab::infrastructure::csv::CsvReader;
using cpu_lab::infrastructure::csv::CsvReadOptions;

namespace
{
    /**
     * @brief 写入 sum_reduce 输入 CSV（write sum-reduce input CSV）；Create deterministic sum test-case CSV file.
     * @param file_path 文件路径（file path）。
     * @return 无返回值（no return value）。
     */
    void write_input_csv(const std::filesystem::path &file_path)
    {
        std::ofstream output(file_path, std::ios::out | std::ios::trunc);
        assert(output.is_open());
        output
            << "case_id,algorithm,length,value_base,value_step,superscalar_lanes,notes\n"
            << "s1,naive,8,1.0,0.5,4,baseline\n"
            << "s2,superscalar,8,1.0,0.5,3,multi_lane\n";
    }
}

int main()
{
    const std::filesystem::path input_path = std::filesystem::temp_directory_path() / "cpu_lab_sum_pipeline_in.csv";
    const std::filesystem::path output_path = std::filesystem::temp_directory_path() / "cpu_lab_sum_pipeline_out.csv";

    std::error_code ignore{};
    std::filesystem::remove(input_path, ignore);
    std::filesystem::remove(output_path, ignore);

    write_input_csv(input_path);

    BenchmarkPipelineConfig common{};
    common.input_csv_path = input_path.string();
    common.output_csv_path = output_path.string();
    common.run_counts = std::vector<std::size_t>{16U, 32U, 64U};
    common.warmup_rounds = 1U;

    SumReduceBenchmarkPipelineConfig config{};
    config.common = common;

    SumReduceBenchmarkPipeline pipeline{config};
    const std::vector<BenchmarkResult> results = pipeline.run();

    assert(results.size() == 2U);
    assert(results[0].benchmark_name == "sum_reduce");
    assert(results[0].algorithm == "sum_naive");
    assert(results[1].algorithm == "sum_superscalar");

    CsvReader reader{CsvReadOptions{}};
    const auto loaded = reader.read_rows_from_file<BenchmarkResult>(output_path.string());
    assert(loaded.size() == 2U);
    assert(loaded[0].algorithm == "sum_naive");
    assert(loaded[1].algorithm == "sum_superscalar");

    std::filesystem::remove(input_path, ignore);
    std::filesystem::remove(output_path, ignore);
    return 0;
}
