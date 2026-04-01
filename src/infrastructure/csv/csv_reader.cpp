#include "infrastructure/csv/csv_reader.hpp"

#include <charconv>
#include <fstream>
#include <stdexcept>

namespace cpu_lab::infrastructure::csv
{

    namespace
    {

        /**
         * @brief 判断记录是否为空（empty record check）；Check whether all cells are empty.
         * @param record 记录单元格（record cells）。
         * @return 是否为空记录（is empty record）。
         */
        [[nodiscard]] bool is_empty_record(const std::vector<std::string> &record)
        {
            for (const auto &cell : record)
            {
                if (!cell.empty())
                {
                    return false;
                }
            }
            return true;
        }

        /**
         * @brief 解析下一条 CSV 记录（record parser）；Parse one CSV record from stream.
         * @param input 输入流（input stream）。
         * @param delimiter 分隔符（delimiter）。
         * @param record 输出记录（output record）。
         * @return 是否成功读取记录（true if one record parsed）。
         * @note 支持双引号字段与双引号转义（supports quoted cell and escaped quotes）。
         */
        [[nodiscard]] bool read_next_record(
            std::istream &input,
            const char delimiter,
            std::vector<std::string> &record)
        {
            record.clear();

            std::string cell{};
            bool in_quotes{false};
            bool has_any_character{false};

            while (true)
            {
                const int raw = input.get();
                if (raw == EOF)
                {
                    if (in_quotes)
                    {
                        throw std::runtime_error("unterminated quoted CSV field");
                    }

                    if (!has_any_character)
                    {
                        return false;
                    }

                    record.push_back(cell);
                    return true;
                }

                has_any_character = true;
                const char ch = static_cast<char>(raw);

                if (in_quotes)
                {
                    if (ch == '"')
                    {
                        if (input.peek() == '"')
                        {
                            static_cast<void>(input.get());
                            cell.push_back('"');
                            continue;
                        }
                        in_quotes = false;
                        continue;
                    }

                    cell.push_back(ch);
                    continue;
                }

                if (ch == '"')
                {
                    in_quotes = true;
                    continue;
                }

                if (ch == delimiter)
                {
                    record.push_back(cell);
                    cell.clear();
                    continue;
                }

                if (ch == '\n')
                {
                    record.push_back(cell);
                    return true;
                }

                if (ch == '\r')
                {
                    if (input.peek() == '\n')
                    {
                        static_cast<void>(input.get());
                    }
                    record.push_back(cell);
                    return true;
                }

                cell.push_back(ch);
            }
        }

    } // namespace

    namespace detail
    {

        std::string_view trim_ascii(std::string_view text) noexcept
        {
            std::size_t begin = 0U;
            std::size_t end = text.size();

            while (begin < end)
            {
                const char ch = text[begin];
                if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v')
                {
                    ++begin;
                    continue;
                }
                break;
            }

            while (end > begin)
            {
                const char ch = text[end - 1U];
                if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v')
                {
                    --end;
                    continue;
                }
                break;
            }

            return text.substr(begin, end - begin);
        }

        std::unordered_map<std::string, std::size_t> build_header_index(
            const std::vector<std::string> &header)
        {
            std::unordered_map<std::string, std::size_t> index{};
            index.reserve(header.size());

            for (std::size_t i = 0U; i < header.size(); ++i)
            {
                const auto [_, inserted] = index.emplace(header[i], i);
                if (!inserted)
                {
                    throw std::runtime_error("duplicated CSV header: '" + header[i] + "'");
                }
            }

            return index;
        }

    } // namespace detail

    CsvReader::CsvReader(const CsvReadOptions options) noexcept
        : options_{options}
    {
    }

    CsvTable CsvReader::read_table(std::istream &input) const
    {
        CsvTable table{};
        std::vector<std::string> record{};

        bool is_first_record = true;
        while (read_next_record(input, options_.delimiter, record))
        {
            if (options_.trim_whitespace)
            {
                for (auto &cell : record)
                {
                    const std::string_view trimmed = detail::trim_ascii(cell);
                    cell.assign(trimmed.data(), trimmed.size());
                }
            }

            if (is_first_record && options_.has_header)
            {
                if (!record.empty() && record[0].size() >= 3U &&
                    static_cast<unsigned char>(record[0][0]) == 0xEFU &&
                    static_cast<unsigned char>(record[0][1]) == 0xBBU &&
                    static_cast<unsigned char>(record[0][2]) == 0xBFU)
                {
                    record[0].erase(0U, 3U);
                }
                table.header = record;
                is_first_record = false;
                continue;
            }

            is_first_record = false;
            if (options_.skip_empty_lines && is_empty_record(record))
            {
                continue;
            }

            table.records.push_back(record);
        }

        return table;
    }

    CsvTable CsvReader::read_table_from_file(const std::string &file_path) const
    {
        std::ifstream input{file_path};
        if (!input.is_open())
        {
            throw std::runtime_error("failed to open CSV file: '" + file_path + "'");
        }

        return read_table(input);
    }

} // namespace cpu_lab::infrastructure::csv

