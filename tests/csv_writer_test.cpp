#include "infrastructure/csv/csv_reader.hpp"
#include "infrastructure/csv/csv_writer.hpp"

#include <cassert>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

using cpu_lab::infrastructure::csv::CsvReadOptions;
using cpu_lab::infrastructure::csv::CsvReader;
using cpu_lab::infrastructure::csv::CsvWriteOptions;
using cpu_lab::infrastructure::csv::CsvWriter;
using cpu_lab::infrastructure::csv::Row;
using cpu_lab::infrastructure::csv::make_row_field;

/**
 * @brief 用于 CSV 写入测试的行类型（test row）；Row model for CSV writer test.
 */
struct WriteExampleRow final : Row<WriteExampleRow>
{
    /** @brief 计数字段（count field）；Integral field. */
    int count{0};

    /** @brief 标签字段（label field）；String field. */
    std::string label{};

    /** @brief 标志字段（enabled field）；Boolean field. */
    bool enabled{false};

    /**
     * @brief 返回字段元信息（metadata）；Field reflection metadata.
     * @return 元组字段描述（tuple descriptors）。
     */
    [[nodiscard]] static constexpr auto meta() noexcept
    {
        return std::make_tuple(
            make_row_field("count", &WriteExampleRow::count),
            make_row_field("label", &WriteExampleRow::label),
            make_row_field("enabled", &WriteExampleRow::enabled));
    }
};

int main()
{
    WriteExampleRow row1{};
    row1.count = 1;
    row1.label = "alpha,beta";
    row1.enabled = true;

    WriteExampleRow row2{};
    row2.count = 2;
    row2.label = "line \"x\"";
    row2.enabled = false;

    const std::vector<WriteExampleRow> rows{row1, row2};

    {
        CsvWriteOptions options{};
        options.write_header = true;

        CsvWriter writer{options};
        std::ostringstream output{};
        writer.write_rows(output, rows);

        const std::string text = output.str();
        assert(text.find("count,label,enabled\n") == 0U);
        assert(text.find("1,\"alpha,beta\",true\n") != std::string::npos);
        assert(text.find("2,\"line \"\"x\"\"\",false\n") != std::string::npos);
    }

    const std::filesystem::path file_path = std::filesystem::temp_directory_path() / "cpu_lab_csv_writer_test.csv";
    std::error_code ignore{};
    std::filesystem::remove(file_path, ignore);

    {
        CsvWriteOptions options{};
        options.atomic_write = true;
        options.write_header = true;

        CsvWriter writer{options};
        writer.write_rows_to_file(file_path.string(), rows);

        CsvReader reader{CsvReadOptions{}};
        const auto loaded_rows = reader.read_rows_from_file<WriteExampleRow>(file_path.string());
        assert(loaded_rows.size() == 2U);
        assert(loaded_rows[0].count == 1);
        assert(loaded_rows[0].label == "alpha,beta");
        assert(loaded_rows[0].enabled);
    }

    {
        CsvWriteOptions options{};
        options.append = true;
        options.write_header = true;

        CsvWriter writer{options};
        writer.write_rows_to_file(file_path.string(), rows);

        CsvReader reader{CsvReadOptions{}};
        const auto loaded_rows = reader.read_rows_from_file<WriteExampleRow>(file_path.string());
        assert(loaded_rows.size() == 4U);
    }

    std::filesystem::remove(file_path, ignore);
    return 0;
}
