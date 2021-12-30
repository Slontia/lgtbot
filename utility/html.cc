#include "html.h"

namespace html {

Table::Table(const uint32_t row, const uint32_t column)
    : boxes_(row, std::vector<Box>(column))
    , row_(row)
    , column_(column)
    , table_style_(" align=\"center\" border=\"1px solid #ccc\" cellspacing=\"0\" cellpadding=\"0\" ")
    , row_style_(" align=\"center\" ")
{
}

std::string Table::ToString() const
{
    std::string outstr = "<table " + table_style_ + " ><tbody>";
    for (const auto& row : boxes_) {
        outstr += "\n<tr>";
        for (const auto& box : row) {
            if (box.merge_num_ == 0) {
                continue;
            }
            outstr += "\n<td " + row_style_;
            if (!box.color_.empty()) {
                outstr += " bgcolor=\"" + box.color_ + "\"";
            }
            if (box.merge_num_ > 1) {
                outstr += (box.merge_direct_ == Box::MergeDirect::TO_BOTTOM ? " rowspan=\"" : " colspan=\"");
                outstr += std::to_string(box.merge_num_) + "\"";
            }
            outstr += ">\n\n";
            outstr += box.content_;
            outstr += "\n\n</td>";
        }
        outstr += "\n</tr>";
    }
    outstr += "\n</tbody></table>";
    return outstr;
}

void Table::MergeDown(const uint32_t row, const uint32_t column, const uint32_t num)
{
    if (num == 1) {
        return;
    }
    assert(num > 1);
    Get(row, column).merge_num_ = num;
    Get(row, column).merge_direct_ = Box::MergeDirect::TO_BOTTOM;
    for (uint32_t i = 1; i < num; ++i) {
        assert(Get(row + i, column).merge_num_ == 1);
        Get(row + i, column).merge_num_ = 0;
    }
}

void Table::MergeRight(const uint32_t row, const uint32_t column, const uint32_t num)
{
    if (num == 1) {
        return;
    }
    assert(num > 1);
    Get(row, column).merge_num_ = num;
    Get(row, column).merge_direct_ = Box::MergeDirect::TO_RIGHT;
    for (uint32_t i = 1; i < num; ++i) {
        assert(Get(row, column + i).merge_num_ == 1);
        Get(row, column + i).merge_num_ = 0;
    }
}

}
