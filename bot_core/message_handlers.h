#pragma once
#include "util.h"
#include "bot_core.h"
#include "utility/msg_checker.h"
#include "utility/msg_sender.h"
#include <type_traits>

using MetaUserFuncType = ErrCode(const UserID, const std::optional<GroupID>, MsgSenderWrapper*);
using MetaCommand = MsgCommand<MetaUserFuncType>;

extern const std::vector<std::shared_ptr<MetaCommand>> meta_cmds;
extern const std::vector<std::shared_ptr<MetaCommand>> admin_cmds;

ErrCode HandleRequest(const UserID uid, const std::optional<GroupID> gid, MsgReader& reader,
    MsgSenderWrapper& sender, const std::vector<std::shared_ptr<MetaCommand>>& cmds, const std::string_view type);

template <UniRef<MsgReader> ReaderRef>
ErrCode HandleMetaRequest(const UserID uid, const std::optional<GroupID> gid,
    ReaderRef&& reader, MsgSenderWrapper& sender)
{
	return HandleRequest(uid, gid, reader, sender, meta_cmds, "元");
}

template <UniRef<MsgReader> ReaderRef>
ErrCode HandleAdminRequest(const UserID uid, const std::optional<GroupID> gid,
    ReaderRef&& reader, MsgSenderWrapper& sender)
{
	return HandleRequest(uid, gid, reader, sender, admin_cmds, "管理");
}
