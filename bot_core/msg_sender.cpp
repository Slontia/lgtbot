#include "msg_sender.h"
#include "bot_core/match.h"

void MsgSender::SavePlayer(const PlayerID& pid, const bool is_at)
{
    if (!match_) {
        SaveText_("[" + std::to_string(pid) + "号玩家]");
        return;
    }
    SaveText_("[" + std::to_string(pid) + "号]<");
    const auto& id = match_->ConvertPid(pid);
    if (const auto pval = std::get_if<ComputerID>(&id)) {
        SaveText_("机器人" + std::to_string(*pval) + "号");
    } else {
        SaveUser(std::get<UserID>(id), is_at);
    }
    SaveText_(">");
}

