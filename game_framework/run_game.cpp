#include <gflags/gflags.h>

#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "game_framework/game_main.h"
#include "game_framework/mock_match.h"

DEFINE_uint64(player_num, 2, "Player number");

MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options);

int main(int argc, char** argv)
{
    std::locale::global(std::locale(""));
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    MockMatch match;

    GameOption option;
    option.SetPlayerNum(FLAGS_player_num);

    MockMsgSender sender;
    std::unique_ptr<MainStageBase> main_stage(MakeMainStage(sender, option));
    if (!main_stage) {
        std::cerr << "Start Game Failed!" << std::endl;
    }
    main_stage->Init(&match);

    for (PlayerID pid = 0; !main_stage->IsOver(); pid = (pid + 1) % FLAGS_player_num) {
        main_stage->HandleComputerAct(0, FLAGS_player_num);
    }
    return 0;
}
