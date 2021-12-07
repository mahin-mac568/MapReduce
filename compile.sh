#!/bin/bash

# g++ version must be >=8 
g++ -O3 -std=c++17 hw4.cpp ngram_counter.cpp utils.cpp -lpthread -lstdc++fs -o hw4
