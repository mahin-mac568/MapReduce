#include <filesystem>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <future>

namespace fs = std::filesystem;
using awaiting_ngrams = std::vector<std::pair<std::string,uint64_t>>; 

namespace ngc {

class ngramCounter {
    public:
    std::map<std::string, uint64_t> freq;                  // global frequency 
    std::map<uint64_t, awaiting_ngrams> display;           // global display 
    fs::path dir;
    uint32_t num_threads;
    uint32_t ngram_length; 

    ngramCounter(const std::string& dir, uint32_t num_threads, uint32_t ngram_length);
    void compute(uint32_t);
};

} 