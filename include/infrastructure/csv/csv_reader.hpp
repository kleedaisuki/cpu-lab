#pragma once

#include "infrastructure/csv/row.hpp"

#include <charconv>
#include <cstddef>
#include <istream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace cpu_lab::infrastructure::csv
{

    /**
     * @brief CSV 读取配置（CSV read options）；Options controlling CSV parsing behavior.
     */
    struct CsvReadOptions
    {
        /** @brief 分隔符（delimiter）；CSV column delimiter. */
        char delimiter{','};

        /** @brief 是否包含表头（has header）；Whether first record is header. */
        bool has_header{true};

        /** @brief 是否去除首尾空白（trim whitespace）；Trim surrounding ASCII spaces. */
        bool trim_whitespace{true};

        /** @brief 是否跳过空行（skip empty lines）；Skip records with all-empty cells. */
        bool skip_empty_lines{true};
    };

    /**
     * @brief 解析后的 CSV 表（parsed CSV table）；Parsed CSV table with optional header.
     */
    struct CsvTable
    {
        /** @brief 表头列名（header columns）；Header names when present. */
        std::vector<std::string> header{};

        /** @brief 数据记录（records）；Data records without header row. */
        std::vector<std::vector<std::string>> records{};
    };

    namespace detail
    {

        /**
         * @brief 去除 ASCII 首尾空白（trim ASCII whitespace）；Trim leading/trailing ASCII whitespaces.
         * @param text 输入文本（input text）。
         * @return 去空白后的视图（trimmed view）。
         */
        [[nodiscard]] std::string_view trim_ascii(std::string_view text) noexcept;

        /**
         * @brief 将字符串转换为目标类型（cell conversion）；Convert CSV cell text to target type.
         * @tparam ValueT 目标值类型（target value type）。
         * @param cell 单元格文本（cell text）。
         * @return 转换后的值（converted value）。
         * @note 当转换失败时抛出 std::runtime_error（throw on conversion failure）。
         */
        template <typename ValueT>
        [[nodiscard]] ValueT parse_cell(std::string_view cell)
        {
            using DecayedType = std::remove_cv_t<std::remove_reference_t<ValueT>>;

            if constexpr (std::is_same_v<DecayedType, std::string>)
            {
                return std::string{cell};
            }
            else if constexpr (std::is_same_v<DecayedType, bool>)
            {
                if (cell == "1" || cell == "true" || cell == "TRUE" || cell == "True")
                {
                    return true;
                }
                if (cell == "0" || cell == "false" || cell == "FALSE" || cell == "False")
                {
                    return false;
                }
                throw std::runtime_error("failed to parse bool cell: '" + std::string{cell} + "'");
            }
            else if constexpr (std::is_integral_v<DecayedType>)
            {
                DecayedType value{};
                const char *const begin = cell.data();
                const char *const end = cell.data() + cell.size();
                const auto result = std::from_chars(begin, end, value);
                if (result.ec != std::errc{} || result.ptr != end)
                {
                    throw std::runtime_error("failed to parse integral cell: '" + std::string{cell} + "'");
                }
                return value;
            }
            else if constexpr (std::is_floating_point_v<DecayedType>)
            {
                DecayedType value{};
                const char *const begin = cell.data();
                const char *const end = cell.data() + cell.size();
                const auto result = std::from_chars(begin, end, value);
                if (result.ec != std::errc{} || result.ptr != end)
                {
                    throw std::runtime_error("failed to parse floating cell: '" + std::string{cell} + "'");
                }
                return value;
            }
            else
            {
                static_assert(!std::is_same_v<DecayedType, DecayedType>, "unsupported CSV cell target type");
            }
        }

        /**
         * @brief 构建表头索引（header index）；Build name -> index map from header row.
         * @param header 表头列名（header columns）。
         * @return 表头索引映射（header index map）。
         */
        [[nodiscard]] std::unordered_map<std::string, std::size_t> build_header_index(
            const std::vector<std::string> &header);

        /**
         * @brief 按字段顺序将记录映射为 Row（row mapping）；Map one CSV record into one Row object.
         * @tparam RowT 行类型（row type）。
         * @param record 单条记录（record cells）。
         * @param header_index 表头索引（header index map）。
         * @param has_header 是否使用表头映射（map by header names）。
         * @return 填充后的行对象（materialized row）。
         */
        template <typename RowT>
        [[nodiscard]] RowT record_to_row(
            const std::vector<std::string> &record,
            const std::unordered_map<std::string, std::size_t> &header_index,
            const bool has_header)
        {
            RowT row{};
            std::size_t ordinal_index{0U};

            row.for_each_field(
                [&record, &header_index, has_header, &ordinal_index](const std::string_view name, auto &value)
                {
                    std::size_t column_index = ordinal_index;
                    if (has_header)
                    {
                        const auto it = header_index.find(std::string{name});
                        if (it == header_index.end())
                        {
                            throw std::runtime_error("missing CSV column: '" + std::string{name} + "'");
                        }
                        column_index = it->second;
                    }

                    if (column_index >= record.size())
                    {
                        throw std::runtime_error("record column index out of range: " + std::to_string(column_index));
                    }

                    value = parse_cell<std::remove_reference_t<decltype(value)>>(record[column_index]);
                    ++ordinal_index;
                });

            return row;
        }

    } // namespace detail

    /**
     * @brief CSV 读取器（CSV reader）；Read CSV into generic table or reflected Row objects.
     */
    class CsvReader final
    {
    public:
        /**
         * @brief 构造读取器（construct reader）；Create reader with options.
         * @param options 读取选项（read options）。
         */
        explicit CsvReader(CsvReadOptions options = {}) noexcept;

        /**
         * @brief 从输入流读取 CSV 表（read table from stream）；Parse CSV from std::istream.
         * @param input 输入流（input stream）。
         * @return 解析后的表对象（parsed table）。
         */
        [[nodiscard]] CsvTable read_table(std::istream &input) const;

        /**
         * @brief 从文件读取 CSV 表（read table from file）；Parse CSV from file path.
         * @param file_path 文件路径（file path）。
         * @return 解析后的表对象（parsed table）。
         */
        [[nodiscard]] CsvTable read_table_from_file(const std::string &file_path) const;

        /**
         * @brief 从输入流读取 Row 列表（read rows from stream）；Parse stream and map to reflected rows.
         * @tparam RowT 行类型（row type），需继承 Row 并实现 meta().
         * @param input 输入流（input stream）。
         * @return 读取到的行对象列表（row vector）。
         */
        template <typename RowT>
        [[nodiscard]] std::vector<RowT> read_rows(std::istream &input) const
        {
            const CsvTable table = read_table(input);

            std::unordered_map<std::string, std::size_t> header_index{};
            if (options_.has_header)
            {
                header_index = detail::build_header_index(table.header);
            }

            std::vector<RowT> rows{};
            rows.reserve(table.records.size());
            for (const auto &record : table.records)
            {
                rows.push_back(detail::record_to_row<RowT>(record, header_index, options_.has_header));
            }

            return rows;
        }

        /**
         * @brief 从文件读取 Row 列表（read rows from file）；Parse file and map to reflected rows.
         * @tparam RowT 行类型（row type），需继承 Row 并实现 meta().
         * @param file_path 文件路径（file path）。
         * @return 读取到的行对象列表（row vector）。
         */
        template <typename RowT>
        [[nodiscard]] std::vector<RowT> read_rows_from_file(const std::string &file_path) const
        {
            const CsvTable table = read_table_from_file(file_path);

            std::unordered_map<std::string, std::size_t> header_index{};
            if (options_.has_header)
            {
                header_index = detail::build_header_index(table.header);
            }

            std::vector<RowT> rows{};
            rows.reserve(table.records.size());
            for (const auto &record : table.records)
            {
                rows.push_back(detail::record_to_row<RowT>(record, header_index, options_.has_header));
            }

            return rows;
        }

    private:
        /** @brief 读取选项（read options）；Reader configuration. */
        CsvReadOptions options_{};
    };

} // namespace cpu_lab::infrastructure::csv
