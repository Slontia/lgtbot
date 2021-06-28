#pragma once

#include <vector>

class Masker
{
  public:
    Masker(const size_t size) : bitset_(size, false), count_(0) {}
    bool Set(const size_t index)
    {
        const bool old_value = bitset_[index];
        bitset_[index] = true;
        if (old_value == false) {
            return bitset_.size() == ++count_;
        }
        return false;
    }
    void Unset(const size_t index)
    {
        const bool old_value = bitset_[index];
        bitset_[index] = false;
        if (old_value == true) {
            --count_;
        }
    }

  private:
    std::vector<bool> bitset_;
    size_t count_;
};
