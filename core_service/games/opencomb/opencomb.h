#pragma once

const uint32_t k_total_block = 73;
const uint32_t k_direct_max = 3;
enum class Direct { TOP_LEFT = 0, VERT = 1, TOP_RIGHT = 2};
// 算分检索
const int32_t line[3][11][9] = {
    {
        {49, 58, 66, -1, -1, -1, -1, -1, -1},
        {32, 41, 50, 59, 67, -1, -1, -1, -1},
        {15, 24, 33, 42, 51, 60, 68, -1, -1},
        {7, 16, 25, 34, 43, 52, 61, 69, -1},
        {0, 8, 17, 26, 35, 44, 53, 62, 70},
        {1, 9, 18, 27, 36, 45, 54, 63, 71},
        {2, 10, 19, 28, 37, 46, 55, 64, 72},
        {3, 11, 20, 29, 38, 47, 56, 65, -1},
        {4, 12, 21, 30, 39, 48, 57, -1, -1},
        {5, 13, 22, 31, 40, -1, -1, -1, -1},
        {6, 14, 23, -1, -1, -1, -1, -1, -1}
    },
    {
        {0, 1, 2, 3, 4, 5, 6, -1, -1},
        {7, 8, 9, 10, 11, 12, 13, 14, -1},
        {15, 16, 17, 18, 19, 20, 21, 22, 23},
        {24, 25, 26, 27, 28, 29, 30, 31, -1},
        {32, 33, 34, 35, 36, 37, 38, 39, 40},
        {41, 42, 43, 44, 45, 46, 47, 48, -1},
        {49, 50, 51, 52, 53, 54, 55, 56, 57},
        {58, 59, 60, 61, 62, 63, 64, 65, -1},
        {66, 67, 68, 69, 70, 71, 72, -1, -1},
        {-1, -1, -1, -1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1, -1, -1, -1}
    },
    {
        {0, 7, 15, -1, -1, -1, -1, -1, -1},
        {1, 8, 16, 24, 32, -1, -1, -1, -1},
        {2, 9, 17, 25, 33, 41, 49, -1, -1},
        {3, 10, 18, 26, 34, 42, 50, 58, -1},
        {4, 11, 19, 27, 35, 43, 51, 59, 66},
        {5, 12, 20, 28, 36, 44, 52, 60, 67},
        {6, 13, 21, 29, 37, 45, 53, 61, 68},
        {14, 22, 30, 38, 46, 54, 62, 69, -1},
        {23, 31, 39, 47, 55, 63, 70, -1, -1},
        {40, 48, 56, 64, 71, -1, -1, -1, -1},
        {57, 65, 72, -1, -1, -1, -1, -1, -1}
    }
};
// 盘面（-1半砖，-2角落，-3无操作）
const int32_t board[18][9] = {
    {-2, -1, 15, -1, 32, -1, 49, -1, -2},
    {-3, 7, -3, 24, -3, 41, -3, 58, -3},
    {0, -3, 16, -3, 33, -3, 50, -3, 66},
    {-3, 8, -3, 25, -3, 42, -3, 59, -3},
    {1, -3, 17, -3, 34, -3, 51, -3, 67},
    {-3, 9, -3, 26, -3, 43, -3, 60, -3},
    {2, -3, 18, -3, 35, -3, 52, -3, 68},
    {-3, 10, -3, 27, -3, 44, -3, 61, -3},
    {3, -3, 19, -3, 36, -3, 53, -3, 69},
    {-3, 11, -3, 28, -3, 45, -3, 62, -3},
    {4, -3, 20, -3, 37, -3, 54, -3, 70},
    {-3, 12, -3, 29, -3, 46, -3, 63, -3},
    {5, -3, 21, -3, 38, -3, 55, -3, 71},
    {-3, 13, -3, 30, -3, 47, -3, 64, -3},
    {6, -3, 22, -3, 39, -3, 56, -3, 72},
    {-3, 14, -3, 31, -3, 48, -3, 65, -3},
    {-2, -3, 23, -3, 40, -3, 57, -3, -2},
    {-3, -1, -3, -1, -3, -1, -3, -1, -3}
};
// 游戏地图
const uint32_t k_map_size = 7;
const std::vector<std::pair<std::string, uint32_t>> map_string = {
    {"随机", 0}, {"经典", 1}, {"环巢", 2}, {"漩涡", 3}, {"飞机", 4}, {"面具", 5}, {"塔楼", 6}, {"三叶草", 7}
};
const int32_t maps[k_map_size][k_total_block] = {
    {   // 1#经典（格数：20）
        1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 0, 0, 0, 1, 1, 1,
        1, 1, 0, 0, 0, 0, 1, 1,
        1, 1, 0, 0, 0, 0, 0, 1, 1,
        1, 1, 0, 0, 0, 0, 1, 1,
        1, 1, 1, 0, 0, 0, 1, 1, 1,
        1, 0, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1,
    },
    {   // 2#环巢（格数：23，作者：铁蛋）
        1, 1, 1, 1, 1, 1, 1,
        1, 1, 0, 0, 0, 0, 1, 1,
        1, 1, 0, 0, 1, 0, 0, 1, 1,
        1, 0, 1, 1, 1, 1, 0, 1,
        1, 0, 2, 1, 0, 1, 2, 0, 1,
        1, 0, 1, 1, 1, 1, 0, 1,
        1, 1, 0, 0, 1, 0, 0, 1, 1,
        1, 1, 0, 0, 0, 0, 1, 1,
        1, 1, 1, 1, 1, 1, 1,
    },
    {   // 3#漩涡（格数：21，作者：纤光）
        1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 0, 0, 1, 1, 1,
        1, 1, 1, 0, 1, 1, 1, 1, 1,
        1, 1, 0, 0, 0, 0, 0, 1,
        1, 0, 1, 0, 0, 0, 1, 0, 1,
        1, 0, 0, 0, 0, 0, 1, 1,
        1, 1, 1, 1, 1, 0, 1, 1, 1,
        1, 1, 1, 0, 0, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1,
    },
    {   // 4#飞机（格数：21，作者：纤光）
        1, 1, 1, 0, 1, 1, 1,
        1, 1, 1, 0, 0, 1, 1, 1,
        1, 1, 1, 1, 0, 1, 1, 0, 1,
        1, 1, 1, 0, 0, 1, 0, 1,
        1, 1, 0, 0, 0, 0, 0, 1, 1,
        1, 1, 1, 0, 0, 1, 0, 1,
        1, 1, 1, 1, 0, 1, 1, 0, 1,
        1, 1, 1, 0, 0, 1, 1, 1,
        1, 1, 1, 0, 1, 1, 1,
    },
    {   // 5#面具（格数：23，作者：纤光）
        1, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 1, 1, 1, 1,
        1, 1, 0, 1, 0, 1, 1, 1, 1,
        1, 1, 0, 0, 1, 0, 0, 1,
        1, 1, 0, 0, 1, 0, 0, 0, 1,
        1, 1, 0, 0, 1, 0, 0, 1,
        1, 1, 0, 1, 0, 1, 1, 1, 1,
        1, 0, 0, 0, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1,
    },
    {   // 6#塔楼（格数：23，作者：纤光）
        1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 0, 1,
        1, 1, 1, 1, 0, 0, 0, 1, 1,
        1, 0, 0, 0, 1, 0, 0, 1,
        1, 0, 1, 0, 0, 0, 0, 1, 1,
        1, 0, 0, 0, 1, 0, 0, 1,
        1, 1, 1, 1, 0, 0, 0, 1, 1,
        1, 1, 1, 1, 1, 1, 0, 1,
        1, 1, 1, 1, 1, 1, 1,
    },
    {   // 7#三叶草（格数：22，作者：九九归一）
        1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 0, 0, 1, 1,
        1, 1, 1, 1, 0, 0, 0, 1, 1,
        1, 0, 0, 2, 0, 0, 1, 1,
        1, 0, 0, 0, 2, 2, 1, 0, 1,
        1, 0, 0, 2, 0, 0, 1, 1,
        1, 1, 1, 1, 0, 0, 0, 1, 1,
        1, 1, 1, 1, 0, 0, 1, 1,
        1, 1, 1, 1, 1, 1, 1,
    },
};

