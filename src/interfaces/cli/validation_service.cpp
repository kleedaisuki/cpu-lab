#include "interfaces/cli/validation_service.hpp"

#include "application/matrix_benchmark_pipeline.hpp"
#include "application/sum_reduce_benchmark_pipeline.hpp"
#include "domain/matrix_dot/matrix_generator.hpp"
#include "domain/sum_reduce/sum_generator.hpp"
#include "infrastructure/csv/csv_reader.hpp"

#include <charconv>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cpu_lab::interfaces::cli
{
    namespace
    {
        [[nodiscard]] std::vector<std::size_t> default_run_counts()
        {
            return std::vector<std::size_t>{16U, 32U, 64U};
        }

        [[nodiscard]] std::string default_matrix_input_path()
        {
            return "config/benchmark_matrix.csv";
        }

        [[nodiscard]] std::string default_sum_input_path()
        {
            return "config/benchmark_sum.csv";
        }

        [[nodiscard]] std::string to_lower_ascii(std::string_view input)
        {
            std::string output{};
            output.reserve(input.size());

            for (const char ch : input)
            {
                if (ch >= 'A' && ch <= 'Z')
                {
                    output.push_back(static_cast<char>(ch - 'A' + 'a'));
                }
                else
                {
                    output.push_back(ch);
                }
            }

            return output;
        }

        void add_issue(ValidationReport &report, const std::string &scope, const std::string &message)
        {
            report.issues.push_back(ValidationIssue{scope, message});
        }

        [[nodiscard]] bool parse_size_t(std::string_view text, std::size_t &value) noexcept
        {
            value = 0U;
            if (text.empty())
            {
                return false;
            }

            const char *const begin = text.data();
            const char *const end = text.data() + text.size();
            const auto result = std::from_chars(begin, end, value);
            return result.ec == std::errc{} && result.ptr == end;
        }

        [[nodiscard]] bool parse_double(std::string_view text, double &value) noexcept
        {
            value = 0.0;
            if (text.empty())
            {
                return false;
            }

            const char *const begin = text.data();
            const char *const end = text.data() + text.size();
            const auto result = std::from_chars(begin, end, value);
            return result.ec == std::errc{} && result.ptr == end;
        }

        [[nodiscard]] std::unordered_map<std::string, std::size_t> make_header_index(
            const std::vector<std::string> &header,
            ValidationReport &report)
        {
            std::unordered_map<std::string, std::size_t> index_map{};
            for (std::size_t index = 0U; index < header.size(); ++index)
            {
                const std::string &column = header[index];
                if (column.empty())
                {
                    add_issue(report, "header", "column name must not be empty");
                    continue;
                }

                const auto inserted = index_map.emplace(column, index);
                if (!inserted.second)
                {
                    add_issue(report, "header", "duplicate column: '" + column + "'");
                }
            }

            return index_map;
        }

        [[nodiscard]] bool ensure_columns(
            const std::vector<std::string> &required_columns,
            const std::unordered_map<std::string, std::size_t> &header_index,
            ValidationReport &report)
        {
            bool ok = true;
            for (const std::string &column : required_columns)
            {
                if (header_index.find(column) == header_index.end())
                {
                    add_issue(report, "header", "missing required column: '" + column + "'");
                    ok = false;
                }
            }

            return ok;
        }

        [[nodiscard]] std::string_view get_cell(
            const std::vector<std::string> &record,
            const std::unordered_map<std::string, std::size_t> &header_index,
            const std::string &column_name,
            ValidationReport &report,
            const std::string &row_scope)
        {
            const auto it = header_index.find(column_name);
            if (it == header_index.end())
            {
                add_issue(report, row_scope, "unknown column access: '" + column_name + "'");
                return {};
            }

            const std::size_t column_index = it->second;
            if (column_index >= record.size())
            {
                add_issue(report, row_scope, "record has too few columns for: '" + column_name + "'");
                return {};
            }

            return record[column_index];
        }

        void validate_common_options(const ValidateOptions &options, ValidationReport &report)
        {
            if (options.run_counts.empty())
            {
                add_issue(report, "options", "run-count list must not be empty");
            }

            for (const std::size_t run_count : options.run_counts)
            {
                if (run_count == 0U)
                {
                    add_issue(report, "options", "run-count values must be positive");
                    break;
                }
            }

            if (!options.strict_mode)
            {
                add_issue(report, "options", "v1 only supports strict mode");
            }

            if (options.input_csv_path.empty())
            {
                add_issue(report, "options", "input CSV path must not be empty");
                return;
            }

            const std::filesystem::path path = options.input_csv_path;
            if (!std::filesystem::exists(path))
            {
                add_issue(report, "options", "input CSV file does not exist: '" + options.input_csv_path + "'");
                return;
            }

            std::ifstream input(path);
            if (!input.is_open())
            {
                add_issue(report, "options", "input CSV file is not readable: '" + options.input_csv_path + "'");
            }
        }

        [[nodiscard]] std::string json_escape(const std::string &text)
        {
            std::string escaped{};
            escaped.reserve(text.size() + 8U);

            for (const char ch : text)
            {
                switch (ch)
                {
                case '\\':
                    escaped += "\\\\";
                    break;
                case '"':
                    escaped += "\\\"";
                    break;
                case '\n':
                    escaped += "\\n";
                    break;
                case '\r':
                    escaped += "\\r";
                    break;
                case '\t':
                    escaped += "\\t";
                    break;
                default:
                    escaped.push_back(ch);
                    break;
                }
            }

            return escaped;
        }

        [[nodiscard]] ValidationReport validate_matrix_rows(const ValidateOptions &options)
        {
            ValidationReport report{"matrix", {}};
            validate_common_options(options, report);
            if (!report.ok())
            {
                return report;
            }

            infrastructure::csv::CsvReader reader{};
            infrastructure::csv::CsvTable table{};

            try
            {
                table = reader.read_table_from_file(options.input_csv_path);
            }
            catch (const std::exception &error)
            {
                add_issue(report, "csv", error.what());
                return report;
            }

            const auto header_index = make_header_index(table.header, report);
            const bool has_columns = ensure_columns(
                std::vector<std::string>{
                    "case_id",
                    "algorithm",
                    "rows",
                    "cols",
                    "matrix_base",
                    "matrix_row_step",
                    "matrix_col_step",
                    "vector_base",
                    "vector_step",
                    "cache_block_cols",
                    "notes"},
                header_index,
                report);

            if (!has_columns)
            {
                return report;
            }

            for (std::size_t row_index = 0U; row_index < table.records.size(); ++row_index)
            {
                const auto &record = table.records[row_index];
                const std::string scope = "matrix:row[" + std::to_string(row_index + 1U) + "]";

                const std::string case_id = std::string(get_cell(record, header_index, "case_id", report, scope));
                if (case_id.empty())
                {
                    add_issue(report, scope, "case_id must not be empty");
                }

                const std::string algorithm = to_lower_ascii(get_cell(record, header_index, "algorithm", report, scope));
                if (!(algorithm == "naive" || algorithm == "cache"))
                {
                    add_issue(report, scope, "algorithm must be one of: naive, cache");
                }

                std::size_t rows{0U};
                if (!parse_size_t(get_cell(record, header_index, "rows", report, scope), rows) || rows == 0U)
                {
                    add_issue(report, scope, "rows must be a positive integer");
                }

                std::size_t cols{0U};
                if (!parse_size_t(get_cell(record, header_index, "cols", report, scope), cols) || cols == 0U)
                {
                    add_issue(report, scope, "cols must be a positive integer");
                }

                std::size_t cache_block_cols{0U};
                if (!parse_size_t(get_cell(record, header_index, "cache_block_cols", report, scope), cache_block_cols) ||
                    cache_block_cols == 0U)
                {
                    add_issue(report, scope, "cache_block_cols must be a positive integer");
                }

                double number_value{0.0};
                if (!parse_double(get_cell(record, header_index, "matrix_base", report, scope), number_value))
                {
                    add_issue(report, scope, "matrix_base must be a floating-point number");
                }
                if (!parse_double(get_cell(record, header_index, "matrix_row_step", report, scope), number_value))
                {
                    add_issue(report, scope, "matrix_row_step must be a floating-point number");
                }
                if (!parse_double(get_cell(record, header_index, "matrix_col_step", report, scope), number_value))
                {
                    add_issue(report, scope, "matrix_col_step must be a floating-point number");
                }
                if (!parse_double(get_cell(record, header_index, "vector_base", report, scope), number_value))
                {
                    add_issue(report, scope, "vector_base must be a floating-point number");
                }
                if (!parse_double(get_cell(record, header_index, "vector_step", report, scope), number_value))
                {
                    add_issue(report, scope, "vector_step must be a floating-point number");
                }
            }

            application::MatrixBenchmarkPipelineConfig pipeline_config{};
            pipeline_config.common.input_csv_path = options.input_csv_path;
            pipeline_config.common.output_csv_path = "validation_only_matrix.csv";
            pipeline_config.common.run_counts = options.run_counts;
            pipeline_config.common.warmup_rounds = options.warmup_rounds;
            pipeline_config.benchmark_name = "matrix_dot";

            if (!application::MatrixBenchmarkPipeline::validate_config(pipeline_config))
            {
                add_issue(report, "pipeline", "matrix benchmark pipeline configuration is invalid");
            }

            return report;
        }

        [[nodiscard]] ValidationReport validate_sum_rows(const ValidateOptions &options)
        {
            ValidationReport report{"sum", {}};
            validate_common_options(options, report);
            if (!report.ok())
            {
                return report;
            }

            infrastructure::csv::CsvReader reader{};
            infrastructure::csv::CsvTable table{};

            try
            {
                table = reader.read_table_from_file(options.input_csv_path);
            }
            catch (const std::exception &error)
            {
                add_issue(report, "csv", error.what());
                return report;
            }

            const auto header_index = make_header_index(table.header, report);
            const bool has_columns = ensure_columns(
                std::vector<std::string>{
                    "case_id",
                    "algorithm",
                    "length",
                    "value_base",
                    "value_step",
                    "superscalar_lanes",
                    "notes"},
                header_index,
                report);

            if (!has_columns)
            {
                return report;
            }

            for (std::size_t row_index = 0U; row_index < table.records.size(); ++row_index)
            {
                const auto &record = table.records[row_index];
                const std::string scope = "sum:row[" + std::to_string(row_index + 1U) + "]";

                const std::string case_id = std::string(get_cell(record, header_index, "case_id", report, scope));
                if (case_id.empty())
                {
                    add_issue(report, scope, "case_id must not be empty");
                }

                const std::string algorithm = to_lower_ascii(get_cell(record, header_index, "algorithm", report, scope));
                if (!(algorithm == "naive" || algorithm == "superscalar"))
                {
                    add_issue(report, scope, "algorithm must be one of: naive, superscalar");
                }

                std::size_t length{0U};
                if (!parse_size_t(get_cell(record, header_index, "length", report, scope), length) || length == 0U)
                {
                    add_issue(report, scope, "length must be a positive integer");
                }

                std::size_t lanes{0U};
                if (!parse_size_t(get_cell(record, header_index, "superscalar_lanes", report, scope), lanes) || lanes == 0U)
                {
                    add_issue(report, scope, "superscalar_lanes must be a positive integer");
                }

                double number_value{0.0};
                if (!parse_double(get_cell(record, header_index, "value_base", report, scope), number_value))
                {
                    add_issue(report, scope, "value_base must be a floating-point number");
                }
                if (!parse_double(get_cell(record, header_index, "value_step", report, scope), number_value))
                {
                    add_issue(report, scope, "value_step must be a floating-point number");
                }
            }

            application::SumReduceBenchmarkPipelineConfig pipeline_config{};
            pipeline_config.common.input_csv_path = options.input_csv_path;
            pipeline_config.common.output_csv_path = "validation_only_sum.csv";
            pipeline_config.common.run_counts = options.run_counts;
            pipeline_config.common.warmup_rounds = options.warmup_rounds;
            pipeline_config.benchmark_name = "sum_reduce";

            if (!application::SumReduceBenchmarkPipeline::validate_config(pipeline_config))
            {
                add_issue(report, "pipeline", "sum benchmark pipeline configuration is invalid");
            }

            return report;
        }
    } // namespace

    bool ValidationReport::ok() const noexcept
    {
        return issues.empty();
    }

    ValidationReport ValidationService::validate_matrix(const ValidateOptions &options)
    {
        ValidateOptions resolved = options;
        resolved.target = ValidateTarget::Matrix;
        if (resolved.input_csv_path.empty())
        {
            resolved.input_csv_path = default_matrix_input_path();
        }
        if (resolved.run_counts.empty())
        {
            resolved.run_counts = default_run_counts();
        }

        return validate_matrix_rows(resolved);
    }

    ValidationReport ValidationService::validate_sum(const ValidateOptions &options)
    {
        ValidateOptions resolved = options;
        resolved.target = ValidateTarget::Sum;
        if (resolved.input_csv_path.empty())
        {
            resolved.input_csv_path = default_sum_input_path();
        }
        if (resolved.run_counts.empty())
        {
            resolved.run_counts = default_run_counts();
        }

        return validate_sum_rows(resolved);
    }

    ValidationReport ValidationService::validate_all(const ValidateOptions &options)
    {
        ValidationReport report{"all", {}};

        ValidateOptions matrix_options = options;
        matrix_options.target = ValidateTarget::Matrix;
        matrix_options.input_csv_path = default_matrix_input_path();

        ValidateOptions sum_options = options;
        sum_options.target = ValidateTarget::Sum;
        sum_options.input_csv_path = default_sum_input_path();

        const ValidationReport matrix_report = validate_matrix(matrix_options);
        for (const auto &issue : matrix_report.issues)
        {
            add_issue(report, issue.scope, issue.message);
        }

        const ValidationReport sum_report = validate_sum(sum_options);
        for (const auto &issue : sum_report.issues)
        {
            add_issue(report, issue.scope, issue.message);
        }

        return report;
    }

    std::string ValidationService::render_report(
        const ValidationReport &report,
        const ValidationOutputFormat format)
    {
        if (format == ValidationOutputFormat::Json)
        {
            std::ostringstream json{};
            json << "{"
                 << "\"target\":\"" << json_escape(report.target) << "\"," 
                 << "\"ok\":" << (report.ok() ? "true" : "false") << ","
                 << "\"issues\":[";

            for (std::size_t index = 0U; index < report.issues.size(); ++index)
            {
                if (index > 0U)
                {
                    json << ',';
                }

                const auto &issue = report.issues[index];
                json << "{"
                     << "\"scope\":\"" << json_escape(issue.scope) << "\"," 
                     << "\"message\":\"" << json_escape(issue.message) << "\""
                     << "}";
            }

            json << "]}";
            return json.str();
        }

        std::ostringstream text{};
        text << "validate target=" << report.target << " status=" << (report.ok() ? "ok" : "failed") << '\n';

        for (const auto &issue : report.issues)
        {
            text << "- [" << issue.scope << "] " << issue.message << '\n';
        }

        return text.str();
    }

} // namespace cpu_lab::interfaces::cli
