#include "infrastructure/csv/row.hpp"

#include <string>

namespace cpu_lab::infrastructure::csv::detail
{

    /**
     * @brief 复制头字段名（copy header name）；Materialize header name as owning std::string.
     * @param header_name 头字段名（header column name）。
     * @return 拷贝字符串（copied string）。
     */
    std::string copy_header_name(const std::string_view header_name)
    {
        return std::string{header_name};
    }

} // namespace cpu_lab::infrastructure::csv::detail