static std::string GetStyle(std::string resource_path)
{
    return R"(
<style>
    @font-face {
        font-family: \'CustomFont\';
        src: url(\"file:///)" + resource_path + R"(arial.ttf\");
    }
    .custom-font {
        font-family: 'CustomFont', sans-serif;
        position: absolute;
        z-index: 2;
        left: 50%;
        top: 50%;
        transform: translate(-50%, -50%);
        margin: 0;
    }
    .brick {
        position: relative;
        width: 64px;
        height: 64px;
        display: flex;
        justify-content: center;
        align-items: center;
    }
    .brick img {
        position: absolute;
        width: 100%;
        height: 100%;
        left: 0;
        top: 0;
        z-index: 1;
    }
</style>)";
}

class AreaCard
{
  public:
    AreaCard() = delete;    // wild card change to 10/10/10

    AreaCard(std::string type) : type_(type) {}

    AreaCard(const int32_t a, const int32_t b, const int32_t c) : points_(std::in_place, std::array<int32_t, k_direct_max>{a, b, c}) {}

    template <Direct direct>
    bool IsMatch(const int32_t point) const { return points_->at(static_cast<uint32_t>(direct)) == 10 || point == 10 || points_->at(static_cast<uint32_t>(direct)) == point; }

    template <Direct direct>
    std::optional<int32_t> Point() const
    {
        if (points_.has_value()) {
            return points_->at(static_cast<uint32_t>(direct));
        } else {
            return std::nullopt;
        }
    }

