// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifndef TREASURE_HUNTER_BOARD_H
#define TREASURE_HUNTER_BOARD_H

#include <algorithm>
#include <cstddef>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "utility/html.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

using namespace std;

// ==================== 卡牌 ====================

enum class CardType { WEALTH, TREASURE, EVENT, MONSTER };

// 怪物编号
enum MonsterType {
    PHOENIX = 1,
    CERBERUS = 2,
    MEDUSA = 3,
};
const int k_monster_type_count = 3;

// 奇遇编号
enum EventType {
    UPCOMING_WEALTH = 1,
    UPCOMING_TREASURE = 2,
    UPCOMING_MONSTER = 3,
    DOUBLE_WEALTH = 4,
    EXTRA_WEALTH = 5,
    DOUBLE_TREASURE = 6,
    MAX_TREASURE = 7,
    SUDDEN_DEATH = 8,
    QUIT_REWARD = 9,
    WEALTH_INCREASE = 10,
    WEALTH_DISTRIBUTE = 11,
    WEALTH_RESET = 12,
    TREASURE_CONVERT = 13,
};
const int k_event_type_count = 13;

// 每局牌堆构成
const int k_shuffled_wealth_num = 15;   // 参与洗牌的宝藏
const int k_leading_wealth_num = 3;     // 开局固定翻开的宝藏
const int k_treasure_num = 5;
const int k_monster_num = 6;
const int k_event_num = 8;              // 从全部奇遇中抽取的数量

// 赛况图的排版尺寸
const int k_table_width = 800;         // 4 人及以下时的赛况图宽度
const int k_base_player_num = 4;       // 超过该人数后每多一人加宽一档
const int k_width_per_player = 100;    // 每多一名玩家增加的宽度，用于容纳更长的昵称
const int k_card_width = 121;          // 单张卡牌宽度
const int k_card_height = 185;         // 单张卡牌高度
const int k_cards_per_row = 7;         // 每行摆放的卡牌数
const int k_card_pitch = 100;          // 相邻卡牌的横向间距，小于卡宽形成叠放效果
const int k_card_left = 52;            // 每行第一张卡牌的左边距
const int k_card_image_width = 121;    // 单张卡牌单独成图时的宽度
const int k_avatar_size = 60;          // 状态栏头像边长
const int k_avatar_space = 76;         // 头像连同外边距占用的横向空间

const int k_special_cerberus_chance = 10;  // 每局启用地狱犬彩蛋卡面的概率（百分比）
const uint32_t k_hook_timeout = 15;        // 仍在探险的玩家全部挂机时，本回合的等待秒数
const uint32_t k_min_round_seconds = 20;   // 存在挂机玩家时，一回合最短的等待秒数
const uint32_t k_grace_notice_seconds = 5; // 剩余等待时间少于该值时不再公屏提示，玩家已来不及反应

// 框架返回的玩家名形如「<昵称(账号)>」，取出其中的昵称部分
inline string ExtractNickname(const string& player_name)
{
    if (player_name.empty()) {
        return player_name;
    }
    size_t begin = player_name.front() == '<' ? 1 : 0;
    size_t end = player_name.size();
    if (player_name.back() == '>') {
        const size_t bracket = player_name.rfind('(');
        end = bracket != string::npos && bracket > begin ? bracket : end - 1;
    }
    return end > begin ? player_name.substr(begin, end - begin) : player_name;
}

inline string MonsterName(const int monster)
{
    switch (monster) {
        case PHOENIX:  return "不死鸟";
        case CERBERUS: return "地狱犬";
        case MEDUSA:   return "美杜莎";
        default:       return "";
    }
}

inline string MonsterImageName(const int monster)
{
    switch (monster) {
        case PHOENIX:  return "phoenix";
        case CERBERUS: return "cerberus";
        case MEDUSA:   return "medusa";
        default:       return "";
    }
}

inline string MonsterColor(const int monster)
{
    switch (monster) {
        case PHOENIX:  return "#FF931E";
        case CERBERUS: return "#AD0101";
        case MEDUSA:   return "#007E00";
        default:       return "#000000";
    }
}

