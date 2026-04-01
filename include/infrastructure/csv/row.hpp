#pragma once

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
     * @brief 行反射基类（Row reflection base）；CRTP base for tuple-like row metadata reflection.
     * @tparam DerivedT 派生行类型（derived row type），需实现 static constexpr meta().
     */
    template <typename DerivedT>
    class Row
    {
    public:
        /** @brief 派生类型别名（derived alias）；Alias for derived row type. */
        using DerivedType = DerivedT;

        /**
         * @brief 字段数量（field count）；Number of reflected fields.
         * @return 字段数量（field count）。
         */
        [[nodiscard]] static constexpr std::size_t field_count() noexcept
        {
            using MetaType = decltype(DerivedType::meta());
            return std::tuple_size_v<MetaType>;
        }

        /**
         * @brief 生成 CSV 表头（CSV header）；Build CSV header names from metadata.
         * @return 表头列名数组（header columns）。
         */
        [[nodiscard]] static std::vector<std::string> header()
        {
            std::vector<std::string> columns{};
            columns.reserve(field_count());

            for_each_meta(
                [&columns](const auto &field)
                {
                    columns.push_back(detail::copy_header_name(field.name));
                });

            return columns;
        }

        /**
         * @brief 遍历元信息字段（iterate metadata）；Visit each field descriptor in meta().
         * @tparam FnT 可调用对象类型（callable type）。
         * @param fn 回调函数（callback）。
         * @return 无返回值；No return value.
         */
        template <typename FnT>
        static constexpr void for_each_meta(FnT &&fn)
        {
            const auto meta = DerivedType::meta();
            detail::for_each_in_tuple(meta, std::forward<FnT>(fn));
        }

        /**
         * @brief 遍历当前对象字段（mutable visit）；Visit each reflected field with mutable access.
         * @tparam FnT 可调用对象类型（callable type）。
         * @param fn 回调函数，签名建议为 fn(std::string_view, ValueT&)。
         *          Callback, suggested signature fn(std::string_view, ValueT&).
         * @return 无返回值；No return value.
         */
        template <typename FnT>
        constexpr void for_each_field(FnT &&fn)
        {
            DerivedType &self = static_cast<DerivedType &>(*this);
            for_each_meta(
                [&self, &fn](const auto &field)
                {
                    fn(field.name, self.*(field.member));
                });
        }

        /**
         * @brief 遍历当前对象字段（const visit）；Visit each reflected field with const access.
         * @tparam FnT 可调用对象类型（callable type）。
         * @param fn 回调函数，签名建议为 fn(std::string_view, const ValueT&)。
         *          Callback, suggested signature fn(std::string_view, const ValueT&).
         * @return 无返回值；No return value.
         */
        template <typename FnT>
        constexpr void for_each_field(FnT &&fn) const
        {
            const DerivedType &self = static_cast<const DerivedType &>(*this);
            for_each_meta(
                [&self, &fn](const auto &field)
                {
                    fn(field.name, self.*(field.member));
                });
        }
    };

} // namespace cpu_lab::infrastructure::csv