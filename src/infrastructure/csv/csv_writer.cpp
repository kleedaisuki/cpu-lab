#include "infrastructure/csv/csv_writer.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

namespace cpu_lab::infrastructure::csv
{

    namespace
    {

        /** @brief 文件锁映射互斥量（map mutex）；Protect global file lock map. */
        std::mutex g_file_map_mutex{};

        /** @brief 文件锁映射（file lock map）；Per-file mutex map for write serialization. */
        std::unordered_map<std::string, std::shared_ptr<std::mutex>> g_file_mutex_map{};

        /**
         * @brief 获取文件互斥量（get file mutex）；Get/create mutex bound to one file path.
         * @param file_path 文件路径（file path）。
         * @return 对应的互斥量引用（mutex reference）。
         */
        [[nodiscard]] std::mutex &get_file_mutex(const std::string &file_path)
        {
            const std::lock_guard<std::mutex> guard{g_file_map_mutex};
            auto it = g_file_mutex_map.find(file_path);
            if (it != g_file_mutex_map.end())
            {
                return *(it->second);
            }

            auto inserted = g_file_mutex_map.emplace(file_path, std::make_shared<std::mutex>());
            return *(inserted.first->second);
        }

        /**
         * @brief 写入一条 CSV 记录（write one record）；Write one record with escaping.
         * @param output 输出流（output stream）。
         * @param record 记录单元格（record cells）。
         * @param options 写入配置（write options）。
         * @return 无返回值（no return value）。
         */
        void write_record(
            std::ostream &output,
            const std::vector<std::string> &record,
            const CsvWriteOptions &options)
        {
            for (std::size_t index = 0U; index < record.size(); ++index)
            {
                if (index > 0U)
                {
                    output.put(options.delimiter);
                }
                output << detail::escape_csv_cell(record[index], options.delimiter, options.quote_all);
            }
            output.put('\n');
        }

        /**
         * @brief 判断文件是否为空（empty file check）；Check file exists and has zero size.
         * @param path 文件路径（file path）。
         * @return 是否为空文件（is empty file）。
         */
        [[nodiscard]] bool is_empty_file(const std::filesystem::path &path)
        {
            std::error_code error{};
            if (!std::filesystem::exists(path, error))
            {
                return false;
            }
            return std::filesystem::file_size(path, error) == 0U;
        }

    } // namespace

    namespace detail
    {

        std::string stringify_cell(const std::string &value)
        {
            return value;
        }

        std::string stringify_cell(const std::string_view value)
        {
            return std::string{value};
        }

        std::string stringify_cell(const char *value)
        {
            if (value == nullptr)
            {
                return {};
            }
            return std::string{value};
        }

        std::string escape_csv_cell(const std::string_view raw_cell, const char delimiter, const bool quote_all)
        {
            bool must_quote = quote_all;
            for (const char ch : raw_cell)
            {
                if (ch == delimiter || ch == '"' || ch == '\n' || ch == '\r')
                {
                    must_quote = true;
                    break;
                }
            }

            if (!must_quote)
            {
                return std::string{raw_cell};
            }

            std::string escaped{};
            escaped.reserve(raw_cell.size() + 2U);
            escaped.push_back('"');

            for (const char ch : raw_cell)
            {
                if (ch == '"')
                {
                    escaped.push_back('"');
                }
                escaped.push_back(ch);
            }

            escaped.push_back('"');
            return escaped;
        }

    } // namespace detail

    CsvWriter::CsvWriter(const CsvWriteOptions options) noexcept
        : options_{options}
    {
    }

    void CsvWriter::write_table(
        std::ostream &output,
        const std::vector<std::string> &header,
        const std::vector<std::vector<std::string>> &records) const
    {
        if (options_.write_header && !header.empty())
        {
            write_record(output, header, options_);
        }

        for (const auto &record : records)
        {
            write_record(output, record, options_);
        }

        if (!output.good())
        {
            throw std::runtime_error("failed to write CSV data to stream");
        }
    }

    void CsvWriter::write_table_to_file(
        const std::string &file_path,
        const std::vector<std::string> &header,
        const std::vector<std::vector<std::string>> &records) const
    {
        const std::filesystem::path path = std::filesystem::path{file_path};
        const std::filesystem::path absolute_path = std::filesystem::absolute(path);
        const std::string normalized_path = absolute_path.lexically_normal().string();

        auto &file_mutex = get_file_mutex(normalized_path);
        const std::lock_guard<std::mutex> lock{file_mutex};

        if (options_.append)
        {
            const bool existed_before = std::filesystem::exists(absolute_path);
            const bool write_header_now = options_.write_header && (!existed_before || is_empty_file(absolute_path));

            std::ofstream output{absolute_path, std::ios::out | std::ios::app};
            if (!output.is_open())
            {
                throw std::runtime_error("failed to open CSV file for append: '" + normalized_path + "'");
            }

            if (write_header_now)
            {
                write_record(output, header, options_);
            }
            for (const auto &record : records)
            {
                write_record(output, record, options_);
            }

            if (!output.good())
            {
                throw std::runtime_error("failed to append CSV file: '" + normalized_path + "'");
            }
            return;
        }

        if (!options_.atomic_write)
        {
            std::ofstream output{absolute_path, std::ios::out | std::ios::trunc};
            if (!output.is_open())
            {
                throw std::runtime_error("failed to open CSV file for writing: '" + normalized_path + "'");
            }

            write_table(output, header, records);
            return;
        }

        const std::filesystem::path temp_path = absolute_path.string() + ".tmp";
        {
            std::ofstream temp_output{temp_path, std::ios::out | std::ios::trunc};
            if (!temp_output.is_open())
            {
                throw std::runtime_error("failed to open temporary CSV file: '" + temp_path.string() + "'");
            }

            write_table(temp_output, header, records);
        }

        std::error_code remove_error{};
        std::filesystem::remove(absolute_path, remove_error);

        std::error_code rename_error{};
        std::filesystem::rename(temp_path, absolute_path, rename_error);
        if (rename_error)
        {
            std::error_code cleanup_error{};
            std::filesystem::remove(temp_path, cleanup_error);
            throw std::runtime_error("failed to commit CSV file: '" + normalized_path + "'");
        }
    }

} // namespace cpu_lab::infrastructure::csv