    std::string CardName() const
    {
        std::string str;
        if (points_.has_value()) {
            str = "card_";
            for (const int32_t point : *points_) {
                if (point == 10) {
                    str += "X";
                } else {
                    str += std::to_string(point);
                }
            }
            return str;
        } if (type_.has_value()) {
            if (type_ == "wall") str += "墙块";
            if (type_ == "wall_broken") str += "破碎墙块";
            if (type_ == "erase") str += "骷髅";
            if (type_ == "move") str += "移动";
            if (type_ == "reshape") str += "重塑";
            return str;
        }
        return "[Error]砖块信息为空";
    }

    std::string ToHtml(std::string image_path, const bool can_replace = false) const
    {
        std::string div = "<div class=\"brick\"><img src=\"file:///" + image_path + "card.png\">";
        div += "<img src=\"file:///" + image_path + ImageName<Direct::VERT>() + ".png\">";
        div += "<img src=\"file:///" + image_path + ImageName<Direct::TOP_RIGHT>() + ".png\">";
        div += "<img src=\"file:///" + image_path + ImageName<Direct::TOP_LEFT>() + ".png\">";
        if (can_replace) div += "<img src=\"file:///" + image_path + "card_replace.png\">";
        div += "</div>";
        return div;
    }

    template <Direct direct>
    std::string ImageName() const
    {
        if (points_.has_value()) {
            int32_t point = points_->at(static_cast<uint32_t>(direct));
            if (point == 10) {
                if (direct == Direct::VERT) return "Xv";
                if (direct == Direct::TOP_LEFT) return "Xl";
                if (direct == Direct::TOP_RIGHT) return "Xr";
            } else if (point == 0) {
                if (direct == Direct::VERT) return "0v";
                if (direct == Direct::TOP_LEFT) return "0l";
                if (direct == Direct::TOP_RIGHT) return "0r";
            } else {
                return std::to_string(point);
            }
        }
        return "";
    }

