#include <iostream>
#include <charconv>

int main()
{
    char result{};
    std::string str = "9";
    const auto [ptr, ec] { std::from_chars(str.data(), str.data() + str.size(), result) };
    if (ec == std::errc() && ptr == str.data() + str.size()) {
        std::cout << result << std::endl;
    } else {
        std::cout << "failed" << std::endl;
    }
    return 0;
}
