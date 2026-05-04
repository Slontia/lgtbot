#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "bot_core/bot_core.h"
#include "lgtbot_grpc_service.h"

namespace {

const char* EnvOr(const char* key, const char* fallback)
{
    const char* v = std::getenv(key);
    return (v && v[0]) ? v : fallback;
}

} // namespace

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    const std::string game_path = EnvOr("LGTBOT_GAME_PATH", "plugins");
    const std::string image_path = EnvOr("LGTBOT_IMAGE_PATH", "/tmp/lgtbot_images");
    const std::string db_path = EnvOr("LGTBOT_DB_PATH", "");
    const std::string listen = EnvOr("LGTBOT_GRPC_LISTEN", "unix:///tmp/lgtbot.sock");

    LgtbotGrpcService service(image_path, game_path);

    LGTBot_Option opt = LGTBot_InitOptions();
    opt.game_path_ = game_path.c_str();
    opt.image_path_ = image_path.c_str();
    opt.db_path_ = db_path.empty() ? nullptr : db_path.c_str();
    opt.handler_ = &service;
    FillLgtbotCallbacks(&opt.callbacks_);

    const char* init_err = nullptr;
    void* bot = LGTBot_Create(&opt, &init_err);
    if (!bot) {
        std::cerr << "LGTBot_Create failed: " << (init_err ? init_err : "(null)") << "\n";
        return 1;
    }
    service.BindBot(bot);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(listen, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server) {
        std::cerr << "Failed to start gRPC server on " << listen << "\n";
        LGTBot_Release(bot);
        return 1;
    }
    std::cerr << "lgtbot_grpc_server listening on " << listen << "\n";
    server->Wait();
    LGTBot_Release(bot);
    return 0;
}
