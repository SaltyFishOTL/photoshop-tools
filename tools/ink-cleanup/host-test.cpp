#define MSWindows 1
#include <windows.h>
#undef WIN32
#define WIN32 1
#include "PIDefines.h"
#include "PIFilter.h"
#include <vector>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <algorithm>
using Entry=void(*)(int16,FilterRecord*,intptr_t*,int16*);
struct Block{std::vector<uint8_t> bytes;};
Handle New(int32 size){auto p=new Block;p->bytes.resize(size);return (Handle)p;}
void Dispose(Handle h){delete (Block*)h;}
int32 Size(Handle h){return int32(((Block*)h)->bytes.size());}
Ptr Lock(Handle h,Boolean){return (Ptr)((Block*)h)->bytes.data();}
void Unlock(Handle){}
struct Desc {int strength=60,area=60;};
struct Reader {Desc d;int pos=0;};
PIReadDescriptor OpenRead(PIDescriptorHandle h,DescriptorKeyIDArray){auto r=new Reader;memcpy(&r->d,Lock(h,1),sizeof(Desc));return (PIReadDescriptor)r;}
Boolean GetKey(PIReadDescriptor t,DescriptorKeyID* k,DescriptorTypeID* type,int32* flags){auto r=(Reader*)t;if(r->pos==2)return false;*k=r->pos++==0?'IcSt':'IcAr';*type='long';*flags=0;return true;}
OSErr GetInteger(PIReadDescriptor t,int32* out){auto r=(Reader*)t;*out=r->pos==1?r->d.strength:r->d.area;return 0;}
OSErr CloseRead(PIReadDescriptor t){delete (Reader*)t;return 0;}
PIWriteDescriptor OpenWrite(){return (PIWriteDescriptor)new Desc;}
OSErr PutInteger(PIWriteDescriptor t,DescriptorKeyID key,int32 value){auto d=(Desc*)t;if(key=='IcSt')d->strength=value;else if(key=='IcAr')d->area=value;else return -1;return 0;}
OSErr CloseWrite(PIWriteDescriptor t,PIDescriptorHandle* out){*out=New(sizeof(Desc));memcpy(Lock(*out,1),t,sizeof(Desc));delete (Desc*)t;return 0;}
struct Host {
 FilterRecord f{};BigDocumentStruct big{};HandleProcs handles{};ReadDescriptorProcs read{};WriteDescriptorProcs write{};PIDescriptorParameters dp{};
 std::vector<uint8_t> source,destination,input,output;VRect pending{};int abortAfter=0,abortCalls=0,outputRequests=0;bool failOutput=false,selected=false;
 Host(){int w=80,h=80;source.resize(w*h*4);for(int i=0;i<w*h;i++){source[i*4]=170;source[i*4+1]=135;source[i*4+2]=100;source[i*4+3]=255;}for(auto x:{20,60})for(int c=0;c<3;c++)source[(30*w+x)*4+c]=7;source[(50*w+50)*4+3]=0;destination=source;
 handles.newProc=New;handles.disposeProc=Dispose;handles.getSizeProc=Size;handles.lockProc=Lock;handles.unlockProc=Unlock;f.handleProcs=&handles;
 read.openReadDescriptorProc=OpenRead;read.getKeyProc=GetKey;read.getIntegerProc=GetInteger;read.closeReadDescriptorProc=CloseRead;write.openWriteDescriptorProc=OpenWrite;write.putIntegerProc=PutInteger;write.closeWriteDescriptorProc=CloseWrite;
 dp.readDescriptorProcs=&read;dp.writeDescriptorProcs=&write;dp.playInfo=plugInDialogDontDisplay;dp.descriptor=New(sizeof(Desc));Desc d{60,60};memcpy(Lock(dp.descriptor,1),&d,sizeof d);f.descriptorParameters=&dp;
 big.imageSize32={80,80};big.filterRect32={0,0,80,80};f.bigDocumentData=&big;f.planes=4;f.inLayerPlanes=3;f.outLayerPlanes=3;f.inTransparencyMask=1;f.outTransparencyMask=1;f.depth=8;f.imageMode=plugInModeRGBColor;}
 ~Host(){if(f.parameters)Dispose(f.parameters);if(dp.descriptor)Dispose(dp.descriptor);}
 OSErr advance(){if(pending.bottom>pending.top){int width=pending.right-pending.left;for(int y=pending.top;y<pending.bottom;y++)for(int x=pending.left;x<pending.right;x++){size_t k=(y*80+x)*4;auto src=output.data()+(y-pending.top)*(width*8+16)+(x-pending.left)*8;for(int c=0;c<3;c++)destination[k+c]=src[c*2];}pending={};}
 auto in=big.inRect32;f.inData=nullptr;if(in.bottom>in.top){int w=in.right-in.left,h=in.bottom-in.top;input.assign(h*(w*10+16),0);for(int y=0;y<h;y++)for(int x=0;x<w;x++)for(int c=0;c<4;c++)input[y*(w*10+16)+x*10+c*2]=source[((y+in.top)*80+x+in.left)*4+c];f.inData=input.data();f.inRowBytes=w*10+16;f.inColumnBytes=10;f.inPlaneBytes=2;}
 auto out=big.outRect32;f.outData=nullptr;if(out.bottom>out.top){outputRequests++;if(failOutput)return filterBadParameters;int w=out.right-out.left;output.assign((out.bottom-out.top)*(w*8+16),0);f.outData=output.data();f.outRowBytes=w*8+16;f.outColumnBytes=8;f.outPlaneBytes=2;pending=out;}return 0;}
};
Host* current=nullptr;OSErr Advance(){return current->advance();}Boolean Abort(){return current->abortAfter&&++current->abortCalls>=current->abortAfter;}void Progress(int32,int32){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(int argc,char** argv){try{require(argc==2,"DLL path required");HMODULE dll=LoadLibraryA(argv[1]);require(dll!=nullptr,"LoadLibrary failed");auto entry=(Entry)GetProcAddress(dll,"PluginMain");require(entry!=nullptr,"PluginMain missing");require(FindResourceW(dll,L"JSON_PIPL",L"JSON")!=nullptr,"Native menu resource missing");
 for(int test=0;test<6;test++){Host host;current=&host;host.f.advanceState=Advance;host.f.abortProc=Abort;host.f.progressProc=Progress;if(test==1)host.big.filterRect32={10,10,45,40};if(test==2)host.abortAfter=1;if(test==5)host.abortAfter=2;if(test==3)host.failOutput=true;if(test==4){Desc d{60,1};memcpy(Lock(host.dp.descriptor,1),&d,sizeof d);}
 intptr_t data=0;int16 error=0;entry(filterSelectorPrepare,&host.f,&data,&error);require(error==0,"prepare failed");entry(filterSelectorStart,&host.f,&data,&error);
 if(test==2||test==5){require(error==userCanceledErr,"cancel not propagated");require(host.source==host.destination&&host.outputRequests==0,"cancel wrote pixels");}
 else if(test==3){require(error!=0&&host.source==host.destination,"output failure not propagated");}
 else {require(error==0,"filter failed");require(host.destination[(30*80+20)*4]==170,"dot not cleaned");require(host.destination[(30*80+60)*4]==(test==1?7:170),"selection bounds wrong");require(host.destination[(50*80+50)*4+3]==0,"transparency changed");require(host.dp.descriptor!=nullptr,"parameter descriptor not recorded");Desc d;memcpy(&d,Lock(host.dp.descriptor,1),sizeof d);require(d.strength==60&&d.area==(test==4?1:60),"recorded parameters wrong");}
 entry(filterSelectorFinish,&host.f,&data,&error);require(data==0,"state not released");}
 FreeLibrary(dll);std::cout<<"PASS native ABI, menu resource, padded/strided RGB/alpha, selection bounds, cancel, failure, descriptor replay/record\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
