#pragma once

#include <string>
#include <vector>

#define HTML_COLOR_FONT_HEADER(color) "<font color=" #color ">"
#define HTML_FONT_TAIL "</font>"
#define HTML_ESCAPE_LT "&lt;"
#define HTML_ESCAPE_GT "&gt;"

struct Box
{
    std::string content_;
    std::string color_;
};

class Table
{
  public:
    Table(const uint32_t row, const uint32_t column);

    uint32_t Row() const;
    uint32_t Column() const;
    std::string ToString() const;

    void AppendRow();
    void AppendColumn();
    void InsertRow(const uint32_t idx);
    void InsertColumn(const uint32_t idx);
    void DeleteRow(const uint32_t idx);
    void DeleteColumn(const uint32_t idx);

    Box& Get(const uint32_t row, const uint32_t column) { return boxes_[row][column]; }
    const Box& Get(const uint32_t row, const uint32_t column) const { return boxes_[row][column]; }

  private:
    std::vector<std::vector<Box>> boxes_;
    uint32_t row_;
    uint32_t column_;
};

