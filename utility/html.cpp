#include "html.h"

Table::Table(const uint32_t row, const uint32_t column) : boxes_(row, std::vector<Box>(column)), row_(row), column_(column)
{
}

std::string Table::ToString() const
{
    std::string outstr = "<table align=\"center\"><tbody>";
    for (const auto& row : boxes_) {
        outstr += "\n<tr>";
        for (const auto& box : row) {
            outstr += "\n<td align=\"center\"";
            if (!box.color_.empty()) {
                outstr += " bgcolor=\"" + box.color_ + "\"";
            }
            outstr += ">";
            outstr += box.content_;
            outstr += "</td>";
        }
        outstr += "\n</tr>";
    }
    return outstr;
}
