#include "preview-cache.h"
#include <iostream>
#include <chrono>
#include <stdexcept>
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(){try{
 int w=3200,h=2100;std::vector<uint8_t> a(size_t(w)*h*4);for(size_t i=0;i<a.size();i+=4){a[i]=170;a[i+1]=135;a[i+2]=100;a[i+3]=255;}
 for(int y=30;y<h;y+=1024)for(int x=30;x<w;x+=1024)for(int c=0;c<3;c++)a[(size_t(y)*w+x)*4+c]=7;
 std::atomic<bool> cancel=false;Settings p{60,60};PreviewCache cache(w,h);cache.Configure(p);ImageRect visible{10,10,400,300};auto missing=cache.Missing(visible);require(missing==std::vector<int>{0},"preview must request one tile, not full image");
 auto start=std::chrono::steady_clock::now();auto first=CleanTiles(a,w,h,p,cancel,missing);double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();cache.Merge(first);require(cache.Missing(visible).empty(),"unchanged viewport should reuse cache");require(cache.pixels[(30*w+30)*4+3]==255,"visible dot not repaired");require(cache.pixels[(30*w+1054)*4+3]==0,"offscreen tile was computed");
 auto rest=cache.Missing({0,0,w,h});require(rest.size()==11,"apply must reuse one of twelve tiles");cache.Merge(CleanTiles(a,w,h,p,cancel,rest));auto whole=Clean(a,w,h,p,cancel);require(cache.pixels==whole,"preview plus remaining tiles must equal full render");
 cache.Configure({60,1});require(cache.Missing(visible)==std::vector<int>{0},"parameter change must invalidate cache");
 bool stopped=false;try{CleanTiles(a,w,h,p,cancel,{0,1},[&](int){cancel=true;});}catch(const std::exception&){stopped=true;}require(stopped,"cancellation between tiles ignored");
 std::cout<<"PASS 3200x2100: preview 1/12 tiles, reuse cache, apply remaining 11, exact full-render equality, parameter invalidation, cancellation; preview_seconds="<<seconds<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