// 宝藏卡按剩余金币数选用不同大小的金币堆图案，剩余为 0 时不绘制图案
inline int WealthIconLevel(const int value)
{
    if (value >= 5) { return 4; }
    if (value >= 3) { return 3; }
    if (value >= 2) { return 2; }
    if (value >= 1) { return 1; }
    return 0;
}

inline string EventDescription(const int event)
{
    switch (event) {
        case UPCOMING_WEALTH:    return "接下来三回合必定找到数额较大的宝藏";
        case UPCOMING_TREASURE:  return "接下来三回合必定找到珍宝";
        case UPCOMING_MONSTER:   return "接下来三回合必定遭遇怪物";
        case DOUBLE_WEALTH:      return "下一次发现的宝藏数量翻倍";
        case EXTRA_WEALTH:       return "下一次发现的宝藏数量增加";
        case DOUBLE_TREASURE:    return "下一次发现的珍宝价值翻倍";
        case MAX_TREASURE:       return "下一次发现的珍宝价值必定是20";
        case SUDDEN_DEATH:       return "接下来一旦遭遇怪物就会立刻结束游戏";
        case QUIT_REWARD:        return "本回合选择返回营地的玩家，平分一定数量的金币";
        case WEALTH_INCREASE:    return "过去发现的所有宝藏，额外增加1~2个金币";
        case WEALTH_DISTRIBUTE:  return "过去发现的所有宝藏，平分一定数量的金币";
        case WEALTH_RESET:       return "过去发现的宝藏，随机一个恢复到初始数值";
        case TREASURE_CONVERT:   return "过去发现的宝藏，随机一个变成珍宝";
        default:                 return "";
    }
}

// 奇遇卡牌面上下两行的文字
inline pair<string, string> EventCaption(const int event)
{
    switch (event) {
        case UPCOMING_WEALTH:    return {"前方", "宝藏"};
        case UPCOMING_TREASURE:  return {"前方", "珍宝"};
        case UPCOMING_MONSTER:   return {"怪物", "出没"};
        case DOUBLE_WEALTH:      return {"双倍", "宝藏"};
        case EXTRA_WEALTH:       return {"更多", "宝藏"};
        case DOUBLE_TREASURE:    return {"双倍", "珍宝"};
        case MAX_TREASURE:       return {"稀世", "珍宝"};
        case SUDDEN_DEATH:       return {"怪物", "突袭"};
        case QUIT_REWARD:        return {"见好", "就收"};
        case WEALTH_INCREASE:    return {"宝藏", "升值"};
        case WEALTH_DISTRIBUTE:  return {"天降", "横财"};
        case WEALTH_RESET:       return {"宝藏", "再生"};
        case TREASURE_CONVERT:   return {"点石", "成金"};
        default:                 return {"", ""};
    }
}

class Card
{
  public:
    Card(const CardType type, const string& name, const string& description, const int value)
        : type(type), name(name), description(description), value(value), real_value(value) {}

    CardType type;
    string name;
    string description;
    int value;        // 卡牌的初始数值
    int real_value;   // 卡牌当前剩余的数值

    bool IsWealth() const { return type == CardType::WEALTH; }
    bool IsTreasure() const { return type == CardType::TREASURE; }
    bool IsEvent() const { return type == CardType::EVENT; }
    bool IsMonster() const { return type == CardType::MONSTER; }
};

inline Card MakeWealthCard(const int value)
{
    return Card(CardType::WEALTH, "宝藏", "总计" + to_string(value) + "枚金币", value);
}

inline Card MakeTreasureCard(const int value)
{
    return Card(CardType::TREASURE, "珍宝", "价值" + to_string(value) + "金币", value);
}

inline Card MakeMonsterCard(const int monster)
{
    return Card(CardType::MONSTER, "怪物", "遭遇了" + MonsterName(monster), monster);
}

