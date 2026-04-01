#include "domain/matrix_dot/matrix_generator.hpp"

#include "infrastructure/csv/row.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cpu_lab::domain::matrix_dot
{

    auto MatrixDotTestCaseRow::meta() noexcept
        -> std::tuple<
            infrastructure::csv::RowField<MatrixDotTestCaseRow, std::string>,
            infrastructure::csv::RowField<MatrixDotTestCaseRow, std::string>,
            infrastructure::csv::RowField<MatrixDotTestCaseRow, std::size_t>,
            infrastructure::csv::RowField<MatrixDotTestCaseRow, std::size_t>,
            infrastructure::csv::RowField<MatrixDotTestCaseRow, double>,
            infrastructure::csv::RowField<MatrixDotTestCaseRow, double>,
            infrastructure::csv::RowField<MatrixDotTestCaseRow, double>,
            infrastructure::csv::RowField<MatrixDotTestCaseRow, double>,
            infrastructure::csv::RowField<MatrixDotTestCaseRow, double>,
            infrastructure::csv::RowField<MatrixDotTestCaseRow, std::size_t>,
            infrastructure::csv::RowField<MatrixDotTestCaseRow, std::string>>
    {
        using infrastructure::csv::make_row_field;

        return std::make_tuple(
            make_row_field("case_id", &MatrixDotTestCaseRow::case_id),
            make_row_field("algorithm", &MatrixDotTestCaseRow::algorithm),
            make_row_field("rows", &MatrixDotTestCaseRow::rows),
            make_row_field("cols", &MatrixDotTestCaseRow::cols),
            make_row_field("matrix_base", &MatrixDotTestCaseRow::matrix_base),
            make_row_field("matrix_row_step", &MatrixDotTestCaseRow::matrix_row_step),
            make_row_field("matrix_col_step", &MatrixDotTestCaseRow::matrix_col_step),
            make_row_field("vector_base", &MatrixDotTestCaseRow::vector_base),
            make_row_field("vector_step", &MatrixDotTestCaseRow::vector_step),
            make_row_field("cache_block_cols", &MatrixDotTestCaseRow::cache_block_cols),
            make_row_field("notes", &MatrixDotTestCaseRow::notes));
    }

    std::size_t MatrixDotCtorArgs::problem_size() const noexcept
    {
        return matrix.rows() * matrix.cols();
    }

    bool MatrixDotCtorArgsGenerator::validate_row(const MatrixDotTestCaseRow &row) noexcept
    {
        if (row.case_id.empty() || row.algorithm.empty()) {
            return false;
        }

        if (row.rows == 0U || row.cols == 0U) {
            return false;
        }

        if (row.cache_block_cols == 0U) {
            return false;
        }

        return true;
    }

    MatrixDotAlgorithm MatrixDotCtorArgsGenerator::parse_algorithm(const std::string_view algorithm)
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

        if (normalized == "naive") {
            return MatrixDotAlgorithm::Naive;
        }

        if (normalized == "cache") {
            return MatrixDotAlgorithm::Cache;
        }

        throw std::invalid_argument("unknown matrix_dot algorithm: '" + std::string(algorithm) + "'.");
    }

    MatrixDotCtorArgs MatrixDotCtorArgsGenerator::make_ctor_args(const MatrixDotTestCaseRow &row)
    {
        if (!validate_row(row)) {
            throw std::invalid_argument("MatrixDotCtorArgsGenerator got invalid test-case row.");
        }

        MatrixDotCtorArgs args{};
        args.case_id = row.case_id;
        args.algorithm = parse_algorithm(row.algorithm);
        args.matrix = build_matrix(row);
        args.vector = build_vector(row);
        args.cache_block_cols = row.cache_block_cols;
        args.notes = row.notes;
        return args;
    }

    std::vector<MatrixDotCtorArgs> MatrixDotCtorArgsGenerator::make_ctor_args_list(
        const std::vector<MatrixDotTestCaseRow> &rows)
    {
        std::vector<MatrixDotCtorArgs> args_list{};
        args_list.reserve(rows.size());

        for (const MatrixDotTestCaseRow &row : rows) {
            args_list.push_back(make_ctor_args(row));
        }

        return args_list;
    }

    Matrix MatrixDotCtorArgsGenerator::build_matrix(const MatrixDotTestCaseRow &row)
    {
        Matrix matrix(row.rows, row.cols, 0.0);

        for (std::size_t r = 0U; r < row.rows; ++r) {
            for (std::size_t c = 0U; c < row.cols; ++c) {
                matrix(r, c) = row.matrix_base +
                    (static_cast<double>(r) * row.matrix_row_step) +
                    (static_cast<double>(c) * row.matrix_col_step);
            }
        }

        return matrix;
    }

    Vector MatrixDotCtorArgsGenerator::build_vector(const MatrixDotTestCaseRow &row)
    {
        Vector vector(row.rows, 0.0);
        for (std::size_t i = 0U; i < row.rows; ++i) {
            vector[i] = row.vector_base + (static_cast<double>(i) * row.vector_step);
        }
        return vector;
    }

} // namespace cpu_lab::domain::matrix_dot
