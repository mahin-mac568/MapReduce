#!/bin/bash

# g++ version must be >=8 
g++ -O3 -std=c++17 wc++.cpp word_counter.cpp utils.cpp -lpthread -lstdc++fs -o wc++
