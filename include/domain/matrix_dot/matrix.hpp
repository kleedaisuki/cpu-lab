#pragma once

#include <cstddef>
#include <vector>

namespace cpu_lab::domain::matrix_dot
{

    /**
     * @brief 二维矩阵容器（matrix container）；Contiguous row-major numeric matrix.
     */
    class Matrix final
    {
    public:
        /**
         * @brief 默认构造函数（default constructor）；Construct an empty matrix.
         */
        Matrix() = default;

        /**
         * @brief 按维度构造矩阵（shape constructor）；Construct matrix with shape and fill value.
         * @param rows 行数（row count）。
         * @param cols 列数（column count）。
         * @param value 初始值（initial value）。
         */
        Matrix(std::size_t rows, std::size_t cols, double value = 0.0);

        /**
         * @brief 从存储构造矩阵（storage constructor）；Construct matrix from explicit storage.
         * @param rows 行数（row count）。
         * @param cols 列数（column count）。
         * @param values 行主序存储（row-major storage）。
         * @note 若 values.size() != rows * cols 则抛出 std::invalid_argument（throws std::invalid_argument on shape mismatch）。
         */
        Matrix(std::size_t rows, std::size_t cols, std::vector<double> values);

        /**
         * @brief 获取行数（rows）；Return row count.
         * @return 行数（row count）。
         */
        [[nodiscard]] std::size_t rows() const noexcept;

        /**
         * @brief 获取列数（cols）；Return column count.
         * @return 列数（column count）。
         */
        [[nodiscard]] std::size_t cols() const noexcept;

        /**
         * @brief 获取总元素数（size）；Return number of scalar elements.
         * @return 元素总数（number of elements）。
         */
        [[nodiscard]] std::size_t size() const noexcept;

        /**
         * @brief 判空（empty）；Check whether matrix has no elements.
         * @return 空矩阵返回 true（true if empty）。
         */
        [[nodiscard]] bool empty() const noexcept;

        /**
         * @brief 边界检查访问（checked access）；Bounds-checked element access.
         * @param row 行索引（row index）。
         * @param col 列索引（column index）。
         * @return 元素引用（element reference）。
         * @note 索引越界抛出 std::out_of_range（throws std::out_of_range on invalid index）。
         */
        [[nodiscard]] double &at(std::size_t row, std::size_t col);

        /**
         * @brief 边界检查访问（checked access）；Bounds-checked element access.
         * @param row 行索引（row index）。
         * @param col 列索引（column index）。
         * @return 元素常量引用（const element reference）。
         * @note 索引越界抛出 std::out_of_range（throws std::out_of_range on invalid index）。
         */
        [[nodiscard]] const double &at(std::size_t row, std::size_t col) const;

        /**
         * @brief 无边界检查访问（unchecked access）；Unchecked element access.
         * @param row 行索引（row index）。
         * @param col 列索引（column index）。
         * @return 元素引用（element reference）。
         */
        [[nodiscard]] double &operator()(std::size_t row, std::size_t col) noexcept;

        /**
         * @brief 无边界检查访问（unchecked access）；Unchecked element access.
         * @param row 行索引（row index）。
         * @param col 列索引（column index）。
         * @return 元素常量引用（const element reference）。
         */
        [[nodiscard]] const double &operator()(std::size_t row, std::size_t col) const noexcept;

        /**
         * @brief 获取底层只读存储（const data）；Access immutable row-major storage.
         * @return 只读行主序数组（immutable row-major array）。
         */
        [[nodiscard]] const std::vector<double> &data() const noexcept;

        /**
         * @brief 获取底层可写存储（mutable data）；Access mutable row-major storage.
         * @return 可写行主序数组（mutable row-major array）。
         */
        [[nodiscard]] std::vector<double> &data() noexcept;

        /**
         * @brief 批量填充值（fill）；Assign one value to all elements.
         * @param value 填充值（fill value）。
         * @return 无返回值；No return value.
         */
        void fill(double value) noexcept;

    private:
        /** @brief 行数（row count）；Number of rows. */
        std::size_t rows_{0U};

        /** @brief 列数（column count）；Number of columns. */
        std::size_t cols_{0U};

        /** @brief 行主序存储（row-major storage）；Owned contiguous matrix values. */
        std::vector<double> values_{};

        /**
         * @brief 线性索引换算（linear index）；Convert 2D index to row-major linear index.
         * @param row 行索引（row index）。
         * @param col 列索引（column index）。
         * @return 线性索引（linear index）。
         */
        [[nodiscard]] std::size_t linear_index(std::size_t row, std::size_t col) const noexcept;
    };

} // namespace cpu_lab::domain::matrix_dot
