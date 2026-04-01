#pragma once

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace cpu_lab::infrastructure::csv
{

    /**
     * @brief 行字段描述（Row field descriptor）；Compile-time field descriptor for Row reflection.
     * @tparam OwnerT 所属行类型（owner row type）。
     * @tparam ValueT 字段值类型（field value type）。
     */
    template <typename OwnerT, typename ValueT>
    struct RowField final
    {
        /** @brief 字段名（field name）；CSV column name. */
        std::string_view name{};

        /** @brief 成员指针（member pointer）；Pointer to member field. */
        ValueT OwnerT::*member{nullptr};
    };

    /**
     * @brief 创建字段描述（create field descriptor）；Factory for RowField.
     * @tparam OwnerT 所属行类型（owner row type）。
     * @tparam ValueT 字段值类型（field value type）。
     * @param name 字段名（field name）。
     * @param member 成员指针（member pointer）。
     * @return 字段描述对象（field descriptor）。
     */
    template <typename OwnerT, typename ValueT>
    [[nodiscard]] constexpr RowField<OwnerT, ValueT> make_row_field(
        const std::string_view name,
        ValueT OwnerT::*member) noexcept
    {
        return RowField<OwnerT, ValueT>{name, member};
    }

    namespace detail
    {

        /**
         * @brief 将头字段复制为字符串（copy header name）；Materialize header text to std::string.
         * @param header_name 头字段名（header column name）。
         * @return 拷贝后的字符串（copied string）。
         */
        [[nodiscard]] std::string copy_header_name(std::string_view header_name);

        /**
         * @brief 遍历 tuple 中每个元素（tuple iteration）；Apply a callable to each tuple element.
         * @tparam TupleT tuple 类型（tuple type）。
         * @tparam FnT 可调用对象类型（callable type）。
         * @tparam Indexes 索引序列（index sequence）。
         * @param tuple 目标 tuple（target tuple）。
         * @param fn 回调函数（callback）。
         * @return 无返回值；No return value.
         */
        template <typename TupleT, typename FnT, std::size_t... Indexes>
        constexpr void for_each_in_tuple_impl(
            TupleT &&tuple,
            FnT &&fn,
            std::index_sequence<Indexes...>)
        {
            (fn(std::get<Indexes>(std::forward<TupleT>(tuple))), ...);
        }

        /**
         * @brief 遍历 tuple 中每个元素（tuple iteration）；Convenience wrapper for tuple iteration.
         * @tparam TupleT tuple 类型（tuple type）。
         * @tparam FnT 可调用对象类型（callable type）。
         * @param tuple 目标 tuple（target tuple）。
         * @param fn 回调函数（callback）。
         * @return 无返回值；No return value.
         */
        template <typename TupleT, typename FnT>
        constexpr void for_each_in_tuple(TupleT &&tuple, FnT &&fn)
        {
            constexpr std::size_t tuple_size = std::tuple_size_v<std::remove_reference_t<TupleT>>;
            for_each_in_tuple_impl(
                std::forward<TupleT>(tuple),
                std::forward<FnT>(fn),
                std::make_index_sequence<tuple_size>{});
        }

    } // namespace detail

    /**
     * @brief 行类型概念（row-like concept）；Compile-time contract for reflected CSV row types.
     * @tparam RowT 行类型（row type）。
     */
    template <typename RowT>
    concept RowLike = requires
    {
        { RowT::meta() };
    };

    /**
     * @brief 字段数量（field count）；Number of reflected fields.
     * @tparam RowT 行类型（row type）。
     * @return 字段数量（field count）。
     */
    template <RowLike RowT>
    [[nodiscard]] constexpr std::size_t row_field_count() noexcept
    {
        using MetaType = decltype(RowT::meta());
        return std::tuple_size_v<MetaType>;
    }

    /**
     * @brief 遍历元信息字段（iterate metadata）；Visit each field descriptor in meta().
     * @tparam RowT 行类型（row type）。
     * @tparam FnT 可调用对象类型（callable type）。
     * @param fn 回调函数（callback）。
     * @return 无返回值；No return value.
     */
    template <RowLike RowT, typename FnT>
    constexpr void row_for_each_meta(FnT &&fn)
    {
        const auto meta = RowT::meta();
        detail::for_each_in_tuple(meta, std::forward<FnT>(fn));
    }

    /**
     * @brief 生成 CSV 表头（CSV header）；Build CSV header names from metadata.
     * @tparam RowT 行类型（row type）。
     * @return 表头列名数组（header columns）。
     */
    template <RowLike RowT>
    [[nodiscard]] std::vector<std::string> row_header()
    {
        std::vector<std::string> columns{};
        columns.reserve(row_field_count<RowT>());

        row_for_each_meta<RowT>(
            [&columns](const auto &field)
            {
                columns.push_back(detail::copy_header_name(field.name));
            });

        return columns;
    }

    /**
     * @brief 遍历当前对象字段（mutable visit）；Visit each reflected field with mutable access.
     * @tparam RowT 行类型（row type）。
     * @tparam FnT 可调用对象类型（callable type）。
     * @param row 目标对象（target row object）。
     * @param fn 回调函数，签名建议为 fn(std::string_view, ValueT&)。
     *          Callback, suggested signature fn(std::string_view, ValueT&).
     * @return 无返回值；No return value.
     */
    template <RowLike RowT, typename FnT>
    constexpr void for_each_field(RowT &row, FnT &&fn)
    {
        row_for_each_meta<RowT>(
            [&row, &fn](const auto &field)
            {
                fn(field.name, row.*(field.member));
            });
    }

    /**
     * @brief 遍历当前对象字段（const visit）；Visit each reflected field with const access.
     * @tparam RowT 行类型（row type）。
     * @tparam FnT 可调用对象类型（callable type）。
     * @param row 目标对象（target row object）。
     * @param fn 回调函数，签名建议为 fn(std::string_view, const ValueT&)。
     *          Callback, suggested signature fn(std::string_view, const ValueT&).
     * @return 无返回值；No return value.
     */
    template <RowLike RowT, typename FnT>
    constexpr void for_each_field(const RowT &row, FnT &&fn)
    {
        row_for_each_meta<RowT>(
            [&row, &fn](const auto &field)
            {
                fn(field.name, row.*(field.member));
            });
    }

} // namespace cpu_lab::infrastructure::csv