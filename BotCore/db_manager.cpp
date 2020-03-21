#include "stdafx.h"
#include "db_manager.h"

std::unique_ptr<DBManager> DBManager::g_db_manager_ = nullptr;
