#include "infrastructure/csv/csv_reader.hpp"

#include <cassert>
#include <cmath>
#include <sstream>
#include <string>
#include <tuple>

using cpu_lab::infrastructure::csv::CsvReader;
using cpu_lab::infrastructure::csv::CsvReadOptions;
using cpu_lab::infrastructure::csv::make_row_field;

/**
 * @brief 用于 CSV 读取测试的行类型（test row）；Row model for CSV reader test.
 */
struct ExampleRow final
{
    /** @brief 计数字段（count field）；Integral field. */
    int count{0};

    /** @brief 比例字段（ratio field）；Floating-point field. */
    double ratio{0.0};

    /** @brief 标签字段（label field）；String field. */
    std::string label{};

    /** @brief 布尔字段（enabled field）；Boolean field. */
    bool enabled{false};

    /**
     * @brief 返回字段元信息（metadata）；Field reflection metadata.
     * @return 元组字段描述（tuple descriptors）。
     */
    [[nodiscard]] static constexpr auto meta() noexcept
    {
        return std::make_tuple(
            make_row_field("count", &ExampleRow::count),
            make_row_field("ratio", &ExampleRow::ratio),
            make_row_field("label", &ExampleRow::label),
            make_row_field("enabled", &ExampleRow::enabled));
    }
};

int main()
{
    const std::string csv =
        "ratio,label,enabled,count\n"
        "3.5,\"alpha, beta\",true,42\n"
        "1.25,\"line \"\"x\"\"\",0,7\n";

    CsvReadOptions options{};
    options.has_header = true;
    options.trim_whitespace = true;

    CsvReader reader{options};
    std::istringstream input{csv};

    const auto rows = reader.read_rows<ExampleRow>(input);
    assert(rows.size() == 2U);

    assert(rows[0].count == 42);
    assert(std::abs(rows[0].ratio - 3.5) < 1e-12);
    assert(rows[0].label == "alpha, beta");
    assert(rows[0].enabled);

    assert(rows[1].count == 7);
    assert(std::abs(rows[1].ratio - 1.25) < 1e-12);
    assert(rows[1].label == "line \"x\"");
    assert(!rows[1].enabled);

    return 0;
}