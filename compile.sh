#!/bin/bash

# g++ version must be >=8 
# g++ -O3 -std=c++17 main.cpp ngram_counter.cpp utils.cpp -lpthread -lstdc++fs -o main

clang++ -O3 -std=c++17 main.cpp ngram_counter.cpp utils.cpp -lpthread -stdlib=libc++ -o main