    std::string Type() const
    {
        if (type_.has_value()) {
            return *type_;
        } else {
            return "";
        }
    }

  private:
    std::optional<std::array<int32_t, k_direct_max>> points_;
    std::optional<std::string> type_;
};

class Area
{
  public:
    friend class OpenComb;

    Area(const uint32_t num) : num_(num), box_(nullptr), is_wall_(0), can_replace_(false), card_(std::nullopt) {}

    void SetBox(html::Box* box) { box_ = box; }

  private:
    html::Box* box_;
    std::optional<AreaCard> card_;
    uint32_t is_wall_;      // 0无墙 1墙块 2破碎墙块
    bool can_replace_;      // 是否可无限替换
    const uint32_t num_;
};

class OpenComb
{
  public:
    OpenComb(std::string image_path, const uint32_t map) : image_path_(std::move(image_path)), map_(map), table_(k_max_row, k_max_column)
    {
        analysisMap_();
        table_.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        for (int32_t row = 0; row < table_.Row(); ++row) {
            for (int32_t col = 0; col < table_.Column(); ++col) {
                const int32_t id = board[row][col];
                const bool is_full_box = id != -1 && id != -3;
                if (is_full_box) {
                    table_.MergeDown(row, col, 2);
                }
                html::Box& box = table_.Get(row, col);
                if (id == -1) {
                    box.SetContent(Image_("wall_half"));
                } else if (id == -2) {
                    box.SetContent(Image_("wall_full"));
                } else if (id != -3) {
                    box.SetContent(EmptyAreaHtml_(maps[map_][id], 0, areas_[id].num_));
                    areas_[id].is_wall_ = maps[map_][id];
                    areas_[id].SetBox(&box);
                }
            }
        }
        initial_table_ = ToHtml();
    }

    OpenComb(const OpenComb& comb) = delete;
    OpenComb(OpenComb&& comb) = delete;

    struct CombScore
    {
        int32_t score_;
        int32_t extra_score_;
        friend CombScore operator+(const CombScore& s1, const CombScore& s2)
        {
            return CombScore{.score_ = s1.score_ + s2.score_, .extra_score_ = s1.extra_score_ + s2.extra_score_};
        }
    };

    CombScore Reshape(const uint32_t num)
    {
        auto& area = areas_[numToid[num]];
        if (area.is_wall_) {
            area.is_wall_ = 3 - area.is_wall_;
            area.box_->SetContent(EmptyAreaHtml_(area.is_wall_, area.can_replace_, num));
        } else if (area.card_.has_value()) {
            const AreaCard wild = AreaCard(10, 10, 10);
            area.card_ = wild;
            const auto img_str = wild.ToHtml(image_path_, area.can_replace_);
            area.box_->SetContent(img_str);
        } else {
            area.can_replace_ = true;
            area.box_->SetContent(EmptyAreaHtml_(0, true, num));
        }
        return CaculateCombScore_();
    }

    CombScore Fill(const uint32_t num, const AreaCard& card)
    {
        if (card.Type() == "reshape") {
            return Reshape(num);
        }
        auto& area = areas_[numToid[num]];
        if (card.Type() == "erase") {
            area.is_wall_ = 0;
            area.card_ = std::nullopt;
            area.box_->SetContent(EmptyAreaHtml_(0, area.can_replace_, num));
        } else if (card.Type() == "wall") {
            area.is_wall_ = 1;
            area.card_ = std::nullopt;
            area.box_->SetContent(EmptyAreaHtml_(1, area.can_replace_, num));
        } else if (card.Type() == "wall_broken") {
            area.is_wall_ = 2;
            area.card_ = std::nullopt;
            area.box_->SetContent(EmptyAreaHtml_(2, area.can_replace_, num));
        } else {
            area.is_wall_ = 0;
            area.card_ = card;
            const auto img_str = card.ToHtml(image_path_, area.can_replace_);
            area.box_->SetContent(img_str);
        }
        return CaculateCombScore_();
    }

