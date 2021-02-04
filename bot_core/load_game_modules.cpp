
#include "util.h"
#include "log.h"
#include "match.h"
#include "db_manager.h"
#include "utility/msg_sender.h"
#include <regex>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#define HINSTANCE void*
#define GetProcAddress dlsym
#define FreeLibrary dlclose
#else
static_assert(false, "Not support OS");
#endif

template <typename Sender>
class MsgSenderForGame : public MsgSender
{
 public:
  MsgSenderForGame(Match& match, Sender&& sender) : match_(match), sender_(std::forward<Sender>(sender)) {}
  virtual ~MsgSenderForGame() {}

  virtual void SendString(const char* const str, const size_t len)
  {
    sender_ << std::string_view(str, len);
  }

  virtual void SendAt(const uint64_t pid)
  {
    sender_ << AtMsg(match_.pid2uid(pid));
  }
 private:
  Match& match_;
  Sender sender_;
};

static MsgSender* Boardcast(void* match_p)
{
  Match& match = *static_cast<Match*>(match_p);
  return new MsgSenderForGame(match, match.Boardcast());
}

static MsgSender* Tell(void* match_p, const uint64_t pid)
{
  Match& match = *static_cast<Match*>(match_p);
  return new MsgSenderForGame(match, match.Tell(pid));
}

static void DeleteMsgSender(MsgSender* const msg_sender)
{
  delete msg_sender;
}

static void MatchGameOver(void* match, const int64_t scores[])
{
  Match& match_ref = *static_cast<Match*>(match);
  match_ref.GameOver(scores);
  match_ref.match_manager().DeleteMatch(match_ref.mid());
}

static void StartTimer(void* match, const uint64_t sec)
{
  static_cast<Match*>(match)->StartTimer(sec);
}

static void StopTimer(void* match)
{
  static_cast<Match*>(match)->StopTimer();
}

static void LoadGame(HINSTANCE mod, GameHandleMap& game_handles)
{
  if (!mod)
  {
#ifdef __linux__
    ErrorLog() << "Load mod failed: " << dlerror();
#else
    ErrorLog() << "Load mod failed";
#endif
    return;
  }

  typedef int (/*__cdecl*/ *Init)(const NEW_BOARDCAST_MSG_SENDER_CALLBACK, const NEW_TELL_MSG_SENDER_CALLBACK, const DELETE_MSG_SENDER_CALLBACK, const GAME_OVER_CALLBACK, const START_TIMER_CALLBACK, const STOP_TIMER_CALLBACK);
  typedef char* (/*__cdecl*/ *GameInfo)(uint64_t*, uint64_t*, const char**);
  typedef GameBase* (/*__cdecl*/ *NewGame)(void* const match);
  typedef int (/*__cdecl*/ *DeleteGame)(GameBase* const);

  Init init = (Init)GetProcAddress(mod, "Init");
  GameInfo game_info = (GameInfo)GetProcAddress(mod, "GameInfo");
  NewGame new_game = (NewGame)GetProcAddress(mod, "NewGame");
  DeleteGame delete_game = (DeleteGame)GetProcAddress(mod, "DeleteGame");

  if (!init || !game_info || !new_game || !delete_game)
  {
    ErrorLog() << "Load failed: some interface not be defined." << std::boolalpha << static_cast<bool>(new_game);
    return;
  }

  if (!init(&Boardcast, &Tell, &DeleteMsgSender, &MatchGameOver, &StartTimer, &StopTimer))
  {
    ErrorLog() << "Load failed: init failed";
    return;
  }

  const char* rule = nullptr;
  uint64_t min_player = 0;
  uint64_t max_player = 0;
  char* name = game_info(&min_player, &max_player, &rule);
  if (!name)
  {
    ErrorLog() << "Load failed: Cannot get game game";
    return;
  }
  if (min_player == 0 || max_player < min_player)
  {
    ErrorLog() << "Load failed: Invalid min_player:" << min_player << " max_player:" << max_player;
    return;
  }
  std::optional<uint64_t> game_id;
  if (const std::unique_ptr<DBManager>& db_manager = DBManager::GetDBManager(); db_manager != nullptr)
  {
    game_id = db_manager->GetGameIDWithName(name);
  }
  game_handles.emplace(name, std::make_unique<GameHandle>(game_id, name, min_player, max_player, rule, new_game, delete_game, [mod] { FreeLibrary(mod); }));
  InfoLog() << "Loaded successfully!";
}

void BotCtx::LoadGameModules_(const std::string_view games_path)
{
#ifdef _WIN32
  WIN32_FIND_DATA file_data;
  HANDLE file_handle = FindFirstFile(games_path + L"\\*.dll", &file_data);
  if (file_handle == INVALID_HANDLE_VALUE) { return; }
  do
  {
    std::wstring dll_path = L".\\plugins\\" + std::wstring(file_data.cFileName);
    LoadGame(LoadLibrary(dll_path.c_str()), game_handles_);
  } while (FindNextFile(file_handle, &file_data));
  FindClose(file_handle);
  InfoLog() << "Load module count: " << game_handles_.size();
#elif __linux__
  DIR *d = opendir(games_path.data());
  if (!d)
  {
    ErrorLog() << "opendir failed";
    return;
  }
  const std::regex base_regex(".*so");
  std::smatch base_match;
  struct stat st;
  for (dirent* dp = nullptr; (dp = readdir(d)) != NULL; )
  {
    std::string name(dp->d_name);
    if (std::regex_match(name, base_match, base_regex); base_match.empty())
    {
      DebugLog() << "Find irrelevant file " << name << ", skip";
      continue;
    }
    if (stat(dp->d_name, &st); S_ISDIR(st.st_mode))
    {
      DebugLog() << "Find directory " << name << ", skip";
      continue;
    }
    InfoLog() << "Loading library " << name;
    LoadGame(dlopen((std::string("./plugins/") + name).c_str(), RTLD_LAZY), game_handles_);
  }
  InfoLog() << "Loading finished.";
  closedir(d);
#endif
}

