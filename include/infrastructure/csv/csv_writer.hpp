#pragma once

#include "infrastructure/csv/row.hpp"

#include <cstddef>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace cpu_lab::infrastructure::csv
{

    /**
     * @brief CSV 写入配置（CSV write options）；Options controlling CSV writing behavior.
     */
    struct CsvWriteOptions
    {
        /** @brief 分隔符（delimiter）；CSV column delimiter. */
        char delimiter{','};

        /** @brief 是否写表头（write header）；Whether to emit header row. */
        bool write_header{true};

        /** @brief 是否追加写入（append mode）；Append to existing file instead of truncating. */
        bool append{false};

        /** @brief 是否启用原子写（atomic write）；Use temp-file commit for non-append writes. */
        bool atomic_write{true};

        /** @brief 是否总是加引号（quote all）；Force quote for every CSV cell. */
        bool quote_all{false};
    };

    namespace detail
    {

        /**
         * @brief 规范化单元格文本（stringify cell）；Convert typed value to plain cell text.
         * @param value 输入值（input value）。
         * @return 纯文本单元格（plain text cell）。
         */
        [[nodiscard]] std::string stringify_cell(const std::string &value);

        /**
         * @brief 规范化单元格文本（stringify cell）；Convert typed value to plain cell text.
         * @param value 输入值（input value）。
         * @return 纯文本单元格（plain text cell）。
         */
        [[nodiscard]] std::string stringify_cell(std::string_view value);

        /**
         * @brief 规范化单元格文本（stringify cell）；Convert typed value to plain cell text.
         * @param value 输入值（input value）。
         * @return 纯文本单元格（plain text cell）。
         */
        [[nodiscard]] std::string stringify_cell(const char *value);

        /**
         * @brief 转义 CSV 单元格（CSV escaping）；Escape/quote one cell for CSV output.
         * @param raw_cell 原始单元格文本（raw cell text）。
         * @param delimiter 分隔符（delimiter）。
         * @param quote_all 是否强制加引号（force quote）。
         * @return 可写入 CSV 的单元格文本（CSV-safe cell text）。
         */
        [[nodiscard]] std::string escape_csv_cell(
            std::string_view raw_cell,
            char delimiter,
            bool quote_all);

        /**
         * @brief 规范化单元格文本（stringify cell）；Convert typed value to plain cell text.
         * @tparam ValueT 值类型（value type）。
         * @param value 输入值（input value）。
         * @return 纯文本单元格（plain text cell）。
         */
        template <typename ValueT>
        [[nodiscard]] std::string stringify_cell(const ValueT &value)
        {
            using DecayedType = std::remove_cv_t<std::remove_reference_t<ValueT>>;

            if constexpr (std::is_same_v<DecayedType, bool>)
            {
                return value ? "true" : "false";
            }
            else if constexpr (std::is_integral_v<DecayedType>)
            {
                return std::to_string(value);
            }
            else if constexpr (std::is_floating_point_v<DecayedType>)
            {
                std::ostringstream stream{};
                stream.precision(std::numeric_limits<DecayedType>::max_digits10);
                stream << value;
                return stream.str();
            }
            else
            {
                static_assert(!std::is_same_v<DecayedType, DecayedType>, "unsupported CSV cell source type");
            }
        }

    } // namespace detail

    /**
     * @brief CSV 写入器（CSV writer）；Write reflected Row objects or raw tables into CSV.
     */
    class CsvWriter final
    {
    public:
        /**
         * @brief 构造写入器（construct writer）；Create writer with options.
         * @param options 写入配置（write options）。
         */
        explicit CsvWriter(CsvWriteOptions options = {}) noexcept;

        /**
         * @brief 写入原始表到流（write raw table）；Write header/records as CSV text.
         * @param output 输出流（output stream）。
         * @param header 表头列名（header columns）。
         * @param records 数据记录（records）。
         * @return 无返回值（no return value）。
         */
        void write_table(
            std::ostream &output,
            const std::vector<std::string> &header,
            const std::vector<std::vector<std::string>> &records) const;

        /**
         * @brief 写入原始表到文件（write raw table to file）；Persist CSV to file path.
         * @param file_path 文件路径（file path）。
         * @param header 表头列名（header columns）。
         * @param records 数据记录（records）。
         * @return 无返回值（no return value）。
         */
        void write_table_to_file(
            const std::string &file_path,
            const std::vector<std::string> &header,
            const std::vector<std::vector<std::string>> &records) const;

        /**
         * @brief 写入 Row 列表到流（write rows to stream）；Serialize reflected rows into CSV.
         * @tparam RowT 行类型（row type）。
         * @param output 输出流（output stream）。
         * @param rows 行对象列表（row vector）。
         * @return 无返回值（no return value）。
         */
        template <typename RowT>
        void write_rows(std::ostream &output, const std::vector<RowT> &rows) const
        {
            std::vector<std::string> header{};
            if (options_.write_header)
            {
                header = RowT::header();
            }

            std::vector<std::vector<std::string>> records{};
            records.reserve(rows.size());
            for (const auto &row : rows)
            {
                records.push_back(serialize_row(row));
            }

            write_table(output, header, records);
        }

        /**
         * @brief 写入 Row 列表到文件（write rows to file）；Serialize reflected rows to file.
         * @tparam RowT 行类型（row type）。
         * @param file_path 文件路径（file path）。
         * @param rows 行对象列表（row vector）。
         * @return 无返回值（no return value）。
         */
        template <typename RowT>
        void write_rows_to_file(const std::string &file_path, const std::vector<RowT> &rows) const
        {
            std::vector<std::string> header{};
            if (options_.write_header)
            {
                header = RowT::header();
            }

            std::vector<std::vector<std::string>> records{};
            records.reserve(rows.size());
            for (const auto &row : rows)
            {
                records.push_back(serialize_row(row));
            }

            write_table_to_file(file_path, header, records);
        }

    private:
        /**
         * @brief 序列化单行对象（serialize one row）；Convert reflected row into raw cell strings.
         * @tparam RowT 行类型（row type）。
         * @param row 行对象（row object）。
         * @return 单行单元格数组（record cells）。
         */
        template <typename RowT>
        [[nodiscard]] std::vector<std::string> serialize_row(const RowT &row) const
        {
            std::vector<std::string> record{};
            record.reserve(RowT::field_count());

            row.for_each_field(
                [&record](const std::string_view /*name*/, const auto &value)
                {
                    record.push_back(detail::stringify_cell(value));
                });

            return record;
        }

        /** @brief 写入选项（write options）；Writer configuration. */
        CsvWriteOptions options_{};
    };

} // namespace cpu_lab::infrastructure::csv
