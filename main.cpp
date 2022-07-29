#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "ngram_counter.hpp"

// this program computes ngram frequencies for all .h and .c files in the given directory and its subdirectories
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


    /* DISPLAY */

    std::string display_header = "THE TOP FIVE N-GRAMS FOR THREAD "; 

    for (auto [thread_num, top_five] : word_counter.display) {
        std::cout << display_header << std::to_string(thread_num) << ":" << std::endl; 
        for (int i=0; i<top_five.size(); i++) {
            std::cout << " (" << i+1 << ") '" << top_five[i].first << "' with a count of " << top_five[i].second << std::endl; 
        }
        std::cout << "" << std::endl; 
    } 


    /* Web Page Display */

    std::ofstream results; 
    results.open("results.html"); 

    results << "<!DOCTYPE html>\n"; 
    results << "<html lang=\"en\">\n"; 
    results << "<head>\n"; 
    results << "<title>MapReduce</title>\n"; 
    results << "<meta charset=\"UTF-8\">\n"; 
    results << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"; 
    results << "<link rel=\"stylesheet\" href=\"https://www.w3schools.com/w3css/4/w3.css\">\n"; 
    results << "<link rel=\"stylesheet\" href=\"https://www.w3schools.com/lib/w3-theme-black.css\">\n"; 
    results << "<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Roboto\">\n"; 
    results << "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">\n"; 
    results << "<style>\n"; 
    results << "html,body,h1,h2,h3,h4,h5,h6 {font-family: \"Roboto\", sans-serif;}\n"; 
    results << ".w3-sidebar {\n"; 
    results << "  z-index: 3;\n"; 
    results << "  width: 250px;\n"; 
    results << "  top: 43px;\n"; 
    results << "  bottom: 0;\n"; 
    results << "  height: inherit;\n"; 
    results << "}\n"; 
    results << "</style>\n"; 
    results << "</head>\n"; 
    results << "<body>\n"; 

    results << "<div class=\"w3-top\">\n"; 
    results << "  <div class=\"w3-bar w3-theme w3-top w3-left-align w3-large\">\n"; 
    results << "    <a class=\"w3-bar-item w3-button w3-right w3-hide-large w3-hover-white w3-large w3-theme-l1\" href=\"javascript:void(0)\" onclick=\"w3_open()\"><i class=\"fa fa-bars\"></i></a>\n"; 
    results << "    <a href=\"app.html\" class=\"w3-bar-item w3-button w3-theme-l1\">MR</a>\n"; 
    results << "    <a href=\"#\" class=\"w3-bar-item w3-button w3-hide-small w3-hover-white\">About</a>\n"; 
    results << "    <a href=\"#\" class=\"w3-bar-item w3-button w3-hide-small w3-hover-white\">Design</a>\n"; 
    results << "    <a href=\"#\" class=\"w3-bar-item w3-button w3-hide-small w3-hover-white\">Contact</a>\n"; 
    results << "  </div>\n"; 
    results << "</div>\n"; 

    results << "<nav class=\"w3-sidebar w3-bar-block w3-collapse w3-large w3-theme-l5 w3-animate-left\" id=\"mySidebar\">\n"; 
    results << "  <a href=\"javascript:void(0)\" onclick=\"w3_close()\" class=\"w3-right w3-xlarge w3-padding-large w3-hover-black w3-hide-large\" title=\"Close Menu\">\n"; 
    results << "    <i class=\"fa fa-remove\"></i>\n"; 
    results << "  </a>\n"; 
    results << "  <h4 class=\"w3-bar-item\"><b>Try it out!</b></h4>\n"; 
    results << "  <a class=\"w3-bar-item w3-button w3-hover-black\" href=\"#\">See Results</a>\n"; 
    results << "</nav>\n"; 

    results << "<div class=\"w3-overlay w3-hide-large\" onclick=\"w3_close()\" style=\"cursor:pointer\" title=\"close side menu\" id=\"myOverlay\"></div>\n"; 

    results << "<div class=\"w3-main\" style=\"margin-left:250px\">\n"; 

    for (auto [thread_num, top_five] : word_counter.display) {
        results << "  <div class=\"w3-row w3-padding-64\">\n"; 
        results << "    <div class=\"w3-twothird w3-container\">\n"; 
        results << "      <h1 class=\"w3-text-teal\">" << display_header << std::to_string(thread_num) << ":</h1>\n"; 
        results << "      <ol>\n"; 

        for (int i=0; i<top_five.size(); i++) {
            results << "        <li>'" << top_five[i].first << "' with a count of " << top_five[i].second << "</li>\n";
        }

        results << "      </ol>\n"; 
        results << "    </div>\n"; 
        results << "  </div>\n"; 
        results << "\n"; 
    }

    results << "  <div class=\"w3-row w3-padding-64\">\n"; 
    results << "    <div class=\"w3-twothird w3-container\">\n"; 
    results << "    </div>\n"; 
    results << "  </div>\n"; 

    results << "  <footer id=\"myFooter\">\n"; 
    results << "    <div class=\"w3-container w3-theme-l2 w3-padding-32\">\n"; 
    results << "      <h4></h4>\n"; 
    results << "    </div>\n"; 

    results << "    <div class=\"w3-container w3-theme-l1\">\n"; 
    results << "    </div>\n"; 
    results << "  </footer>\n"; 

    results << "</div>\n"; 


    results << "<script>\n"; 

    results << "var mySidebar = document.getElementById(\"mySidebar\");\n"; 
    results << "var overlayBg = document.getElementById(\"myOverlay\");\n"; 

    results << "function w3_open() {\n"; 
    results << "  if (mySidebar.style.display === 'block') {\n"; 
    results << "    mySidebar.style.display = 'none';\n"; 
    results << "    overlayBg.style.display = \"none\";\n"; 
    results << "  } else {\n"; 
    results << "      mySidebar.style.display = 'block';\n"; 
    results << "      overlayBg.style.display = \"block\";\n"; 
    results << "  }\n"; 
    results << "}\n"; 

    results << "function w3_close() {\n"; 
    results << "  mySidebar.style.display = \"none\";\n"; 
    results << "  overlayBg.style.display = \"none\";\n"; 
    results << "}\n"; 

    results << "</script>\n"; 

    results << "</body>\n"; 
    results << "</html>\n"; 

    results.close(); 
}