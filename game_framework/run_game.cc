// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <chrono>

#include <gflags/gflags.h>

#include "game_framework/util.h"
#include "game_framework/game_options.h"
#include "game_framework/game_main.h"
#include "game_framework/mock_match.h"
#include "game_framework/stage.h"

DEFINE_uint64(player, 0, "Player number: if set to 0, best player num will be set");
DEFINE_uint64(repeat, 1, "Repeat times: if set to 0, will run unlimitedly");
DEFINE_string(resource_dir, "./resource_dir/", "The path of game image resources");
DEFINE_bool(gen_image, false, "Whether generate image or not");
DEFINE_string(image_dir, "./.lgtbot_image/", "The path of directory to store generated images");
DEFINE_bool(input_options, false, "Input the game options by stdin");

extern bool enable_markdown_to_image;

class RunGameMockMatch : public MockMatch
{
  public:
    using MockMatch::MockMatch;

    virtual const char* PlayerAvatar(const PlayerID& pid, const int32_t size) override
    {
        if (!FLAGS_gen_image) {
            return "";
        }
        const std::string avatar_filename = (image_dir() / ("avatar_" + std::to_string(pid) + ".png")).string();
        CharToImage('0' + pid, avatar_filename);
        thread_local static std::string str;
        str = "<img src=\"file:///" + avatar_filename + "\" style=\"width:" + std::to_string(size) + "px; height:" +
            std::to_string(size) + "px; border-radius:50%; vertical-align: middle;\"/>";
        return str.c_str();
    }
};

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

internal::MainStage* MakeMainStage(MainStageFactory factory);

struct OptionsArgs
{
    std::string resource_dir_;
    std::string saved_image_dir_;
};

struct Options
{
    Options(OptionsArgs args)
        : args_(std::move(args))
        , generic_options_{
            ImmutableGenericOptions{
                .user_num_ = 0,
                .resource_dir_ = args_.resource_dir_.c_str(),
                .saved_image_dir_ = args_.saved_image_dir_.c_str(),
            },
            MutableGenericOptions{}
        }
    {
    }

    // Disable copy/move constructor to ensure the correctness of `resource_dir_` and `saved_image_dir_` in `generic_options_`.
    Options(const Options&) = delete;
    Options(Options&&) = delete;

    OptionsArgs args_;
    GameOptions game_options_;
    GenericOptions generic_options_;
};

void ReadGameOptionsFromStdin(GameOptions& game_options)
{
    const auto cin_g = std::cin.tellg();
    for (std::string line; std::getline(std::cin, line); ) {
        if (!game_options.SetOption(line.c_str())) {
            throw std::runtime_error{"Unexpected option: " + line};
        }
    }
    std::cin.seekg(cin_g);
}

void SetPlayerNumber(Options& options)
{
    if (FLAGS_player != 0) {
        options.generic_options_.bench_computers_to_player_num_ = FLAGS_player;
    } else if (MsgReader reader{"单机"}; std::ranges::none_of(k_init_options_commands,
                [&](const auto& cmd) { return cmd.CallIfValid(reader, options.game_options_, options.generic_options_).has_value(); })) {
        throw std::runtime_error{"The game does not support single-player mode"};
    }
}

void AdaptOptions(MockMsgSender& sender, Options& options)
{
    if (!AdaptOptions(sender, options.game_options_, options.generic_options_, options.generic_options_)) {
        throw std::runtime_error{"Invalid options!"};
    }
}

void InitOptions(MockMsgSender& sender, Options& options)
{
    if (FLAGS_input_options) {
        ReadGameOptionsFromStdin(options.game_options_);
    }
    SetPlayerNumber(options);
    AdaptOptions(sender, options);
}

std::unique_ptr<internal::MainStage> StartMainStage(Options& options, RunGameMockMatch& match)
{
    std::unique_ptr<internal::MainStage> main_stage{lgtbot::game::GAME_MODULE_NAME::MakeMainStage(
            MainStageFactory{options.game_options_, options.generic_options_, match})};
    if (!main_stage) {
        throw std::runtime_error{"Start Game Failed!"};
    }
    main_stage->HandleStageBegin();
    return main_stage;
}

void KeepPlayersActUntilGameOver(const Options& options, const RunGameMockMatch& match, MainStageBase& main_stage)
{
    uint64_t ok_count = 0;
    for (uint64_t i = 0;
            !main_stage.IsOver() && ok_count < options.generic_options_.bench_computers_to_player_num_;
            i = (i + 1) % options.generic_options_.bench_computers_to_player_num_) {
        if (match.IsEliminated(i) || StageErrCode::OK == main_stage.HandleComputerAct(i, true)) {
            ++ok_count;
        } else {
            ok_count = 0;
        }
    }
}

void ShowScores(MockMsgSender& sender, const Options& options, const MainStageBase& main_stage)
{
    auto sender_guard = sender();
    sender_guard << "分数结果";
    for (PlayerID pid = 0; pid < options.generic_options_.bench_computers_to_player_num_; ++pid) {
        sender_guard << "\n" << Name(pid) << "：" << main_stage.PlayerScore(pid);
    }
}

int Run(const uint64_t index)
{
    static const auto image_dir_base = std::filesystem::absolute(FLAGS_image_dir) /
        std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

    Options options{OptionsArgs{
        .resource_dir_ = std::filesystem::absolute(FLAGS_resource_dir + "/").string(),
        .saved_image_dir_ = (image_dir_base / std::to_string(index)).string(),
    }};
    MockMsgSender sender(options.generic_options_.saved_image_dir_);
    InitOptions(sender, options);
    RunGameMockMatch match{
        options.generic_options_.saved_image_dir_,
        options.generic_options_.bench_computers_to_player_num_
    };
    const auto main_stage = StartMainStage(options, match);
    KeepPlayersActUntilGameOver(options, match, *main_stage);
    assert(main_stage->IsOver());
    ShowScores(sender, options, *main_stage);
    return 0;
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

int main(int argc, char** argv)
{
    std::srand(std::chrono::steady_clock::now().time_since_epoch().count());

#ifdef __linux__
    std::locale::global(std::locale(""));
#endif
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    enable_markdown_to_image = FLAGS_gen_image && !FLAGS_image_dir.empty();

    for (uint64_t i = 0; i < FLAGS_repeat; ++i) {
        lgtbot::game::GAME_MODULE_NAME::Run(i);
    }

    return 0;
}
