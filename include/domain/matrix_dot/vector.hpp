#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cpu_lab::domain::matrix_dot
{

    /**
     * @brief 一维向量容器（vector container）；Contiguous one-dimensional numeric vector.
     */
    class Vector final
    {
    public:
        /**
         * @brief 默认构造函数（default constructor）；Construct an empty vector.
         */
        Vector() = default;

        /**
         * @brief 按长度构造向量（size constructor）；Construct vector with fixed size and fill value.
         * @param size 向量长度（vector size）；Number of elements.
         * @param value 初始值（initial value）；Fill value for all elements.
         */
        explicit Vector(std::size_t size, double value = 0.0);

        /**
         * @brief 从存储构造向量（storage constructor）；Construct vector from owned storage.
         * @param values 元素存储（element storage）；Owned contiguous values.
         */
        explicit Vector(std::vector<double> values);

        /**
         * @brief 获取元素个数（size）；Return element count.
         * @return 向量长度（vector size）；Number of elements.
         */
        [[nodiscard]] std::size_t size() const noexcept;

        /**
         * @brief 判空（empty）；Check whether vector has no elements.
         * @return 空向量返回 true（true if empty）。
         */
        [[nodiscard]] bool empty() const noexcept;

        /**
         * @brief 获取底层只读存储（const data）；Access immutable storage.
         * @return 只读元素数组（immutable element array）。
         */
        [[nodiscard]] const std::vector<double> &data() const noexcept;

        /**
         * @brief 获取底层可写存储（mutable data）；Access mutable storage.
         * @return 可写元素数组（mutable element array）。
         */
        [[nodiscard]] std::vector<double> &data() noexcept;

        /**
         * @brief 边界检查访问（checked access）；Bounds-checked element access.
         * @param index 元素索引（element index）。
         * @return 对应元素引用（element reference）。
         * @note 索引越界抛出 std::out_of_range（throws std::out_of_range on invalid index）。
         */
        [[nodiscard]] double &at(std::size_t index);

        /**
         * @brief 边界检查访问（checked access）；Bounds-checked element access.
         * @param index 元素索引（element index）。
         * @return 对应元素常量引用（const element reference）。
         * @note 索引越界抛出 std::out_of_range（throws std::out_of_range on invalid index）。
         */
        [[nodiscard]] const double &at(std::size_t index) const;

        /**
         * @brief 无边界检查下标访问（unchecked access）；Unchecked index access.
         * @param index 元素索引（element index）。
         * @return 对应元素引用（element reference）。
         */
        [[nodiscard]] double &operator[](std::size_t index) noexcept;

        /**
         * @brief 无边界检查下标访问（unchecked access）；Unchecked index access.
         * @param index 元素索引（element index）。
         * @return 对应元素常量引用（const element reference）。
         */
        [[nodiscard]] const double &operator[](std::size_t index) const noexcept;

        /**
         * @brief 批量填充值（fill）；Assign one value to all elements.
         * @param value 填充值（fill value）。
         * @return 无返回值；No return value.
         */
        void fill(double value) noexcept;

        /**
         * @brief 向量点积（dot product）；Compute Euclidean dot product.
         * @param other 另一向量（other vector）。
         * @return 点积结果（dot product result）。
         * @note 尺寸不一致抛出 std::invalid_argument（throws std::invalid_argument on size mismatch）。
         */
        [[nodiscard]] double dot(const Vector &other) const;

        /**
         * @brief 输出指纹（output fingerprint）；Compute stable fingerprint of vector values.
         * @return 64 位指纹（64-bit fingerprint）。
         */
        [[nodiscard]] std::uint64_t fingerprint() const noexcept;

    private:
        /** @brief 连续元素存储（contiguous storage）；Owned vector values. */
        std::vector<double> values_{};
    };

} // namespace cpu_lab::domain::matrix_dot