inline Card MakeEventCard(const int event)
{
    return Card(CardType::EVENT, "奇遇", EventDescription(event), event);
}

// ==================== 玩家 ====================

enum class PlayerState {
    EXPLORING,  // 仍在秘境中探险
    RETREATED,  // 已带着金币返回营地
    DEVOURED,   // 被怪物吞噬，失去全部金币
};

enum class Choice {
    NONE,       // 本回合尚未做出选择
    CONTINUE,   // 继续探险
    RETREAT,    // 返回营地
};

// 成就判定所需的统计
struct Record
{
    int retreat_round = 0;             // 撤离时所处的选择回合，0 表示从未撤离
    int retreat_cards = 0;             // 撤离时已经翻开的卡牌数
    int treasure_taken = 0;            // 撤离时带走的珍宝件数
    int best_treasure = 0;             // 撤离时带走的最贵珍宝价值
    int event_seen = 0;                // 仍在探险时见证的奇遇张数
    int gold_after_sudden_death = 0;   // 「怪物突袭」生效后在路上分得的金币，不含撤离时的结算
    bool retreat_alone = false;        // 是否独自撤离
    bool escaped_before_wipe = false;  // 撤离后翻开的下一张牌吞噬了所有留守玩家
    bool saw_sudden_death = false;     // 是否在场见证「怪物突袭」生效
    bool met_monster[k_monster_type_count + 1] = {};  // 仍在探险时遇见过的怪物种类

    int MetMonsterTypes() const
    {
        return static_cast<int>(count(met_monster + 1, met_monster + k_monster_type_count + 1, true));
    }
};

class Player
{
  public:
    Player(const string& name, const string& avatar) : name(name), avatar(avatar) {}

    const string name;
    const string avatar;

    int gold = 0;
    PlayerState state = PlayerState::EXPLORING;
    Choice choice = Choice::NONE;
    // 挂机状态：每回合自动继续探险，其他玩家行动完毕后不再等待
    bool hook_status = false;
    Record record;

    bool IsExploring() const { return state == PlayerState::EXPLORING; }
};

// ==================== 牌堆 ====================

class Board
{
  public:
    Board(const string& resource_dir)
        : resource_path_(resource_dir.empty() || resource_dir.back() == '/' || resource_dir.back() == '\\'
                ? resource_dir : resource_dir + "/")
    {}

    const string resource_path_;

    vector<Player> players;

    vector<Card> pool;    // 尚未翻开的牌堆，按顺序抽取
    vector<Card> past;    // 已经翻开的卡牌
    vector<Card> events;  // 正在等待生效的奇遇卡

    // 本局的地狱犬是否使用彩蛋卡面，开局决定一次，整局保持一致
    bool special_cerberus = false;

    // 按固定构成生成本局牌堆，并把三张开局宝藏放到最前面
    void Initialize()
    {
        rng_.seed(random_device{}());
        special_cerberus = Rand(1, 100) <= k_special_cerberus_chance;
        pool.clear();
        past.clear();
        events.clear();

        for (int i = 0; i < k_shuffled_wealth_num; ++i) {
            pool.push_back(MakeWealthCard(Rand(3, 15)));
        }
        for (int i = 0; i < k_treasure_num; ++i) {
            pool.push_back(MakeTreasureCard(Rand(10, 20)));
        }
        for (int i = 0; i < k_monster_num; ++i) {
            pool.push_back(MakeMonsterCard(i % k_monster_type_count + 1));
        }
        shuffle(pool.begin(), pool.end(), rng_);

        InsertEventCards_();

        for (int i = 0; i < k_leading_wealth_num; ++i) {
            pool.insert(pool.begin(), MakeWealthCard(Rand(3, 15)));
        }
    }

    int Rand(const int lo, const int hi) { return uniform_int_distribution<int>(lo, hi)(rng_); }

    // 人数越多昵称区越窄，因此按人数适当加宽整张赛况图
    int TableWidth() const
    {
        return k_table_width +
            max(0, static_cast<int>(players.size()) - k_base_player_num) * k_width_per_player;
    }

