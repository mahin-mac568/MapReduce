#include "ngram_counter.hpp"

#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <sstream> 
#include <mutex>
#include <regex>
#include <thread>
#include <vector>
#include <future>

#include "utils.hpp"


// Character conversion functions 
char compress_newline(char); 
char insert_punct_break(char); 
char keep_alpha_or_space(char);
std::string process_text(std::string); 

// MapReduce functions 


ngc::ngramCounter::ngramCounter(const std::string& dir, uint32_t num_threads, uint32_t ngram_length)
  : dir(dir), num_threads(num_threads), ngram_length(ngram_length) {}

void ngc::ngramCounter::compute(uint32_t ngram_length) {
    std::mutex wc_mtx;

    std::vector<fs::path> files_to_sweep = utils::find_all_files(dir, [](const std::string& extension) {
        return extension == ".txt";
    });

    std::atomic<uint64_t> global_index = 0;

    // Create tables of promises and futures for message passing 
    using awaiting_ngrams = std::vector<std::pair<std::string,uint64_t>>; 
    std::vector<std::vector<std::promise<awaiting_ngrams>>> promises; 
    std::vector<std::vector<std::future<awaiting_ngrams>>> futures; 

    // Initialize the promise objects and inner vectors 
    for (int i=0; i<num_threads; i++) {
        std::vector<std::promise<awaiting_ngrams>> promise_vector; 
        for (int j=0; j<num_threads; j++) {
            std::promise<awaiting_ngrams> promise; 
            promise_vector.push_back(std::move(promise)); 
        }
        promises.push_back(std::move(promise_vector)); 
    } 

    // Initialize the future objects and inner vectors 
    for (int i=0; i<num_threads; i++) {
        std::vector<std::future<awaiting_ngrams>> future_vector; 
        for (int j=0; j<num_threads; j++) {
            std::future future = promises[j][i].get_future(); 
            future_vector.push_back(std::move(future)); 
        }   
        futures.push_back(std::move(future_vector)); 
    }

    auto sweep = [this, &files_to_sweep, &global_index, &wc_mtx, &ngram_length, 
                  &promises, &futures] (int curr_thread) 
    {
        // Declare a hash function to be used by the local frequency map
        std::hash<std::string> hash_function{};

        // a local freq data structure for each worker
        std::map<size_t,uint64_t> local_freq;  

        // a local data structure that maps hash keys to corresponding ngrams
        std::map<size_t,std::string> local_ngrams; 
        
        // the final ngram map for this thread 
        using pair_t = std::pair<std::string,uint64_t>;
        std::vector<pair_t> sorted_reduced_collection; 

        uint64_t file_index;
        while((file_index = global_index++) < files_to_sweep.size()) {
            // This code was taken from the former process_file function 
            std::ifstream fin(files_to_sweep[file_index]);
            std::stringstream buffer;
            buffer << fin.rdbuf();
            std::string contents = buffer.str();

            // Convert uppercase to lowercase, convert newlines to spaces, and turn numbers and punctuation to '|'
            std::transform(contents.begin(), contents.end(), contents.begin(), 
            [](unsigned char c){ return std::tolower(c); });
            std::transform(contents.begin(), contents.end(), contents.begin(), 
            [](unsigned char c){ return compress_newline(c); });
            std::transform(contents.begin(), contents.end(), contents.begin(), 
            [](unsigned char c){ return insert_punct_break(c); });
            std::transform(contents.begin(), contents.end(), contents.begin(), 
            [](unsigned char c){ return keep_alpha_or_space(c); });

            // Initialize a list to hold the longest possible n-grams 
            std::vector<std::string> max_ngrams; 

            // Create longest possible n-grams out of the contents string using the '|' breaks 
            std::string contents_copy = contents; 
            int idx = 0; 
            while (contents_copy.find("|") != std::string::npos) {
                if (contents_copy[idx] == '|') {
                    int pos = contents_copy.find("|"); 
                    std::string an_ngram = contents_copy.substr(0, pos);
                    std::string everything_else = contents_copy.substr(pos + 1);
                    
                    if (an_ngram[0] == ' ') {
                        while (an_ngram[0] == ' ') {
                            an_ngram = an_ngram.substr(1);
                        }
                    }
                    bool populated = false; 
                    for (int i=0; i<an_ngram.size(); i++) {
                        if (isalpha(an_ngram[i])) {
                            populated = true; 
                            break; 
                        }
                    }
                    if (populated) {
                        max_ngrams.push_back(an_ngram); 
                    }
                    contents_copy = everything_else; 
                    idx = 0; 
                }
                else {
                    idx++; 
                }
            }
            
            // Create a table of word lists for each max n-gram 
            std::vector<std::vector<std::string>> max_ngram_table; 
            for (std::string ngram : max_ngrams) {
                std::istringstream ss(ngram);
                std::string word; 
                std::vector<std::string> word_list; 
                while (ss >> word) {
                    word_list.push_back(word); 
                }
                max_ngram_table.push_back(word_list);
            }
            // inspiration from https://www.geeksforgeeks.org/split-a-sentence-into-words-in-cpp/

            // Now compute the n-grams from the table of max n-gram word lists 
            // And count those n-grams by storing them in a local_freq map 
            for (int k=0; k<max_ngram_table.size(); k++) {
                std::vector<std::string> word_list = max_ngram_table[k]; 
                if (word_list.size() < ngram_length) {
                    continue; 
                }
                else if (word_list.size() == ngram_length) {
                    std::string ngram = max_ngrams[k];
                    auto hash_key = hash_function(ngram);
                    local_ngrams[hash_key] = ngram; 
                    if (local_freq.find(hash_key) != local_freq.end()) {
                        local_freq[hash_key] += 1; 
                    }
                    else {
                        local_freq[hash_key] = 1; 
                    }
                }
                else {
                    int idx = 0; 
                    while ((word_list.size()-idx) >= ngram_length) {
                        std::string ngram = ""; 
                        for (int i=0; i<ngram_length; i++) {
                            std::string word = word_list[i]; 
                            ngram = ngram + word + " "; 
                        }
                        if (ngram.back() == ' ') {  // get rid of trailing space 
                            while (ngram.back() == ' ') {
                                ngram.pop_back(); 
                            }
                        }
                        auto hash_key = hash_function(ngram);
                        local_ngrams[hash_key] = ngram; 
                        if (local_freq.find(hash_key) != local_freq.end()) {
                            local_freq[hash_key] += 1; 
                        }
                        else {
                            local_freq[hash_key] = 1; 
                        }
                        idx++; 
                    }
                }
            }
        }

        // Create a data structure with subsets of n-grams that belong to other workers 
        std::map<int,std::vector<std::pair<std::string,uint64_t>>> destination_data;     // (<thread-#>: [(n-grams,count)])

        // Hash the ngrams to figure out which workers own each one 
        auto hash_ngram_pair = local_freq.begin(); 
        while (hash_ngram_pair != local_freq.end()) {
            auto hash_key = hash_ngram_pair->first; 
            std::string ngram = local_ngrams[hash_key];
            int dest_thread = hash_key % num_threads; 
            uint64_t count = local_freq[hash_key]; 
            if (destination_data.find(dest_thread) != destination_data.end()) {
                destination_data[dest_thread].push_back(std::make_pair(ngram, count)); 
            }
            else {
                std::vector<std::pair<std::string,uint64_t>> pairs; 
                pairs.push_back(std::make_pair(ngram, count)); 
                destination_data[dest_thread] = pairs; 
            }
            hash_ngram_pair++; 
        }

        // Using the destination data, set the values in the promise table 
        auto owner_counts_pair = destination_data.begin(); 
        while (owner_counts_pair != destination_data.end()) {
            int dest_thread = owner_counts_pair->first;
            promises[curr_thread][dest_thread].set_value(owner_counts_pair->second); 
            owner_counts_pair++; 
        }

        // Retrieve the values from the futures table and store the values 
        using sub_collection = std::vector<std::pair<std::string,uint64_t>>; 
        std::vector<sub_collection> sub_collections; 
        for (int i=0; i<num_threads; i++) {
            sub_collections.push_back(futures[curr_thread][i].get()); 
        }    

        // Right now, the futures table is holding all the owner threads' rightful ngrams 
        // Each cell is holding [ (ngram1, c1), (ngram1, c2), (ngram2, c3), ... ]. 

        // The sub collection storage is a vector OF THESE DATA STRUCTURES ^ 
        // We need to flatten that into one big collection  
        std::vector<std::pair<std::string,uint64_t>> big_collection; 
        for (int i=0; i<sub_collections.size(); i++) {
            for (std::pair<std::string,uint64_t> pair : sub_collections[i]) {
                big_collection.push_back(pair); 
            }
        }

        // Need to get to this form: [ ("n1": [c1,c2,...]), (), ... ]
        std::map<std::string,std::vector<uint64_t>> groups_to_reduce; 
        for (std::pair<std::string,uint64_t> pair : big_collection) {
            std::string ngram = pair.first; 
            if (groups_to_reduce.find(ngram) != groups_to_reduce.end()) {
                groups_to_reduce[ngram].push_back(pair.second); 
            }
            else {
                std::vector<uint64_t> counts; 
                counts.push_back(pair.second); 
                groups_to_reduce[ngram] = counts; 
            }
        }

        // Reduce the counts by summing them to one value and store in new map 
        std::map<std::string,uint64_t> reduced_collection;
        auto ngram_counts_pair = groups_to_reduce.begin(); 
        while (ngram_counts_pair != groups_to_reduce.end()) {
            uint64_t sum = 0; 
            for (uint64_t count : (ngram_counts_pair->second)) {
                sum += count; 
            }
            reduced_collection[ngram_counts_pair->first] = sum; 
            ngram_counts_pair++; 
        }

        // Sort the reduced collection, display this thread's top five 
        uint32_t index = 0;
        for(auto& [ngram, cnt] : reduced_collection) {
            sorted_reduced_collection.emplace_back(std::make_pair(ngram, cnt));
            index++; 
        }
        std::sort(sorted_reduced_collection.begin(), sorted_reduced_collection.end(), 
                  [](const pair_t& p1, const pair_t& p2) {
            return p1.second > p2.second || (p1.second == p2.second && p1.first < p2.first);
        });

        // update this->freq and exit
        std::lock_guard<std::mutex> lock(wc_mtx);

        int count = 0; 
        const int display_length = 5; 
        awaiting_ngrams top_five; 
        
        for(auto [ngram, cnt] : sorted_reduced_collection) {
            freq[ngram] += cnt;
            if (count < display_length) {
                top_five.push_back(std::make_pair(ngram, cnt)); 
                count++; 
            } 
        }

        display[curr_thread+1] = top_five;      // +1, so the thread # is non-zero
    };

    // start all threads and wait for them to finish
    std::vector<std::thread> workers; 
    for(uint32_t i = 0; i < num_threads; ++i) {
        workers.push_back(std::thread(sweep, i));
    }
    for(auto& worker : workers) {
        worker.join();
    }
}

    
/* FILTER FUNCTIONS */ 

char compress_newline(char c) {
    return (c == '\n' || c == '\t') ? ' ' : c; 
}

char insert_punct_break(char c) {
    return (isdigit(c) || ispunct(c)) ? '|' : c; 
}

char keep_alpha_or_space(char c) {
    return (isalpha(c) || c == ' ') ? c : '|'; 
}

std::string process_text(std::string contents) {
    std::transform(contents.begin(), contents.end(), contents.begin(), 
    [](unsigned char c){ return std::tolower(c); });
    std::transform(contents.begin(), contents.end(), contents.begin(), 
    [](unsigned char c){ return compress_newline(c); });
    std::transform(contents.begin(), contents.end(), contents.begin(), 
    [](unsigned char c){ return insert_punct_break(c); });
    std::transform(contents.begin(), contents.end(), contents.begin(), 
    [](unsigned char c){ return keep_alpha_or_space(c); });

    return contents; 
} 