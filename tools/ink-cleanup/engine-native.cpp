#include "engine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
namespace {
using Color=std::array<double,3>;
struct Group {std::vector<int> points;bool solid=false;Color background{};};
int Sign(int x){return (x>0)-(x<0);}
void Cancel(const std::atomic<bool>& cancel){if(cancel.load(std::memory_order_relaxed))throw std::runtime_error("Cancelled");}
double Difference(const uint8_t* a,const Color& b){return std::max({std::abs(a[0]-b[0]),std::abs(a[1]-b[1]),std::abs(a[2]-b[2])});}
double Difference(const Color& a,const Color& b){return std::max({std::abs(a[0]-b[0]),std::abs(a[1]-b[1]),std::abs(a[2]-b[2])});}
Color Pixel(const uint8_t* a){return {double(a[0]),double(a[1]),double(a[2])};}
std::vector<uint8_t> Tile(const std::vector<uint8_t>& a,int w,int h,Settings p,const std::array<bool,4>& edges,std::atomic<bool>& cancel){
 const int n=w*h;std::vector<uint8_t> dark(n),seed(n),uncertain(n),seen(n),mask(n);std::vector<int> queue;queue.reserve(n);std::vector<Group> groups;
 const int radii[]={3,5,8};const int dirs[][2]={{1,0},{0,1},{1,1},{1,-1}};const double threshold=std::max(2.0,16-.2*p.strength);
 struct Candidate{Color color;int scale;};
 for(int y=0;y<h;y++){Cancel(cancel);for(int x=0;x<w;x++){
  int i=y*w+x;const uint8_t* pixel=a.data()+i*4;if(pixel[3]<250)continue;
  Candidate candidates[12];int count=0;bool same=true;
  for(int scale=0;scale<3;scale++)for(auto& dir:dirs){int dx=dir[0]*radii[scale],dy=dir[1]*radii[scale],ax=x+dx,ay=y+dy,bx=x-dx,by=y-dy;if(ax<0||ay<0||bx<0||by<0||ax>=w||bx>=w||ay>=h||by>=h)continue;auto ap=a.data()+(ay*w+ax)*4;auto bp=a.data()+(by*w+bx)*4;if(ap[3]<250||bp[3]<250)continue;
   int delta=std::max({std::abs(int(ap[0])-bp[0]),std::abs(int(ap[1])-bp[1]),std::abs(int(ap[2])-bp[2])});if(delta<=20){Color color{(ap[0]+bp[0])/2.0,(ap[1]+bp[1])/2.0,(ap[2]+bp[2])/2.0};candidates[count++]={color,scale};if(Difference(pixel,color)!=0)same=false;}}
  // Exact shortcut: every candidate explains the original color, with enough evidence.
  if(count>=2&&same)continue;
  int bestVotes=0,bestScales=0;double bestDiff=std::numeric_limits<double>::infinity();
  for(int c=0;c<count;c++){int votes=0,scales=0;Color sum{};for(int j=0;j<count;j++)if(Difference(candidates[c].color,candidates[j].color)<=12){votes++;for(int k=0;k<3;k++)sum[k]+=candidates[j].color[k];scales|=1<<candidates[j].scale;}
   Color mean{sum[0]/votes,sum[1]/votes,sum[2]/votes};double diff=Difference(pixel,mean);if(votes>bestVotes||(votes==bestVotes&&diff<bestDiff)){bestVotes=votes;bestDiff=diff;bestScales=scales;}}
  if(count&&bestDiff>=2)dark[i]=1;
  if(bestVotes>=3&&(bestScales&(bestScales-1))!=0&&bestDiff>=threshold)seed[i]=1;
  if(bestVotes<2){int range=0;for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){int nx=x+dx,ny=y+dy;if(nx<0||ny<0||nx>=w||ny>=h)continue;auto j=a.data()+(ny*w+nx)*4;for(int c=0;c<3;c++)range=std::max(range,std::abs(int(pixel[c])-j[c]));}if(range>=4)uncertain[i]=1;}
 }}
 for(int start=0;start<n;start++){if((start&4095)==0)Cancel(cancel);if(!dark[start]||seen[start])continue;queue.clear();queue.push_back(start);seen[start]=1;bool eligible=true,hasSeed=false;
  for(size_t head=0;head<queue.size();head++){if((head&4095)==0)Cancel(cancel);int i=queue[head],x=i%w,y=i/w;hasSeed=hasSeed||seed[i];if((x<2&&edges[0])||(y<2&&edges[1])||(x>=w-2&&edges[2])||(y>=h-2&&edges[3]))eligible=false;
   for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++){int nx=x+dx,ny=y+dy;if(nx<0||ny<0||nx>=w||ny>=h)continue;int j=ny*w+nx;if(!dark[j]||seen[j])continue;if(std::abs(dx)>1||std::abs(dy)>1){int ux=Sign(dx),uy=Sign(dy),bx=x-ux,by=y-uy,fx=nx+ux,fy=ny+uy;if(bx<0||by<0||fx<0||fy<0||bx>=w||fx>=w||by>=h||fy>=h)continue;if(!dark[by*w+bx]||!dark[fy*w+fx])continue;}seen[j]=1;queue.push_back(j);}}
  if(!eligible||!hasSeed||queue.size()>size_t(p.area))continue;Group g;g.points=queue;for(int i:queue)mask[i]=255;groups.push_back(std::move(g));
 }
 for(int i=0;i<n;i++)if(uncertain[i])dark[i]=1;
 std::vector<uint32_t> visited(n),membership(n);uint32_t stamp=0;const int islandRadii[]={2,3,5,8};const int ringDirs[][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{-1,1},{1,-1},{-1,-1}};
 for(int y=8;y<h-8;y++){Cancel(cancel);for(int x=8;x<w-8;x++){int start=y*w+x;if(!dark[start]||mask[start])continue;
  for(int radius:islandRadii){int ring[8],support[8],supportCount=0;for(int j=0;j<8;j++)ring[j]=(y+ringDirs[j][1]*radius)*w+x+ringDirs[j][0]*radius;
   for(int j:ring){Color color=Pixel(a.data()+j*4);int ids[8],count=0;for(int k:ring)if(a[k*4+3]>=250&&Difference(a.data()+k*4,color)<=3)ids[count++]=k;if(count>supportCount){supportCount=count;std::copy(ids,ids+count,support);}}
   if(supportCount<6)continue;Color bg;for(int c=0;c<3;c++){uint8_t values[8];for(int j=0;j<supportCount;j++)values[j]=a[support[j]*4+c];std::sort(values,values+supportCount);bg[c]=values[supportCount/2];}
   if(Difference(a.data()+start*4,bg)<threshold)continue;
   if(++stamp==0){std::fill(visited.begin(),visited.end(),0);std::fill(membership.begin(),membership.end(),0);stamp=1;}
   queue.clear();queue.push_back(start);visited[start]=stamp;bool valid=true;
   for(size_t head=0;head<queue.size()&&valid;head++){int i=queue[head],px=i%w,py=i/w;if(px<2||py<2||px>=w-2||py>=h-2||a[i*4+3]<250||mask[i]){valid=false;break;}
    for(int dy=-1;dy<=1&&valid;dy++)for(int dx=-1;dx<=1;dx++){int j=(py+dy)*w+px+dx;if(visited[j]==stamp)continue;visited[j]=stamp;if(a[j*4+3]<250){valid=false;break;}if(Difference(a.data()+j*4,bg)>3){queue.push_back(j);if(queue.size()>size_t(p.area)){valid=false;break;}}}}
   if(!valid)continue;for(int i:queue)membership[i]=stamp;
   for(int i:queue){int px=i%w,py=i/w;for(int dy=-2;dy<=2&&valid;dy++)for(int dx=-2;dx<=2;dx++){int j=(py+dy)*w+px+dx;if(membership[j]==stamp)continue;if(a[j*4+3]<250){valid=false;break;}if(Difference(a.data()+j*4,bg)>3){int ux=Sign(dx),uy=Sign(dy),back=(py-uy)*w+px-ux,fx=px+dx+ux,fy=py+dy+uy;if(membership[back]==stamp&&fx>=0&&fy>=0&&fx<w&&fy<h&&Difference(a.data()+(fy*w+fx)*4,bg)>3){valid=false;break;}}}if(!valid)break;}
   if(!valid)continue;Group g;g.points=queue;g.solid=true;g.background=bg;groups.push_back(std::move(g));for(int i:queue)mask[i]=255;break;
  }
 }}
 std::vector<uint8_t> output(a.size());std::vector<uint32_t> sampleSeen(n);uint32_t sampleStamp=0;
 struct Cluster{Color mean;std::vector<int> ids;};
 for(const Group& group:groups){Cancel(cancel);if(group.solid){for(int i:group.points){for(int c=0;c<3;c++)output[i*4+c]=uint8_t(group.background[c]);output[i*4+3]=255;}continue;}
  ++sampleStamp;std::vector<int> samples;for(int i:group.points){int x=i%w,y=i/w;for(int dy=-5;dy<=5;dy++)for(int dx=-5;dx<=5;dx++){if(dx*dx+dy*dy>25)continue;int nx=x+dx,ny=y+dy;if(nx<0||ny<0||nx>=w||ny>=h)continue;int j=ny*w+nx;if(!dark[j]&&a[j*4+3]>=250&&sampleSeen[j]!=sampleStamp){sampleSeen[j]=sampleStamp;samples.push_back(j);}}}
  if(samples.size()<4)continue;std::vector<Cluster> clusters;
  for(int j:samples){Color rgb=Pixel(a.data()+j*4);int best=-1;double limit=400;for(size_t c=0;c<clusters.size();c++){double distance=0;for(int k=0;k<3;k++){double d=clusters[c].mean[k]-rgb[k];distance+=d*d;}if(distance<=limit){limit=distance;best=int(c);}}
   if(best<0){clusters.push_back({rgb,{j}});continue;}auto& cluster=clusters[best];double count=double(cluster.ids.size());for(int k=0;k<3;k++)cluster.mean[k]=(cluster.mean[k]*count+rgb[k])/(count+1);cluster.ids.push_back(j);}
  std::stable_sort(clusters.begin(),clusters.end(),[](const Cluster& a,const Cluster& b){return a.ids.size()>b.ids.size();});if(double(clusters[0].ids.size())/double(samples.size())<.85)continue;
  for(int i:group.points){int x=i%w,y=i/w;double sum=0,r=0,g=0,b=0;for(int j:clusters[0].ids){int dx=x-j%w,dy=y-j/w;double weight=1.0/(1+dx*dx+dy*dy);sum+=weight;r+=a[j*4]*weight;g+=a[j*4+1]*weight;b+=a[j*4+2]*weight;}
   output[i*4]=uint8_t(std::floor(r/sum+.5));output[i*4+1]=uint8_t(std::floor(g/sum+.5));output[i*4+2]=uint8_t(std::floor(b/sum+.5));output[i*4+3]=255;}
 }
 return output;
}
}
std::vector<TilePatch> CleanTiles(const std::vector<uint8_t>& rgba,int w,int h,Settings p,std::atomic<bool>& cancel,const std::vector<int>& tileIds,std::function<void(int)> progress){
 if(w<=0||h<=0||rgba.size()!=size_t(w)*h*4||p.area<1||p.area>1000||p.strength<0||p.strength>100)throw std::runtime_error("Invalid image or parameters");Cancel(cancel);
 std::vector<TilePatch> output;const int halo=2*p.area+20,cols=(w+1023)/1024,rows=(h+1023)/1024;int done=0;
 for(int id:tileIds){if(id<0||id>=cols*rows)throw std::runtime_error("Invalid tile");int cy=(id/cols)*1024,cx=(id%cols)*1024;Cancel(cancel);int cr=std::min(w,cx+1024),cb=std::min(h,cy+1024),l=std::max(0,cx-halo),t=std::max(0,cy-halo),r=std::min(w,cr+halo),b=std::min(h,cb+halo),tw=r-l,th=b-t;
  std::vector<uint8_t> input(size_t(tw)*th*4);for(int y=0;y<th;y++)memcpy(input.data()+size_t(y)*tw*4,rgba.data()+(size_t(y+t)*w+l)*4,size_t(tw)*4);
  auto patch=Tile(input,tw,th,p,{l>0,t>0,r<w,b<h},cancel);
  TilePatch core{{cx,cy,cr,cb},std::vector<uint8_t>(size_t(cr-cx)*(cb-cy)*4),id};
  for(int y=cy;y<cb;y++)memcpy(core.pixels.data()+size_t(y-cy)*(cr-cx)*4,patch.data()+(size_t(y-t)*tw+cx-l)*4,size_t(cr-cx)*4);output.push_back(std::move(core));if(progress)progress(++done*100/int(tileIds.size()));
 }
 return output;
}
std::vector<uint8_t> CleanRegion(const std::vector<uint8_t>& rgba,int w,int h,Settings p,std::atomic<bool>& cancel,ImageRect region,std::function<void(int)> progress){
 if(w<=0||h<=0)throw std::runtime_error("Invalid dimensions");std::vector<int> ids;int id=0;for(int y=0;y<h;y+=1024)for(int x=0;x<w;x+=1024,id++)if(x<region.right&&y<region.bottom&&x+1024>region.left&&y+1024>region.top)ids.push_back(id);
 auto tiles=CleanTiles(rgba,w,h,p,cancel,ids,progress);std::vector<uint8_t> output(rgba.size());for(auto& tile:tiles){auto b=tile.bounds;for(int y=b.top;y<b.bottom;y++)memcpy(output.data()+(size_t(y)*w+b.left)*4,tile.pixels.data()+size_t(y-b.top)*(b.right-b.left)*4,size_t(b.right-b.left)*4);}return output;
}
std::vector<uint8_t> Clean(const std::vector<uint8_t>& rgba,int w,int h,Settings p,std::atomic<bool>& cancel,std::function<void(int)> progress){return CleanRegion(rgba,w,h,p,cancel,{0,0,w,h},progress);}
