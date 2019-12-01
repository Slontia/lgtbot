#pragma once

#include <sstream>

#include "lgtbot.h"
#include "message_iterator.h"
#include "game_container.h"
#include "match.h"

void show_gamelist(MessageIterator& msg);
void start_game(MessageIterator& msg);
void new_game(MessageIterator& msg);
void leave(MessageIterator& msg);
void join(MessageIterator& msg);