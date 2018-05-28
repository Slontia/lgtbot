#include "stdafx.h"

Game::Game(Match& match) : match_(match) {}

inline bool Game::valid_pnum(uint32_t pnum)
{
  return pnum >= kMinPlayer && (kMaxPlayer == 0 || pnum <= kMaxPlayer);
}

void Game::set_state_container(std::shared_ptr<StateContainer> state_container)
{
  state_container_ = state_container;
}

void Game::Reply(uint32_t pid, std::string msg) const
{
  match_.Reply(pid, msg);
}

void Game::Broadcast(std::string msg) const
{
  match_.Broadcast(msg);
}

void Game::Join(std::shared_ptr<GamePlayer> player)
{
  players_.push_back(player);
}

bool Game::StartGame()
{
  timer_.clear_stack();
  timer_.push_handle_to_stack(std::bind(&Game::RecordResult, this));
  main_state_ = state_container_->MakeMain();
  main_state_->Start();
}

void Game::Request(uint32_t pid, const char* msg, int32_t sub_type)
{
  main_state_->Request(pid, msg, sub_type);
}

//std::unique_ptr<sql::Statement> state(sql::mysql::get_mysql_driver_instance()
//                                      ->connect("tcp://127.0.0.1:3306", "root", "")
//                                      ->createStatement());