    int ExploringCount() const
    {
        return static_cast<int>(count_if(players.begin(), players.end(),
                    [](const Player& p) { return p.IsExploring(); }));
    }

    // ==================== 绘制 ====================

    // 单张卡牌单独成图，用于公布本回合翻开的卡牌
    string GetCardMarkdown(const Card& card) const
    {
        return Style_() + CardHtml_(card);
    }

    // 玩家状态栏 + 已翻开的全部卡牌
    string GetTableMarkdown() const
    {
        return Style_() + HeadHtml_() + CardsHtml_();
    }

  private:
    // 字体style
    string FontFace_() const
    {
        const string url = "file:///" + resource_path_;
        return
            "@font-face { font-family: '" + string(k_number_font_) + "'; src: url(\"" + url + "ERASDEMI.ttf\"); }\n"
            "@font-face { font-family: '" + string(k_monster_font_) + "'; src: url(\"" + url + "simkai.ttf\"); }\n"
            "@font-face { font-family: '" + string(k_event_font_) + "'; src: url(\"" + url + "STXinwei.ttf\"); }\n"
            "@font-face { font-family: '" + string(k_tag_font_) + "'; src: url(\"" + url + "Dengb.ttf\"); }\n"
            "@font-face { font-family: '" + string(k_text_font_) + "'; src: url(\"" + url + "msyh.ttf\"); }\n";
    }

    string Style_() const
    {
        // 昵称长度不可控，状态栏按人数分配固定宽度，超出部分省略，避免撑破表格
        const int cell_width = players.empty() ? k_table_width : TableWidth() / static_cast<int>(players.size());
        const int info_width = max(60, cell_width - k_avatar_space);
        return
            "<style>\n" + FontFace_() +
            "body { margin: 0; font-family: '" + k_text_font_ + "', sans-serif; }\n"
            "p { margin: 0; }\n"
            ".th-player { display: flex; align-items: center; justify-content: flex-start; }\n"
            ".th-avatar { width: " + to_string(k_avatar_size) + "px; height: " + to_string(k_avatar_size) +
                "px; margin: 6px 8px 6px 4px; flex: none; }\n"
            ".th-avatar img { width: " + to_string(k_avatar_size) + "px; height: " + to_string(k_avatar_size) +
                "px; }\n"
            ".th-info { width: " + to_string(info_width) + "px; height: " + to_string(k_avatar_size) +
                "px; text-align: left; overflow: hidden; }\n"
            ".th-name, .th-state, .th-gold { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }\n"
            // 三行合计正好与头像等高
            ".th-name { font-size: 13px; line-height: 17px; color: #666666; }\n"
            ".th-state { line-height: 22px; }\n"
            ".th-gold { font-size: 16px; line-height: 21px; font-weight: bold; color: #BD8900; }\n"
            // 状态做成胶囊标签，用同色系浅底色让三种状态一眼可辨
            ".th-tag { display: inline-block; padding: 2px 10px; border-radius: 10px;"
                " font-size: 14px; line-height: 17px; font-weight: bold; }\n"
            ".th-tag-exploring { color: #147130; background: #CDEFD8; }\n"
            ".th-tag-retreated { color: #0A57AE; background: #D2E6FB; }\n"
            ".th-tag-devoured { color: #A31414; background: #F8D2D2; }\n"
            ".th-row { position: relative; width: " + to_string(TableWidth()) + "px; height: " +
                to_string(k_card_height + 2) + "px; overflow: hidden; }\n"
            ".th-slot { position: absolute; top: 1px; }\n"
            // 绝对定位的子元素会撑大文档滚动区域，进而让渲染出的图片多出边缘，因此裁掉卡面之外的部分
            ".card { position: relative; overflow: hidden; width: " + to_string(k_card_width) + "px; height: " +
                to_string(k_card_height) + "px; }\n"
            ".card .bg { position: absolute; left: 0; top: 0; width: " + to_string(k_card_width) + "px; height: " +
                to_string(k_card_height) + "px; }\n"
            ".card .icon { position: absolute; left: 12px; width: 96px; }\n"
            ".card .txt { position: absolute; left: 0; width: " + to_string(k_card_width) +
                "px; display: flex; align-items: center; justify-content: center; }\n"
            ".card .num { top: 122px; height: 60px; font-size: 48px; font-weight: bold; color: #FFFFFF;"
                " font-family: '" + k_number_font_ + "', Arial, sans-serif; }\n"
            ".card .tag { top: 0; height: 30px; font-size: 20px; font-weight: bold; color: #0000FF;"
                " font-family: '" + k_tag_font_ + "', sans-serif; }\n"
            ".card .monster { top: 114px; height: 56px; font-size: 36px; font-weight: bold;"
                " font-family: '" + k_monster_font_ + "', serif;"
                " text-shadow: 1px 1px 0 #000000, -1px 1px 0 #000000, 1px -1px 0 #000000, -1px -1px 0 #000000; }\n"
            ".card .ev { height: 56px; font-size: 52px; color: #FFFF00;"
                " font-family: '" + k_event_font_ + "', serif; text-shadow: 1px 1px 0 #404040; }\n"
            ".card .ev1 { top: 34px; }\n"
            ".card .ev2 { top: 84px; }\n"
            "</style>\n";
    }