    CombScore Move(const uint32_t from, const uint32_t to)
    {
        auto& area_from = areas_[numToid[from]];
        auto& area_to = areas_[numToid[to]];
        if (area_from.is_wall_) {   // 墙块移动
            area_to.is_wall_ = area_from.is_wall_;
            area_from.is_wall_ = 0;
            area_to.card_ = std::nullopt;
            area_to.box_->SetContent(EmptyAreaHtml_(area_to.is_wall_ ,area_to.can_replace_, to));
            area_from.box_->SetContent(EmptyAreaHtml_(0, area_from.can_replace_, from));
        } else {   // 普通砖块移动
            area_to.card_ = area_from.card_;
            area_to.is_wall_ = 0;
            const auto img_str = area_to.card_->ToHtml(image_path_, area_to.can_replace_);
            area_to.box_->SetContent(img_str);
            area_from.card_ = std::nullopt;
            area_from.box_->SetContent(EmptyAreaHtml_(0, area_from.can_replace_, from));
        }
        return CaculateCombScore_();
    }

    std::pair<uint32_t, CombScore> SeqFill(const AreaCard& card)
    {
        for (uint32_t i = 1; i < areas_.size(); ++i) {
            auto& area = areas_[i];
            if (!area.is_wall_ && !area.card_.has_value()) {
                const auto img_str = card.ToHtml(image_path_, area.can_replace_);
                area.box_->SetContent(img_str);
                area.card_ = card;
                return {area.num_, CaculateCombScore_()};
            }
        }
        assert(false);
        return {UINT32_MAX, CombScore{0, 0}}; // unexpected case
    }

    bool IsAllFilled() const
    {
        for (uint32_t i = 0; i < areas_.size(); ++i) {
            if (!areas_[i].is_wall_ && !areas_[i].card_.has_value()) {
                return false;
            }
        }
        return true;
    }

    int32_t Score() const { return score_; }
    int32_t ExtraScore() const { return extra_score_; }

    int32_t BreakLine() const { return breakline_count_; }
    bool AirAllLine() const { return air_allline_; }
    bool WildAll10() const { return wild_all10_; }
    bool Length9() const { return length9_; }

    uint32_t IsWall(const uint32_t num) const { return areas_[numToid[num]].is_wall_; }

    bool IsFilled(const uint32_t num) const { return areas_[numToid[num]].card_.has_value(); }

    bool CanReplace(const uint32_t num) const { return (IsWall(num) != 1 && !IsFilled(num)) || areas_[numToid[num]].can_replace_; }
    
    std::string ToHtml() const { return table_.ToString(); }

    std::string GetInitTable_() const { return initial_table_; }

    std::string Image_(std::string name) const { return "![](file:///" + image_path_ + std::move(name) + ".png)"; }

  private:
    static constexpr uint32_t k_size = 3 + 1;
    static constexpr uint32_t k_max_row = k_size * 4 + 2;
    static constexpr uint32_t k_max_column = k_size * 2 + 1;

    CombScore CaculateCombScore_()
    {
        breakline_count_ = 0;
        air_allline_ = true;
        wild_all10_ = true;
        CombScore result = CaculateOneDirect_<Direct::VERT>() +
                        CaculateOneDirect_<Direct::TOP_LEFT>() +
                        CaculateOneDirect_<Direct::TOP_RIGHT>();
        int32_t point = result.score_ - score_;
        int32_t extra_point = result.extra_score_ - extra_score_;
        score_ = result.score_;
        extra_score_ = result.extra_score_;
        return CombScore{point, extra_point};   // return score change
    }

