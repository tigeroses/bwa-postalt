/*
 * File: postalt.cpp
 * Created Data: 2020-9-25
 * Author: fxzhao
 * Contact: <zhaofuxiang@genomics.cn>
 *
 * Copyright (c) 2020 BGI
 */

#include <postalt/postalt.h>

#include <iostream>

using namespace postalt;

Postalt::Postalt(std::string _alt_filename) : alt_filename(_alt_filename) {}

bool Postalt::run() const {
  std::cout << "Running with alt file: " << alt_filename << std::endl;

  return true;
}
