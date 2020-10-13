
#include <postalt/postalt.h>

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace postalt;

Postalt::Postalt(std::string alt_filename) : 
  m_alt_filename(alt_filename),
  m_block_lines(10*1000)
  {

  }

bool Postalt::run() const 
{
  std::cout << "Running with alt file: " << m_alt_filename << std::endl;

  
  return true;
}

void Postalt::test_io() const
{
  std::string line;
  // while (std::cin >> line)
  // while (std::getline(std::cin, line))
  {
    //std::cout<<line<<std::endl;
  }

  //FILE* f = fopen(stdin);
  // char* buff = (char*)malloc(1024);
  constexpr int size = 1024;
  char buff[size];
  while (NULL != fgets(buff, size, stdin))
  {
    std::cout<<buff;
    // printf("%s", buff);
  }
  // free(buff);
  // fclose(f);

  return;
}