    string Img_(const string& file, const string& classes, const string& style) const
    {
        return "<img class=\"" + classes + "\" style=\"" + style + "\" src=\"file:///" + resource_path_ + file + "\">";
    }

    string CardHtml_(const Card& card) const
    {
        string html = "<div class=\"card\">";
        switch (card.type) {
            case CardType::WEALTH: {
                html += Img_("bg_wealth.png", "bg", "");
                const int level = WealthIconLevel(card.real_value);
                if (level > 0) {
                    html += Img_("wealth_" + to_string(level) + ".png", "icon", "top: 4px;");
                    html += "<div class=\"txt num\">" + to_string(card.real_value) + "</div>";
                }
                break;
            }
            case CardType::TREASURE: {
                html += Img_("bg_treasure.png", "bg", "");
                if (card.real_value > 0) {
                    html += Img_("treasure.png", "icon", "top: 30px;");
                    html += "<div class=\"txt tag\">仅1人独享</div>";
                    html += "<div class=\"txt num\">" + to_string(card.real_value) + "</div>";
                }
                break;
            }
            case CardType::MONSTER: {
                const string image = MonsterImageName(card.value);
                html += Img_("bg_" + image + ".png", "bg", "");
                if (special_cerberus && card.value == CERBERUS) {
                    // 彩蛋卡面是不带透明背景的照片，加圆角让它与卡片贴合
                    html += Img_("cerberus_special.png", "icon", "left: 13px; top: 12px; border-radius: 8px;");
                } else {
                    html += Img_(image + ".png", "icon", "left: 13px; top: 12px;");
                }
                html += "<div class=\"txt monster\" style=\"color: " + MonsterColor(card.value) + ";\">" +
                    MonsterName(card.value) + "</div>";
                break;
            }
            case CardType::EVENT: {
                html += Img_("bg_event.png", "bg", "");
                const pair<string, string> caption = EventCaption(card.value);
                html += "<div class=\"txt ev ev1\">" + caption.first + "</div>";
                html += "<div class=\"txt ev ev2\">" + caption.second + "</div>";
                break;
            }
        }
        return html + "</div>";
    }

    static string StateText_(const Player& player)
    {
        switch (player.state) {
            case PlayerState::EXPLORING: return "<span class=\"th-tag th-tag-exploring\">探险中</span>";
            case PlayerState::RETREATED: return "<span class=\"th-tag th-tag-retreated\">已撤离</span>";
            default:                     return "<span class=\"th-tag th-tag-devoured\">被吞噬</span>";
        }
    }

