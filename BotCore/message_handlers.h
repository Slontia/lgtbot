#pragma once
#include "bot_core.h"
#include "Utility/msg_checker.h"
#include <type_traits>

template <typename TRef, typename T> concept UniRef = std::is_same_v<std::decay_t<TRef>, T>;

using MetaUserFuncType = std::string(const UserID, const std::optional<GroupID>);
using MetaCommand = MsgCommand<MetaUserFuncType>;

extern const std::vector<std::shared_ptr<MetaCommand>> meta_cmds;
extern const std::vector<std::shared_ptr<MetaCommand>> admin_cmds;

extern std::string HandleRequest(const UserID uid, const std::optional<GroupID> gid, MsgReader& reader, const std::vector<std::shared_ptr<MetaCommand>>& cmds, const std::string& type);

template <UniRef<MsgReader> ReaderRef>
std::string HandleMetaRequest(const UserID uid, const std::optional<GroupID> gid, ReaderRef&& reader)
{
	return HandleRequest(uid, gid, reader, meta_cmds, "ิช");
}

template <UniRef<MsgReader> ReaderRef>
std::string HandleAdminRequest(const UserID uid, const std::optional<GroupID> gid, ReaderRef&& reader)
{
	return HandleRequest(uid, gid, reader, admin_cmds, "นÜภํ");
}