    template <Direct direct>
    CombScore CaculateOneDirect_()
    {
        int32_t score = 0;
        int32_t extra_score = 0;
        for (int32_t i = 0; i < 11; i++) {
            std::optional<int32_t> point;
            int32_t count = 0;
            bool connect = true;
            for (int32_t j = 0; j < 9; j++) {
                int32_t id = line[static_cast<uint32_t>(direct)][i][j];
                if (id == -1) {   // 到达地图边缘
                    LineConnect(connect, point, count, score, extra_score);
                    break;
                }
                if (areas_[id].card_.has_value()) {   // 砖块
                    if (!point.has_value()) {   // 第一块或墙块
                        point = areas_[id].card_->Point<direct>();
                        count++;
                    } else {
                        if (areas_[id].card_->IsMatch<direct>(*point)) {   // 匹配
                            count++;
                            if (*point == 10 && areas_[id].card_->Point<direct>() != 10) {   // 癞子线出现普通砖块
                                point = areas_[id].card_->Point<direct>();
                                wild_all10_ = false;
                            }
                            if (*point != 10 && areas_[id].card_->Point<direct>() == 10) wild_all10_ = false;   // 普通线出现癞子（成就失败）
                        } else {
                            connect = false;
                            if (*point == 0 || areas_[id].card_->Point<direct>() == 0) air_allline_ = false;    // 0线被其他数字阻断（成就失败）
                        }
                    }
                    if (*point == 0 && connect == false) air_allline_ = false;
                } else if (areas_[id].is_wall_) {   // 墙块（结算当前分数）
                    LineConnect(connect, point, count, score, extra_score);
                    connect = true;
                    point = std::nullopt;
                    count = 0;
                } else {   // 空区域
                    connect = false;
                    if (point.has_value()) {
                        if (*point == 0) air_allline_ = false;
                        if (*point == 10) wild_all10_ = false;
                    }
                }
                if (j == 8) {   // 地图最长边
                    LineConnect(connect, point, count, score, extra_score);
                }
            }
        }
        return CombScore{score, extra_score};
    }

    // 连线分数结算
    void LineConnect(const bool connect, const std::optional<int32_t> point, const int32_t count, int32_t& score, int32_t& extra_score) {
        if (connect) {
            if (!point.has_value()) {
                return;
            }
            score += *point * count;
            if (count >= 3) {
                const double multiple[10] = {0, 0, 0, 0.3, 0.4, 0.4, 0.5, 0.5, 0.6, 0.6};
                extra_score += *point * count * multiple[count];
            }
            if (count == 9) { length9_ = true; }
        } else {
            breakline_count_++;
        }
    }

    std::string EmptyAreaHtml_(const uint32_t is_wall, const bool can_replace, const uint32_t num) const
    {
        std::string div = "<div class=\"brick\"><img src=\"file:///" + image_path_ + (is_wall == 1 ? "wall" : (is_wall == 2 ? "wall_broken" : "card")) + ".png\">";
        div += "<p class=\"custom-font\"><font size=6>" + std::to_string(num) + "</font></p>";
        if (can_replace) div += "<img src=\"file:///" + image_path_ + "card_replace.png\">";
        div += "</div>";
        return div;
    }

    void analysisMap_()
    {
        int wall = std::count(std::begin(maps[map_]), std::end(maps[map_]), 0);
        int empty = 0;
        for (int i = 0; i < k_total_block; i++) {
            if (maps[map_][i] != 0) {
                numToid.push_back(i);
                areas_.push_back(++wall);
            } else {
                numToid.insert(numToid.begin() + empty, i);
                areas_.push_back(++empty);
            }
        }
        numToid.insert(numToid.begin(), -1);
    }

    const std::string image_path_;
    const uint32_t map_;
    html::Table table_;
    std::string initial_table_;
    std::vector<Area> areas_;
    std::vector<uint32_t> numToid;

    int32_t score_ = 0;
    int32_t extra_score_ = 0;

    int32_t breakline_count_ = 0;
    bool air_allline_ = true;
    bool wild_all10_ = true;
    bool length9_ = false;
};