    string HeadHtml_() const
    {
        const uint32_t player_num = static_cast<uint32_t>(players.size());
        if (player_num == 0) {
            return "";
        }
        html::Table head(1, player_num);
        head.SetTableStyle(" align=\"center\" cellspacing=\"0\" cellpadding=\"0\" style=\"width:" +
                to_string(TableWidth()) + "px;\" ");
        for (uint32_t i = 0; i < player_num; ++i) {
            const Player& player = players[i];
            head.Get(0, i)
                .SetStyle("style=\"width:" + to_string(TableWidth() / static_cast<int>(player_num)) + "px;\"")
                .SetContent(
                    "<div class=\"th-player\">"
                        "<div class=\"th-avatar\">" + player.avatar + "</div>"
                        "<div class=\"th-info\">"
                            "<div class=\"th-name\">" + player.name + "</div>"
                            "<div class=\"th-state\">" + StateText_(player) + "</div>"
                            "<div class=\"th-gold\">金币：" + to_string(player.gold) + "</div>"
                        "</div>"
                    "</div>");
        }
        return head.ToString();
    }

    string CardsHtml_() const
    {
        // 加宽赛况图时，卡牌整体跟着居中，保持与状态栏一致的左右留白
        const int card_left = k_card_left + (TableWidth() - k_table_width) / 2;
        string html;
        for (size_t i = 0; i < past.size(); ++i) {
            const size_t column = i % k_cards_per_row;
            if (column == 0) {
                html += (i == 0 ? "" : "</div>");
                html += "<div class=\"th-row\">";
            }
            html += "<div class=\"th-slot\" style=\"left: " +
                to_string(card_left + k_card_pitch * static_cast<int>(column)) + "px;\">" +
                CardHtml_(past[i]) + "</div>";
        }
        return past.empty() ? html : html + "</div>";
    }


    // 把随机抽取的奇遇卡插入牌堆，其中预告类奇遇需要紧挨着它所预告的卡牌
    void InsertEventCards_()
    {
        vector<Card> all_events;
        for (int i = 1; i <= k_event_type_count; ++i) {
            all_events.push_back(MakeEventCard(i));
        }
        shuffle(all_events.begin(), all_events.end(), rng_);

        vector<Card> upcoming_events;
        for (int i = 0; i < k_event_num; ++i) {
            const Card& card = all_events[i];
            if (card.value <= UPCOMING_MONSTER) {
                upcoming_events.push_back(card);
            } else if (card.value == DOUBLE_WEALTH || card.value == EXTRA_WEALTH) {
                InsertApartFrom_(card, DOUBLE_WEALTH, EXTRA_WEALTH, CardType::WEALTH);
            } else if (card.value == DOUBLE_TREASURE || card.value == MAX_TREASURE) {
                InsertApartFrom_(card, DOUBLE_TREASURE, MAX_TREASURE, CardType::TREASURE);
            } else {
                pool.insert(pool.begin() + Rand(0, static_cast<int>(pool.size()) - 1), card);
            }
        }

        vector<int> occupied;
        for (const Card& card : upcoming_events) {
            if (card.value == UPCOMING_WEALTH) {
                InsertUpcomingWealth_(card, occupied);
            } else if (card.value == UPCOMING_TREASURE) {
                InsertBefore_(card, CardType::TREASURE, occupied);
            } else {
                InsertBefore_(card, CardType::MONSTER, occupied);
            }
        }
    }

    // 同类增益奇遇之间至少隔着一张可以被它增益的卡牌，避免连续出现时相互浪费
    void InsertApartFrom_(const Card& card, const int same_kind_a, const int same_kind_b, const CardType between_type)
    {
        const int pool_size = static_cast<int>(pool.size());
        int index = -1;
        for (int i = 0; i < pool_size; ++i) {
            if (pool[i].IsEvent() && (pool[i].value == same_kind_a || pool[i].value == same_kind_b)) {
                index = i;
                break;
            }
        }
        if (index < 0) {
            pool.insert(pool.begin() + Rand(0, pool_size - 1), card);
            return;
        }
        int pos = -1;
        for (int attempt = 0; attempt < k_max_attempt_ && pos < 0; ++attempt) {
            const int candidate = Rand(0, pool_size - 1);
            const int begin = min(candidate, index);
            const int end = max(candidate, index);
            for (int i = begin; i < end; ++i) {
                if (pool[i].type == between_type) {
                    pos = candidate;
                    break;
                }
            }
        }
        pool.insert(pool.begin() + (pos < 0 ? Rand(0, pool_size - 1) : pos), card);
    }

