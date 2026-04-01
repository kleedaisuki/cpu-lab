#include "domain/shared/benchmark_result.hpp"
#include "infrastructure/csv/csv_reader.hpp"
#include "infrastructure/csv/csv_writer.hpp"

#include <cassert>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

using cpu_lab::domain::shared::BenchmarkResult;
using cpu_lab::infrastructure::csv::CsvReadOptions;
using cpu_lab::infrastructure::csv::CsvReader;
using cpu_lab::infrastructure::csv::CsvWriteOptions;
using cpu_lab::infrastructure::csv::CsvWriter;
using cpu_lab::infrastructure::csv::row_field_count;
using cpu_lab::infrastructure::csv::row_header;

/**
 * @brief 构造一个有效基准结果（build valid benchmark result）；Create a valid sample result row.
 * @return 有效结果对象（valid row object）。
 */
static BenchmarkResult make_valid_row()
{
    BenchmarkResult row{};
    row.benchmark_name = "matrix_dot";
    row.algorithm = "naive";
    row.problem_size = 1024U;
    row.runs_per_sample = 32U;
    row.sample_count = 8U;
    row.best_ns_per_run = 120.5;
    row.mean_ns_per_run = 130.2;
    row.median_ns_per_run = 128.0;
    row.stddev_ns = 5.4;
    row.fit_slope_ns_per_run = 129.8;
    row.fit_intercept_ns = 41.0;
    row.fit_r_squared = 0.998;
    row.notes = "warm cache";
    return row;
}

int main()
{
    const auto header = row_header<BenchmarkResult>();
    assert(header.size() == row_field_count<BenchmarkResult>());
    assert(!header.empty());
    assert(header.front() == "benchmark_name");
    assert(header.back() == "notes");

    BenchmarkResult valid = make_valid_row();
    assert(valid.is_valid());

    BenchmarkResult bad_name = valid;
    bad_name.benchmark_name.clear();
    assert(!bad_name.is_valid());

    BenchmarkResult bad_score = valid;
    bad_score.fit_r_squared = 1.5;
    assert(!bad_score.is_valid());

    BenchmarkResult bad_nan = valid;
    bad_nan.mean_ns_per_run = std::nan("");
    assert(!bad_nan.is_valid());

    const std::vector<BenchmarkResult> rows{valid};
    std::ostringstream output{};

    CsvWriteOptions write_options{};
    write_options.write_header = true;
    CsvWriter writer{write_options};
    writer.write_rows(output, rows);

    CsvReadOptions read_options{};
    read_options.has_header = true;
    CsvReader reader{read_options};

    std::istringstream input{output.str()};
    const auto loaded = reader.read_rows<BenchmarkResult>(input);

    assert(loaded.size() == 1U);
    assert(loaded[0].benchmark_name == valid.benchmark_name);
    assert(loaded[0].algorithm == valid.algorithm);
    assert(loaded[0].problem_size == valid.problem_size);
    assert(loaded[0].runs_per_sample == valid.runs_per_sample);
    assert(loaded[0].sample_count == valid.sample_count);
    assert(std::abs(loaded[0].mean_ns_per_run - valid.mean_ns_per_run) < 1e-12);
    assert(std::abs(loaded[0].fit_r_squared - valid.fit_r_squared) < 1e-12);
    assert(loaded[0].notes == valid.notes);

    return 0;
}