
#include "util.h"
#include "log.h"
#include "match.h"
#include "db_manager.h"
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

static void BoardcastPlayers(void* match, const char* const msg)
{
  static_cast<Match*>(match)->BoardcastPlayers(msg);
}

static void TellPlayer(void* match, const uint64_t pid, const char* const msg)
{
  static_cast<Match*>(match)->TellPlayer(pid, msg);
}

static const char* AtPlayer(void* match, const uint64_t pid)
{
  static thread_local std::string at_s;
  at_s = static_cast<Match*>(match)->AtPlayer(pid);
  return at_s.c_str();
}

static void MatchGameOver(void* match, const int64_t scores[])
{
  static_cast<Match*>(match)->GameOver(scores);
  MatchManager::DeleteMatch(static_cast<Match*>(match)->mid());
}

static void StartTimer(void* match, const uint64_t sec)
{
  static_cast<Match*>(match)->StartTimer(sec);
}

static void StopTimer(void* match)
{
  static_cast<Match*>(match)->StopTimer();
}

static void LoadGame(HINSTANCE mod)
{
  if (!mod)
  {
    ErrorLog() << "Load mod failed";
    return;
  }

  typedef int (/*__cdecl*/ *Init)(const boardcast, const tell, const at, const game_over, const start_timer, const stop_timer);
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

  if (!init(&BoardcastPlayers, &TellPlayer, &AtPlayer, &MatchGameOver, &StartTimer, &StopTimer))
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
  g_game_handles.emplace(name, std::make_unique<GameHandle>(game_id, name, min_player, max_player, rule, new_game, delete_game, [mod] { FreeLibrary(mod); }));
  InfoLog() << "Loaded successfully!";
}

void LoadGameModules()
{
#ifdef _WIN32
  WIN32_FIND_DATA file_data;
  HANDLE file_handle = FindFirstFile(L".\\plugins\\*.dll", &file_data);
  if (file_handle == INVALID_HANDLE_VALUE) { return; }
  do
  {
    std::wstring dll_path = L".\\plugins\\" + std::wstring(file_data.cFileName);
    LoadGame(LoadLibrary(dll_path.c_str()));
  } while (FindNextFile(file_handle, &file_data));
  FindClose(file_handle);
  InfoLog() << "Load module count: " << g_game_handles.size();
#elif __linux__
  DIR *d = opendir("./plugins/");
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
    if (std::regex_match(name, base_match, base_regex); !base_match.empty())
    {
      DebugLog() << "Find irrelevant file " << name << ", skip";
      continue;
    }
    if (stat(dp->d_name, &st); !S_ISDIR(st.st_mode))
    {
      DebugLog() << "Find directory " << name << ", skip";
      continue;
    }
    InfoLog() << "Loading library " << name;
    LoadGame(dlopen((std::string("./plugins/") + name).c_str(), RTLD_LAZY));
  }
  closedir(d);
#endif
}

