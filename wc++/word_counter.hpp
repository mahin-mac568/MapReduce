/* Used Sagar's word-counting code */ 

#include <filesystem>
#include <map>
#include <string>

namespace fs = std::filesystem;

/*
THIS WILL BE AN N-GRAM COUNTER CLASS INSTEAD OF A WORD COUNTER FILE 
 - THE MAP MEMBER VARIABLE WILL HAVE N-GRAM TO INTEGER PAIRINGS
*/

namespace wc {

class wordCounter {
    // main storage structure for word frequencies
    std::map<std::string, uint64_t> freq;

    fs::path dir;
    uint32_t num_threads;

    void process_file(fs::path& file, std::map<std::string, uint64_t>& local_freq);

public:
    wordCounter(const std::string& dir, uint32_t num_threads);
    void compute();
    void display();
};

}  // namespace wc