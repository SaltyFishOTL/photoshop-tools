#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>
struct Settings { int strength=60; int area=60; };
struct ImageRect {int left=0,top=0,right=0,bottom=0;};
struct TilePatch {ImageRect bounds;std::vector<uint8_t> pixels;int tileId;};
std::vector<TilePatch> CleanTiles(const std::vector<uint8_t>& rgba,int w,int h,Settings settings,std::atomic<bool>& cancel,const std::vector<int>& tileIds,std::function<void(int)> progress={});
std::vector<uint8_t> CleanRegion(const std::vector<uint8_t>& rgba,int w,int h,Settings settings,std::atomic<bool>& cancel,ImageRect region,std::function<void(int)> progress={});
std::vector<uint8_t> Clean(const std::vector<uint8_t>& rgba,int w,int h,Settings settings,std::atomic<bool>& cancel,std::function<void(int)> progress={});
