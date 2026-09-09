#pragma once
#include "engine.h"
#include <algorithm>
#include <cstring>
// Only UI-thread code owns the cache. Worker results are merged after joining.
class PreviewCache {
 int w,h,columns;Settings current{-1,-1};std::vector<uint8_t> completed;
public:
 std::vector<uint8_t> pixels;
 PreviewCache(int width,int height):w(width),h(height),columns((w+1023)/1024),completed(columns*((h+1023)/1024)),pixels(size_t(w)*h*4){}
 void Configure(Settings p){if(p.strength!=current.strength||p.area!=current.area){current=p;std::fill(completed.begin(),completed.end(),0);}}
 std::vector<int> Missing(ImageRect b)const{std::vector<int> ids;int id=0;for(int y=0;y<h;y+=1024)for(int x=0;x<w;x+=1024,id++)if(x<b.right&&y<b.bottom&&x+1024>b.left&&y+1024>b.top&&!completed[id])ids.push_back(id);return ids;}
 void Merge(const std::vector<TilePatch>& tiles){for(const auto& tile:tiles){auto b=tile.bounds;for(int y=b.top;y<b.bottom;y++)memcpy(pixels.data()+(size_t(y)*w+b.left)*4,tile.pixels.data()+size_t(y-b.top)*(b.right-b.left)*4,size_t(b.right-b.left)*4);completed.at(tile.tileId)=1;}}
};
