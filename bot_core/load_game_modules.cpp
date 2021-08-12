
#include <regex>

#include "db_manager.h"
#include "log.h"
#include "match.h"
#include "util.h"
#include "msg_sender.h"

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#define HINSTANCE void*
#define GetProcAddress dlsym
#define FreeLibrary dlclose
#else
static_assert(false, "Not support OS");
#endif

MsgSenderBase::MsgSenderGuard Boardcast(void* match_p) { return static_cast<Match*>(match_p)->Boardcast(); }
MsgSenderBase::MsgSenderGuard Tell(void* match_p, const uint64_t pid) { return static_cast<Match*>(match_p)->Tell(pid); }
void StartTimer(void* match, const uint64_t sec) { static_cast<Match*>(match)->StartTimer(sec); }
void StopTimer(void* match) { static_cast<Match*>(match)->StopTimer(); }

static void LoadGame(HINSTANCE mod, GameHandleMap& game_handles)
{
    if (!mod) {
#ifdef __linux__
        ErrorLog() << "Load mod failed: " << dlerror();
#else
        ErrorLog() << "Load mod failed";
#endif
        return;
    }

    typedef char*(*GameInfo)(uint64_t*, const char**);

    const auto load_proc = [&mod](const char* const name)
    {
        const auto proc = GetProcAddress(mod, name);
        if (!proc) {
            ErrorLog() << "Load proc " << name << " from module failed";
            std::cerr << dlerror() << std::endl;
        }
        return proc;
    };

    const auto game_info_fn = (GameInfo)load_proc("GameInfo");
    const auto game_options_allocator_fn = (GameHandle::game_options_allocator)load_proc("NewGameOptions");
    const auto game_options_deleter_fn = (GameHandle::game_options_deleter)load_proc("DeleteGameOptions");
    const auto main_stage_allocator_fn = (GameHandle::main_stage_allocator)load_proc("NewMainStage");
    const auto main_stage_deleter_fn = (GameHandle::main_stage_deleter)load_proc("DeleteMainStage");

    if (!game_info_fn || !game_options_allocator_fn || !game_options_deleter_fn || !main_stage_allocator_fn || !main_stage_deleter_fn) {
        return;
    }

    const char* rule = nullptr;
    uint64_t max_player = 0;
    char* name = game_info_fn(&max_player, &rule);
    if (!name) {
        ErrorLog() << "Load failed: Cannot get game game";
        return;
    }
    std::optional<uint64_t> game_id;
#ifdef WITH_MYSQL
    if (const std::unique_ptr<DBManager>& db_manager = DBManager::GetDBManager(); db_manager != nullptr) {
        game_id = db_manager->GetGameIDWithName(name);
    }
#endif
    game_handles.emplace(name, std::make_unique<GameHandle>(game_id, name, max_player, rule,
                                                            game_options_allocator_fn, game_options_deleter_fn,
                                                            main_stage_allocator_fn, main_stage_deleter_fn,
                                                            [mod] { FreeLibrary(mod); }));
    InfoLog() << "Loaded successfully!";
}

void BotCtx::LoadGameModules_(const char* const games_path)
{
    if (games_path == nullptr) {
        return;
    }
#ifdef _WIN32
    WIN32_FIND_DATA file_data;
    HANDLE file_handle = FindFirstFile((std::string(games_path) + "\\*.dll").c_str(), &file_data);
    if (file_handle == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        const auto dll_path = std::string(games_path) + "\\" + std::string(file_data.cFileName);
        LoadGame(LoadLibrary(dll_path.c_str()), game_handles_);
    } while (FindNextFile(file_handle, &file_data));
    FindClose(file_handle);
    InfoLog() << "Load module count: " << game_handles_.size();
#elif __linux__
    DIR* d = opendir(games_path);
    if (!d) {
        ErrorLog() << "opendir failed";
        return;
    }
    const std::regex base_regex(".*so");
    std::smatch base_match;
    struct stat st;
    for (dirent* dp = nullptr; (dp = readdir(d)) != NULL;) {
        std::string name(dp->d_name);
        if (std::regex_match(name, base_match, base_regex); base_match.empty()) {
            DebugLog() << "Find irrelevant file " << name << ", skip";
            continue;
        }
        InfoLog() << "Loading library " << name;
        LoadGame(dlopen((std::string(games_path) + "/" + name).c_str(), RTLD_LAZY), game_handles_);
    }
    InfoLog() << "Loading finished.";
    closedir(d);
#endif
}

