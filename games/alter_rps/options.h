#ifndef ALTER_RPS_OPTIONS_H_
#define ALTER_RPS_OPTIONS_H_

constexpr const uint32_t k_card_type_num = 4;
enum class Type : int { ROCK = 0, PAPER = 1, SCISSOR = 2, BLANK = 3 };

struct Card
{
    Type type_;
    int point_; // 0 indicate a negative point card
    auto operator<=>(const Card&) const = default;
};

class CardChecker : public MsgArgChecker<Card>
{
  public:
    CardChecker() : arith_checker_(1, 10, "点数") {}
    virtual ~CardChecker() {}

    virtual std::string FormatInfo() const override
    {
        return "(石头|剪刀|布|空白)(-|" + arith_checker_.FormatInfo() + ")";
    }

    virtual std::string EscapedFormatInfo() const override
    {
        return "(石头|剪刀|布|空白)(-|" + arith_checker_.EscapedFormatInfo() + ")";
    }

    virtual std::string ColoredFormatInfo() const override
    {
        return HTML_COLOR_FONT_HEADER(orange) + EscapedFormatInfo() + HTML_FONT_TAIL;
    }

    virtual std::string ExampleInfo() const override
    {
        return "剪刀" + arith_checker_.ExampleInfo();
    }

    virtual std::optional<Card> Check(MsgReader& reader) const
    {
        if (!reader.HasNext()) {
            return std::nullopt;
        }
        const auto str = reader.NextArg();
        const auto parse_card_number = [&](const std::string_view prefix) -> std::optional<int>
            {
                if (!str.starts_with(prefix)) {
                    return std::nullopt;
                }
                const auto number_str = str.substr(prefix.size());
                if (number_str == "-") {
                    return 0;
                }
                return arith_checker_.Check(number_str);
            };
        std::optional<int> point = 0;
        if (point = parse_card_number("剪刀")) {
            return Card{.type_ = Type::SCISSOR, .point_ = *point};
        } else if (point = parse_card_number("石头")) {
            return Card{.type_ = Type::ROCK, .point_ = *point};
        } else if (point = parse_card_number("布")) {
            return Card{.type_ = Type::PAPER, .point_ = *point};
        } else if (point = parse_card_number("空白")) {
            return Card{.type_ = Type::BLANK, .point_ = *point};
        }
        return std::nullopt;
    }

    virtual std::string ArgString(const Card& value) const
    {
        return (value.type_ == Type::ROCK ? "石头" :
                value.type_ == Type::PAPER ? "布" :
                value.type_ == Type::SCISSOR ? "剪刀" : "空白") + (value.point_ == 0 ? "-" : std::to_string(value.point_));
    }

  private:
    ArithChecker<int> arith_checker_;
};

constexpr const uint32_t k_round_num = 9;

#endif

EXTEND_OPTION("每回合出牌时间", 时限, (ArithChecker<uint32_t>(10, 300, "时间（秒）")), 90)
EXTEND_OPTION("设置双方的手牌（空表示随机手牌）", 手牌, (OptionalChecker<FixedSizeRepeatableChecker<CardChecker>>(k_round_num)), std::nullopt)
EXTEND_OPTION("固定左拳为上一回合未打出的牌", 固定左拳, (BoolChecker("开启", "关闭")), true)
