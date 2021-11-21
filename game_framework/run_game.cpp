#include <chrono>

#include <gflags/gflags.h>

#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "game_framework/game_main.h"
#include "game_framework/mock_match.h"

DEFINE_uint64(player, 0, "Player number: if set to 0, best player num will be set");
DEFINE_uint64(repeat, 1, "Repeat times: if set to 0, will run unlimitedly");
DEFINE_string(resource_dir, "", "The path of game image resources");

MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options, MatchBase& match);

extern inline bool enable_markdown_to_image;

int Run()
{
    MockMatch match(FLAGS_player);

    GameOption option;
    option.SetPlayerNum(FLAGS_player);
    if (!FLAGS_resource_dir.empty()) {
        enable_markdown_to_image = true;
        option.SetResourceDir(std::filesystem::absolute(FLAGS_resource_dir).c_str());
    }

    MockMsgSender sender;
    std::unique_ptr<MainStageBase> main_stage(MakeMainStage(sender, option, match));
    if (!main_stage) {
        std::cerr << "Start Game Failed!" << std::endl;
        return -1;
    }
    main_stage->HandleStageBegin();

    uint64_t ok_count = 0;
    for (uint64_t i = 0; !main_stage->IsOver() && ok_count < FLAGS_player; i = (i + 1) % FLAGS_player) {
        if (match.IsEliminated(i) || StageErrCode::OK == main_stage->HandleComputerAct(i)) {
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


int main(int argc, char** argv)
{
    std::srand(std::chrono::steady_clock::now().time_since_epoch().count());

    std::locale::global(std::locale(""));
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_player == 0) {
        FLAGS_player = GameOption().BestPlayerNum();
    }

    for (uint64_t i = 0; i < FLAGS_repeat; ++i) {
        Run();
    }

    return 0;
}
