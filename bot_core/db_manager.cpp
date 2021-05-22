#ifdef WITH_MYSQL

#include "db_manager.h"

std::unique_ptr<DBManager> DBManager::g_db_manager_ = nullptr;
thread_local std::string DBManager::errmsg_;

#endif
