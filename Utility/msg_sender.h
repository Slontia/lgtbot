#pragma once
#include <vector>
#include <memory>

struct AtMsg { const uint64_t id_; };

class MsgSender
{
 public:
  virtual ~MsgSender() {}
  virtual void SendString(const char* const str, const size_t len) = 0;
  virtual void SendAt(const uint64_t id_) = 0;
};

class MsgSenderWrapper
{
 private:
  using MsgSenderPtr = std::unique_ptr<MsgSender, void (*)(MsgSender* const)>;
 public:
  MsgSenderWrapper(MsgSenderPtr&& msg_sender) : msg_sender_(std::move(msg_sender)) {}
  MsgSenderWrapper(const MsgSender&) = delete;
  MsgSenderWrapper(MsgSenderWrapper&&) = default;

  MsgSenderWrapper& operator<<(const std::string_view s)
  {
    msg_sender_->SendString(s.data(), s.size());
    return *this;
  }

  template <typename Arith> requires std::is_arithmetic_v<Arith>
  MsgSenderWrapper& operator<<(const Arith arith)
  {
    const std::string s(std::to_string(arith));
    msg_sender_->SendString(s.data(), s.size());
    return *this;
  }

  MsgSenderWrapper& operator<<(const AtMsg& at_msg)
  {
    msg_sender_->SendAt(at_msg.id_);
    return *this;
  }

  void Over() { msg_sender_ = nullptr; }

 private:
  std::unique_ptr<MsgSender, void (*)(MsgSender* const)> msg_sender_;
};

class MsgSenderWrapperBatch
{
 public:
  MsgSenderWrapperBatch(std::vector<MsgSenderWrapper>&& senders) : senders_(std::move(senders)) {}
  MsgSenderWrapperBatch(const MsgSenderWrapperBatch&) = delete;
  MsgSenderWrapperBatch(MsgSenderWrapperBatch&&) = default;

  template <typename Arg>
  MsgSenderWrapperBatch& operator<<(Arg&& arg)
  {
    for (auto& sender : senders_) { sender << std::forward<Arg>(arg); }
    return *this;
  }

  void Over() { senders_.clear(); }

 private:
  std::vector<MsgSenderWrapper> senders_;
};
