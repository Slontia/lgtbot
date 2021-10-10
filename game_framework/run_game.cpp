#include <chrono>

#include <gflags/gflags.h>

#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "game_framework/game_main.h"
#include "game_framework/mock_match.h"

DEFINE_uint64(player, 2, "Player number"); // TODO: if set to 0, random player for each run
DEFINE_uint64(repeat, 1, "Repeat times"); // TODO: if set to 0, run unlimitedly

MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options, MatchBase& match);

int Run()
{
    MockMatch match;

    GameOption option;
    option.SetPlayerNum(FLAGS_player);

    MockMsgSender sender;
    std::unique_ptr<MainStageBase> main_stage(MakeMainStage(sender, option, match));
    if (!main_stage) {
        std::cerr << "Start Game Failed!" << std::endl;
        return -1;
    }
    main_stage->HandleStageBegin();

    uint64_t ok_count = 0;
    for (uint64_t i = 0; !main_stage->IsOver() && ok_count < FLAGS_player; i = (i + i) % FLAGS_player) {
        if (StageErrCode::OK == main_stage->HandleComputerAct(i)) {
            ++ok_count;
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

    for (uint64_t i = 0; i < FLAGS_repeat; ++i) {
        Run();
    }

    return 0;
}
