#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "crawler/util/text_handler.h"

int main(int argc, char **argv) {
  const char *fpr = "/home/pengdan/workspace/data/read.txt";
  const char *fpw = "/home/pengdan/workspace/data/write.txt";
  std::string source;
  std::string dest;
  if (crawler::TextHandler::CescapeWholeFile(fpr, &source)) {
    std::cout << source << std::endl;
  }
  dest = source;
  if (crawler::TextHandler::UnCnescapeWholeFile(dest, fpw)) {
    std::cout << "ok2";
  }
  return 0;
}
