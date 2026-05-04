#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <grpcpp/grpcpp.h>

#include "bot_core/bot_core.h"
#include "lgtbot_service.grpc.pb.h"

#include "push_queue.h"

void FillLgtbotCallbacks(LGTBot_Callback* out);

struct UserCacheEntry {
    std::string display_name;
    std::string avatar_rel_url;
};

// gRPC façade over existing LGTBot C API. Uses std threading (compatible with grpc thread pool).
// NOTE: Phase 3 plan targets brpc+bthread; this binary uses gRPC++ so grpc-go works unchanged.
class LgtbotGrpcService final : public lgtbot::LGTBotService::Service {
  public:
    explicit LgtbotGrpcService(std::string image_root, std::string game_path);
    ~LgtbotGrpcService() override;

    void BindBot(void* bot_handle) { bot_ = bot_handle; }

    grpc::Status HandlePrivateRequest(grpc::ServerContext* ctx, const lgtbot::PrivateRequest* req,
            lgtbot::PrivateResponse* resp) override;

    grpc::Status Subscribe(grpc::ServerContext* ctx, const lgtbot::SubscribeRequest* req,
            grpc::ServerWriter<lgtbot::PushEvent>* writer) override;

    grpc::Status GetCommands(grpc::ServerContext* ctx, const lgtbot::GetCommandsRequest* req,
            lgtbot::GetCommandsResponse* resp) override;

    grpc::Status GetUserInfo(grpc::ServerContext* ctx, const lgtbot::GetUserInfoRequest* req,
            lgtbot::GetUserInfoResponse* resp) override;

    grpc::Status GetMatchHistory(grpc::ServerContext* ctx, const lgtbot::GetMatchHistoryRequest* req,
            grpc::ServerWriter<lgtbot::Message>* writer) override;

    grpc::Status UpdateUserInfo(grpc::ServerContext* ctx, const lgtbot::UpdateUserInfoRequest* req,
            lgtbot::UpdateUserInfoResponse* resp) override;

    grpc::Status RecordChatMessage(grpc::ServerContext* ctx, const lgtbot::RecordChatMessageRequest* req,
            lgtbot::RecordChatMessageResponse* resp) override;

    grpc::Status GetGameList(grpc::ServerContext* ctx, const lgtbot::GetGameListRequest* req,
            lgtbot::GetGameListResponse* resp) override;

    grpc::Status StartGame(grpc::ServerContext* ctx, const lgtbot::StartGameRequest* req,
            lgtbot::StartGameResponse* resp) override;

    void OnGetUserName(char* buffer, size_t size, const char* user_id);
    void OnGetUserNameInGroup(char* buffer, size_t size, const char* group_id, const char* user_id);
    int OnDownloadUserAvatar(const char* user_id, const char* dest_filename);
    void OnHandleMessages(const char* id, int is_to_user, const LGTBot_Message* messages, size_t size);

  private:
    PushQueue* QueueForPlatform(const std::string& platform);
    std::string CacheKey(const std::string& platform, const std::string& uid) const;

    void* bot_{nullptr};
    std::string image_root_;
    std::string game_path_;

    std::mutex queues_mu_;
    std::unordered_map<std::string, std::unique_ptr<PushQueue>> push_queues_;

    std::mutex cache_mu_;
    std::unordered_map<std::string, UserCacheEntry> user_cache_;

    std::atomic<uint64_t> event_seq_{0};
};
