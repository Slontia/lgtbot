#pragma once

#include <cmath>
#include <string>

extern "C" {
#include "tinyexpr.h"
}

std::pair<int, int> get_a_b(const std::string& guess, const std::string& target) {
  assert(guess.length() == target.length());
  char cnt[128] = {0};
  int a = 0, b = 0, len = target.length();
  for (int i = 0; i < len; i++) {
    if (guess[i] == target[i])
      a++;
    else
      cnt[target[i]]++;
  }
  for (int i = 0; i < len; i++) {
    if (guess[i] != target[i] && cnt[guess[i]]) {
      cnt[guess[i]]--;
      b++;
    }
  }
  return {a, b};
}

double evaluate(const std::string& formula) { return te_interp(formula.c_str(), 0); }

bool check_equation(const std::string& formula, std::string& error, bool standard) {
  int n = formula.length();
  int eq_pos = -1;
  for (int i = 0; i < n; i++) {
    char c = formula[i];
    if (c == '=') {
      if (eq_pos != -1) {
        error = "等号不能超过一个";
        return false;
      }
      eq_pos = i;
    } else if (!(c >= '0' && c <= '9' || c == '+' || c == '-' || c == '*' || c == '/')) {
      error = "不支持 " + std::string(1, c) + " 运算，长大后再学习";
      return false;
    }
  }
  if (eq_pos == -1) {
    error = "没有找到等号，请提交一个等式";
    return false;
  }
  if (eq_pos == 0 || eq_pos == n - 1) {
    error = "等号不能在开头或结尾";
    return false;
  }
  for (int i = 0; i < n - 1; i++) {
    if ((formula[i] == '+' || formula[i] == '-') &&
        (formula[i + 1] == '+' || formula[i + 1] == '-')) {
      error = "符号 " + std::string(1, formula[i]) + formula[i + 1] + " 的连续使用是不被允许的。";
      return false;
    }
  }
  std::string left = formula.substr(0, eq_pos), right = formula.substr(eq_pos + 1, n - eq_pos - 1);
  if (standard) {
    if (left[0] == '-' || right[0] == '-') {
      error = "在标准模式中，减号不能当作负号使用";
      return false;
    }
    if (left[0] == '+' || right[0] == '+') {
      error = "在标准模式中，加号不能当作正号使用";
      return false;
    }
    bool left_has_operator = false;
    for (char c : left)
      if (c > '9' || c < '0') left_has_operator = true;
    if (!left_has_operator) {
      error = "在标准模式中，等式左侧不能为纯数字";
      return false;
    }
    for (char c : right)
      if (c > '9' || c < '0') {
        error = "在标准模式中，等式右侧只能为纯数字";
        return false;
      }
    for (int i = 0; i < n; i++) {
      if (formula[i] == '0' && (i == 0 || formula[i - 1] > '9' || formula[i - 1] < '0') &&
          !(i == n - 1 || formula[i + 1] > '9' || formula[i + 1] < '0')) {
        error = "在标准模式中，数字不能以0开头";
        return false;
      }
    }
  }
  double val_left = evaluate(left), val_right = evaluate(right);
  if (std::isnan(val_left)) {
    error = "等式左边无法计算。";
    return false;
  }
  if (std::isnan(val_right)) {
    error = "等式右边无法计算。";
    return false;
  }
  if (fabs(val_left - val_right) > 1e-6) {
    error = "等式不成立。";
    return false;
  }
  return true;
}