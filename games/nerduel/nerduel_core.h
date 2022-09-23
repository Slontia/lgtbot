#pragma once

#include "tinyeval.h"

double evaluate(std::string str) {
  exprtk::expression<double> expression;
  exprtk::parser<double> parser;
  parser.compile(str, expression);
  return expression.value();
}