#include <variant>

template <uint32_t k_max_m = 26, uint32_t k_max_n = 99>
static std::variant<std::string, std::pair<uint32_t, uint32_t>> DecodePos(const std::string& str)
{
    static_assert(0 < k_max_m && k_max_m <= 26);
    static_assert(0 < k_max_n && k_max_n <= 99);
    if (str.size() != 2 && str.size() != 3) {
        return "非法的坐标长度 " + std::to_string(str.size()) + " ，应为 2 或 3";
    }
    std::pair<uint32_t, uint32_t> coor;
    const char row_c = str[0];
    if ('A' <= row_c && row_c < 'A' + k_max_m) {
        coor.first = row_c - 'A';
    } else if ('a' <= row_c && row_c < 'a' + k_max_m) {
        coor.first = row_c - 'a';
    } else {
        return "非法的横坐标「" + std::to_string(row_c) + "」，应在 A 和 "  + std::string(1, static_cast<char>('A' + k_max_m - 1)) + " 之间";
    }
    if (std::isdigit(str[1])) {
        coor.second = str[1] - '0';
        if (str.size() == 2 || (std::isdigit(str[2]) && (coor.second = 10 * coor.second + str[2] - '0') < k_max_n)) {
            return coor;
        }
    }
    return "非法的纵坐标「" + str.substr(1) + "」，应在 0 和 " + std::to_string(k_max_n - 1) + " 之间";
}
