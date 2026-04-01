#include "domain/sum_reduce/sum_generator.hpp"

#include "infrastructure/csv/row.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cpu_lab::domain::sum_reduce
{

    auto SumReduceTestCaseRow::meta() noexcept
        -> std::tuple<
            infrastructure::csv::RowField<SumReduceTestCaseRow, std::string>,
            infrastructure::csv::RowField<SumReduceTestCaseRow, std::string>,
            infrastructure::csv::RowField<SumReduceTestCaseRow, std::size_t>,
            infrastructure::csv::RowField<SumReduceTestCaseRow, double>,
            infrastructure::csv::RowField<SumReduceTestCaseRow, double>,
            infrastructure::csv::RowField<SumReduceTestCaseRow, std::size_t>,
            infrastructure::csv::RowField<SumReduceTestCaseRow, std::string>>
    {
        using infrastructure::csv::make_row_field;

        return std::make_tuple(
            make_row_field("case_id", &SumReduceTestCaseRow::case_id),
            make_row_field("algorithm", &SumReduceTestCaseRow::algorithm),
            make_row_field("length", &SumReduceTestCaseRow::length),
            make_row_field("value_base", &SumReduceTestCaseRow::value_base),
            make_row_field("value_step", &SumReduceTestCaseRow::value_step),
            make_row_field("superscalar_lanes", &SumReduceTestCaseRow::superscalar_lanes),
            make_row_field("notes", &SumReduceTestCaseRow::notes));
    }

    std::size_t SumReduceCtorArgs::problem_size() const noexcept
    {
        return values.size();
    }

    bool SumReduceCtorArgsGenerator::validate_row(const SumReduceTestCaseRow &row) noexcept
    {
        if (row.case_id.empty() || row.algorithm.empty())
        {
            return false;
        }

        if (row.length == 0U)
        {
            return false;
        }

        if (row.superscalar_lanes == 0U)
        {
            return false;
        }

        return true;
    }

    SumReduceAlgorithm SumReduceCtorArgsGenerator::parse_algorithm(const std::string_view algorithm)
    {
        std::string normalized(algorithm);
        std::transform(
            normalized.begin(),
            normalized.end(),
            normalized.begin(),
            [](const unsigned char ch) -> char
            {
                return static_cast<char>(std::tolower(ch));
            });

        if (normalized == "naive")
        {
            return SumReduceAlgorithm::Naive;
        }

        if (normalized == "superscalar")
        {
            return SumReduceAlgorithm::Superscalar;
        }

        throw std::invalid_argument("unknown sum_reduce algorithm: '" + std::string(algorithm) + "'.");
    }

    SumReduceCtorArgs SumReduceCtorArgsGenerator::make_ctor_args(const SumReduceTestCaseRow &row)
    {
        if (!validate_row(row))
        {
            throw std::invalid_argument("SumReduceCtorArgsGenerator got invalid test-case row.");
        }

        SumReduceCtorArgs args{};
        args.case_id = row.case_id;
        args.algorithm = parse_algorithm(row.algorithm);
        args.values = build_values(row);
        args.superscalar_lanes = row.superscalar_lanes;
        args.notes = row.notes;
        return args;
    }

    std::vector<SumReduceCtorArgs> SumReduceCtorArgsGenerator::make_ctor_args_list(
        const std::vector<SumReduceTestCaseRow> &rows)
    {
        std::vector<SumReduceCtorArgs> args_list{};
        args_list.reserve(rows.size());

        for (const SumReduceTestCaseRow &row : rows)
        {
            args_list.push_back(make_ctor_args(row));
        }

        return args_list;
    }

    std::vector<double> SumReduceCtorArgsGenerator::build_values(const SumReduceTestCaseRow &row)
    {
        std::vector<double> values(row.length, 0.0);
        for (std::size_t index = 0U; index < row.length; ++index)
        {
            values[index] = row.value_base + (static_cast<double>(index) * row.value_step);
        }
        return values;
    }

} // namespace cpu_lab::domain::sum_reduce