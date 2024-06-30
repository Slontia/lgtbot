// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <functional>

class EmptyFunc
{
 private:
  template <typename Ret, typename ...Args>
  using FuncPointer = Ret(*)(Args...);

 public:
  template <typename Ret, typename ...Args>
  operator FuncPointer<Ret, Args...>() const { return [](Args...) { return Ret(); }; }

  template <typename Ret, typename ...Args>
  operator std::function<Ret(Args...)>() const { return static_cast<FuncPointer<Ret, Args...>>(*this); }
};

inline constexpr EmptyFunc g_empty_func;

static_assert(requires (void(*a)(int), EmptyFunc func) { a = func; });
static_assert(requires (double(*a)(int, EmptyFunc), EmptyFunc func) { a = func; });
static_assert(requires (std::function<void(double)> a, EmptyFunc func) { a = func; });
