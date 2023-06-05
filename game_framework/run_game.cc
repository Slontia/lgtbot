// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <chrono>

#include <gflags/gflags.h>

#include "game_framework/game_options.h"
#include "game_framework/game_main.h"
#include "game_framework/mock_match.h"

DEFINE_uint64(player, 0, "Player number: if set to 0, best player num will be set");
DEFINE_uint64(repeat, 1, "Repeat times: if set to 0, will run unlimitedly");
DEFINE_string(resource_dir, "./resource_dir/", "The path of game image resources");
DEFINE_bool(gen_image, false, "Whether generate image or not");
DEFINE_string(image_path, "./.lgtbot_image/", "The path of directory to store generated images");

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
        const std::string avatar_filename = (image_path() / "avatar" / (std::to_string(pid) + ".png")).string();
        CharToImage('0' + pid, avatar_filename);
        thread_local static std::string str;
        str = "<img src=\"file://" + avatar_filename + "\" style=\"width:" + std::to_string(size) + "px; height:" +
            std::to_string(size) + "px; border-radius:50%; vertical-align: middle;\"/>";
        return str.c_str();
    }
};

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match);

int Run(const uint64_t index)
{
    static const auto image_path_base = std::filesystem::absolute(FLAGS_image_path) /
        std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

    const auto image_path = image_path_base / std::to_string(index);

    RunGameMockMatch match(image_path, FLAGS_player);

    enable_markdown_to_image = FLAGS_gen_image && !FLAGS_image_path.empty();

    GameOption option;
    option.SetPlayerNum(FLAGS_player);
    option.SetResourceDir(std::filesystem::absolute(FLAGS_resource_dir + "/").string().c_str());

    MockMsgSender sender(image_path);
    std::unique_ptr<MainStageBase> main_stage(MakeMainStage(sender, option, match));
    if (!main_stage) {
        std::cerr << "Start Game Failed!" << std::endl;
        return -1;
    }
    main_stage->HandleStageBegin();

    uint64_t ok_count = 0;
    for (uint64_t i = 0; !main_stage->IsOver() && ok_count < FLAGS_player; i = (i + 1) % FLAGS_player) {
        if (match.IsEliminated(i) || StageErrCode::OK == main_stage->HandleComputerAct(i, true)) {
            ++ok_count;
        } else {
            ok_count = 0;
        }
    }

    assert(main_stage->IsOver());

    {
        auto sender_guard = sender();
        sender_guard << "分数结果";
        for (PlayerID pid = 0; pid < FLAGS_player; ++pid) {
            sender_guard << "\n" << Name(pid) << "：" << main_stage->PlayerScore(pid);
        }
    }
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

    if (FLAGS_player == 0) {
        FLAGS_player = lgtbot::game::GAME_MODULE_NAME::GameOption().BestPlayerNum();
    }

    for (uint64_t i = 0; i < FLAGS_repeat; ++i) {
        lgtbot::game::GAME_MODULE_NAME::Run(i);
    }

    return 0;
}
