#include "engine.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
int main(int argc,char** argv){try{if(argc!=7)throw std::runtime_error("input.rgba width height strength area output.rgba");std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("Cannot open input");std::vector<uint8_t>a((std::istreambuf_iterator<char>(f)),{});std::atomic<bool> cancel=false;auto out=Clean(a,std::stoi(argv[2]),std::stoi(argv[3]),{std::stoi(argv[4]),std::stoi(argv[5])},cancel);std::ofstream o(argv[6],std::ios::binary);o.write((const char*)out.data(),out.size());if(!o)throw std::runtime_error("Cannot write output");return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
