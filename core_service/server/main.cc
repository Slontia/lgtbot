#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
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

// `--name=value` or `--name value`; empty after `=` yields std::nullopt.
std::optional<std::string> LongOpt(int argc, char** argv, const char* name)
{
    const std::string eq = std::string(name) + "=";
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        if (std::strncmp(a, eq.c_str(), eq.size()) == 0) {
            std::string v(a + eq.size());
            return v.empty() ? std::optional<std::string>() : std::optional<std::string>(std::move(v));
        }
        if (std::strcmp(a, name) == 0 && i + 1 < argc) {
            return std::string(argv[++i]);
        }
    }
    return std::nullopt;
}

void LogEffectivePath(const char* label, const std::filesystem::path& p)
{
    namespace fs = std::filesystem;
    try {
        std::cerr << "lgtbot_grpc_server: " << label << "=" << fs::weakly_canonical(fs::absolute(p)) << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "lgtbot_grpc_server: " << label << "=" << p.string() << " (canonicalize: " << e.what() << ")\n";
    }
}

} // namespace

int main(int argc, char** argv)
{
    namespace fs = std::filesystem;
    auto game_override = LongOpt(argc, argv, "--game-path");
    auto image_override = LongOpt(argc, argv, "--image-path");

    const std::string game_path = game_override.value_or(std::string(EnvOr("LGTBOT_GAME_PATH", "plugins")));
    const std::string image_path =
        image_override.value_or(std::string(EnvOr("LGTBOT_IMAGE_PATH", "/tmp/lgtbot_images")));

    if (game_override.has_value()) {
        std::cerr << "lgtbot_grpc_server: --game-path overrides LGTBOT_GAME_PATH\n";
    }
    if (image_override.has_value()) {
        std::cerr << "lgtbot_grpc_server: --image-path overrides LGTBOT_IMAGE_PATH\n";
    }
    LogEffectivePath("game_path_effective", game_path);
    LogEffectivePath("image_path_effective", image_path);

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