    // 收集所有目标卡牌之前 0~2 张的位置，作为预告类奇遇的候选插入点
    vector<int> CollectPositionsBefore_(const CardType type, const int min_wealth_value,
                                        const vector<int>& occupied) const
    {
        vector<int> positions;
        const int pool_size = static_cast<int>(pool.size());
        for (int i = 0; i < pool_size; ++i) {
            if (pool[i].type != type || pool[i].value < min_wealth_value) {
                continue;
            }
            positions.push_back(i);
            if (i > 0) {
                positions.push_back(i - 1);
            }
            if (i > 1) {
                positions.push_back(i - 2);
            }
        }
        positions.erase(remove_if(positions.begin(), positions.end(),
                    [&occupied](const int p) { return find(occupied.begin(), occupied.end(), p) != occupied.end(); }),
                positions.end());
        return positions;
    }

    void InsertAt_(const Card& card, const int pos, vector<int>& occupied)
    {
        pool.insert(pool.begin() + pos, card);
        occupied.push_back(pos + 1);
        occupied.push_back(pos + 2);
        occupied.push_back(pos + 3);
    }

    void InsertBefore_(const Card& card, const CardType type, vector<int>& occupied)
    {
        const vector<int> positions = CollectPositionsBefore_(type, 0, occupied);
        if (positions.empty()) {
            pool.insert(pool.begin() + Rand(0, static_cast<int>(pool.size()) - 1), card);
            return;
        }
        InsertAt_(card, positions[Rand(0, static_cast<int>(positions.size()) - 1)], occupied);
    }

    // 优先安排在数额较大的宝藏之前，找不到时把后续的宝藏全部提升到较大数额
    void InsertUpcomingWealth_(const Card& card, vector<int>& occupied)
    {
        const vector<int> rich_positions = CollectPositionsBefore_(CardType::WEALTH, k_rich_wealth_value_, occupied);
        if (!rich_positions.empty()) {
            InsertAt_(card, rich_positions[Rand(0, static_cast<int>(rich_positions.size()) - 1)], occupied);
            return;
        }
        const vector<int> positions = CollectPositionsBefore_(CardType::WEALTH, 0, occupied);
        if (positions.empty()) {
            pool.insert(pool.begin() + Rand(0, static_cast<int>(pool.size()) - 1), card);
            return;
        }
        const int pos = positions[Rand(0, static_cast<int>(positions.size()) - 1)];
        InsertAt_(card, pos, occupied);
        for (int i = pos; i < static_cast<int>(pool.size()); ++i) {
            if (!pool[i].IsWealth()) {
                continue;
            }
            pool[i] = MakeWealthCard(Rand(k_rich_wealth_value_, 15));
        }
    }

    // 卡面与状态栏使用的字体，字体文件随游戏资源一同发布
    static constexpr const char* k_number_font_ = "ErasDemiITC";  // 宝藏与珍宝的数字
    static constexpr const char* k_monster_font_ = "SimKai";      // 怪物名称
    static constexpr const char* k_event_font_ = "STXinwei";      // 奇遇卡的文字
    static constexpr const char* k_tag_font_ = "DengXianBold";    // 珍宝卡的独享提示
    static constexpr const char* k_text_font_ = "MSYaHei";        // 状态栏与提示文字

    static constexpr int k_rich_wealth_value_ = 10;  // 预告卡认定的「数额较大」门槛
    static constexpr int k_max_attempt_ = 100;

    mt19937 rng_;
};

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

#endif // TREASURE_HUNTER_BOARD_H
