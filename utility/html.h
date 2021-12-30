// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <cassert>

#define HTML_COLOR_FONT_HEADER(color) "<font color=" #color ">"
#define HTML_FONT_TAIL "</font>"
#define HTML_ESCAPE_LT "&lt;"
#define HTML_ESCAPE_GT "&gt;"
#define HTML_ESCAPE_SPACE "&nbsp;"

namespace html {

class Box
{
  public:
    friend class Table;

    Box() : merge_direct_(MergeDirect::TO_BOTTOM), merge_num_(1) {}
    enum class MergeDirect { TO_BOTTOM, TO_RIGHT };

    template <typename String>
    void SetContent(String&& str)
    {
        assert(merge_num_ > 0);
        content_ = std::forward<String>(str);
    }

    std::string& Content()
    {
        assert(merge_num_ > 0);
        return content_;
    }

    template <typename String>
    void SetColor(String&& str)
    {
        assert(merge_num_ > 0);
        color_ = std::forward<String>(str);
    }

    bool IsVisable() const { return merge_num_ >= 1; }

  private:
    uint32_t merge_num_;
    MergeDirect merge_direct_;
    std::string content_;
    std::string color_;
};

class Table
{
  public:
    Table(const uint32_t row, const uint32_t column);

    uint32_t Row() const { return row_; }
    uint32_t Column() const { return column_; }
    std::string ToString() const;

    void AppendRow()
    {
        boxes_.emplace_back(column_);
        ++row_;
    }
    void AppendColumn()
    {
        for (auto& row : boxes_) {
            row.emplace_back();
        }
        ++column_;
    }
    void InsertRow(const uint32_t idx);
    void InsertColumn(const uint32_t idx);
    void DeleteRow(const uint32_t idx);
    void DeleteColumn(const uint32_t idx);

    void MergeDown(const uint32_t row, const uint32_t column, const uint32_t num);
    void MergeRight(const uint32_t row, const uint32_t column, const uint32_t num);
    void MergeRow(const uint32_t row) { MergeRight(row, 0, column_); }

    Box& Get(const uint32_t row, const uint32_t column) { return boxes_[row][column]; }
    const Box& Get(const uint32_t row, const uint32_t column) const { return boxes_[row][column]; }

    Box& GetLastRow(const uint32_t column) { return boxes_[row_ - 1][column]; }
    const Box& GetLastRow(const uint32_t column) const { return boxes_[row_ - 1][column]; }

    Box& GetLastColumn(const uint32_t row) { return boxes_[row][column_ - 1]; }
    const Box& GetLastColumn(const uint32_t row) const { return boxes_[row][column_ - 1]; }

    void SetTableStyle(std::string style) { table_style_ = std::move(style); }
    void SetRowStyle(std::string style) { row_style_ = std::move(style); }

  private:
    std::vector<std::vector<Box>> boxes_;
    uint32_t row_;
    uint32_t column_;
    std::string table_style_;
    std::string row_style_;
};

}
