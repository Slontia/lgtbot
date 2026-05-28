// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "match_process/msg_fragment_ipc.h"
#include "utility/utils.h"

namespace lgtbot::ipc {

namespace {

MsgItem MsgFragmentToItem(const MsgFragment& frag)
{
    MsgItem proto;
    std::visit(Overload{
        [&](const std::string& text) { proto.set_text(text); },
        [&](const At<UserID>& at) { proto.set_user_id(at.id_.GetStr()); },
        [&](const Name<UserID>& name) { proto.set_user_id(name.id_.GetStr()); },
        [&](const At<PlayerID>& at) { proto.set_at_player_id(at.id_.Get()); },
        [&](const Name<PlayerID>& name) { proto.set_at_player_id(name.id_.Get()); },
        [&](const ::Image& image) { proto.set_image_path(image.path_); },
        [&](const ::Markdown& markdown) {
            auto* md = proto.mutable_markdown();
            md->set_text(markdown.content_);
            md->set_width(markdown.width_);
        },
    }, frag);
    return proto;
}

} // namespace

std::vector<MsgItem> MsgFragmentsToItems(std::vector<MsgFragment> fragments)
{
    std::vector<MsgItem> items;
    items.reserve(fragments.size());
    for (const auto& frag : fragments) {
        items.push_back(MsgFragmentToItem(frag));
    }
    return items;
}

} // namespace lgtbot::ipc
