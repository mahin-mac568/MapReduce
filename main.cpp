#include <iostream>

#include "ngram_counter.hpp"

// this program computes word frequencies for all .h and .c files in the given directory and its subdirectories
int main(int argc, char* argv[]) {
  if (argc < 3) {
      std::cout << "Usage: " << argv[0] << " <dir> <num-threads> <ngram-length>" << std::endl;
      return 1;
  }

  std::string threads_inp = argv[2];                         
  uint32_t t = stoi(threads_inp.substr(threads_inp.find("=")+1));

  std::string ngram_inp = argv[3];                         
  uint32_t n = stoi(ngram_inp.substr(ngram_inp.find("=")+1));
  
  ngc::ngramCounter word_counter(argv[1], t, n);
  word_counter.compute(n);
}