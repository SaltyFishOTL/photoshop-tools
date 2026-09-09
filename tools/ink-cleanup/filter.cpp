#include "PIDefines.h"
#include <windows.h>
#include <windowsx.h>
#undef WIN32
#define WIN32 1
#include "PIFilter.h"
#include "PIAbout.h"
#include "engine.h"
#include "preview-cache.h"
#include <memory>
#include <algorithm>
#include <thread>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>
#include <stdexcept>
namespace {
HINSTANCE instance=nullptr;
constexpr UINT Done=WM_APP+41,Progress=WM_APP+42;
constexpr DescriptorKeyID Strength='IcSt',Area='IcAr';
struct Saved {uint32_t magic=0x49433037;int strength=60,area=60;};
struct State {
 FilterRecord* f=nullptr;bool show=false,busy=false,pending=false,closing=false,ready=false,initializing=false;
 int w=0,h=0;Settings settings;std::vector<uint8_t> input,patch;std::atomic<bool> cancel{false};std::atomic<int> percent{0};
 std::thread worker;std::string error;HWND dialog=nullptr;double zoom=1,centerX=0,centerY=0;bool dragging=false;POINT drag{};
 std::unique_ptr<PreviewCache> cache;std::vector<TilePatch> batch;uint64_t revision=0,jobRevision=0;bool applyRequested=false;
 ~State(){cancel=true;if(worker.joinable())worker.join();}
};
void Check(int16 code){if(code)throw code;}
void ClearRects(FilterRecord* f){f->inRect={};f->outRect={};f->maskRect={};if(f->bigDocumentData){f->bigDocumentData->inRect32={};f->bigDocumentData->outRect32={};f->bigDocumentData->maskRect32={};}}
void ReadSettings(State& s){auto f=s.f;if(f->parameters&&f->handleProcs){auto hp=f->handleProcs;if(hp->getSizeProc(f->parameters)==sizeof(Saved)){auto p=(Saved*)hp->lockProc(f->parameters,true);if(p&&p->magic==0x49433037)s.settings={p->strength,p->area};hp->unlockProc(f->parameters);}}
 auto dp=f->descriptorParameters;if(dp&&dp->descriptor&&dp->readDescriptorProcs){auto rp=dp->readDescriptorProcs;DescriptorKeyID keys[]={Strength,Area,0};auto token=rp->openReadDescriptorProc(dp->descriptor,keys);if(!token)throw std::runtime_error("Cannot read filter parameters");DescriptorKeyID key;DescriptorTypeID type;int32 flags;int16 error=0;
 while(rp->getKeyProc(token,&key,&type,&flags)){int32 value=0;if(key==Strength||key==Area){int16 e=rp->getIntegerProc(token,&value);if(e)error=e;else if(key==Strength)s.settings.strength=value;else s.settings.area=value;}}
 int16 end=rp->closeReadDescriptorProc(token);if(f->handleProcs)f->handleProcs->disposeProc(dp->descriptor);dp->descriptor=nullptr;Check(error?error:end);s.show=dp->playInfo==plugInDialogDisplay;}
 if(s.settings.strength<0||s.settings.strength>100||s.settings.area<1||s.settings.area>1000)throw std::runtime_error("Invalid saved parameters");
}
void SaveSettings(State& s){auto f=s.f;auto hp=f->handleProcs;if(hp){if(!f->parameters)f->parameters=hp->newProc(sizeof(Saved));if(!f->parameters)throw std::bad_alloc();auto p=(Saved*)hp->lockProc(f->parameters,true);if(!p)throw std::bad_alloc();*p={0x49433037,s.settings.strength,s.settings.area};hp->unlockProc(f->parameters);}
 auto dp=f->descriptorParameters;if(dp&&dp->writeDescriptorProcs){auto wp=dp->writeDescriptorProcs;auto token=wp->openWriteDescriptorProc();if(!token)throw std::runtime_error("Cannot record filter parameters");int16 a=wp->putIntegerProc(token,Strength,s.settings.strength),b=wp->putIntegerProc(token,Area,s.settings.area);PIDescriptorHandle handle=nullptr;int16 c=wp->closeWriteDescriptorProc(token,&handle);if(a||b||c){if(handle&&hp)hp->disposeProc(handle);Check(a?a:b?b:c);}if(dp->descriptor&&hp)hp->disposeProc(dp->descriptor);dp->descriptor=handle;dp->recordInfo=plugInDialogOptional;}
}
void ReadImage(State& s){auto f=s.f;if(f->imageMode!=plugInModeRGBColor||f->depth!=8||f->planes<3||!f->bigDocumentData||!f->advanceState)throw std::runtime_error("Requires Photoshop RGB 8-bit color layer");
 auto big=f->bigDocumentData;big->PluginUsing32BitCoordinates=1;s.w=big->imageSize32.h;s.h=big->imageSize32.v;
 if(s.w<=0||s.h<=0||uint64_t(s.w)*s.h>SIZE_MAX/4)throw std::runtime_error("Invalid canvas size");
 s.input.resize(size_t(s.w)*s.h*4);for(size_t i=3;i<s.input.size();i+=4)s.input[i]=255;
 int channels=f->inTransparencyMask==1?4:3;f->inLoPlane=0;f->inHiPlane=channels-1;f->autoMask=true;
 // Copy source in strips before any output request; preview never mutates the document.
 for(int y=0;y<s.h;y+=256){if(f->abortProc&&f->abortProc())throw int16(userCanceledErr);int bottom=std::min(s.h,y+256);ClearRects(f);big->inRect32={y,0,bottom,s.w};Check(f->advanceState());if(!f->inData)throw std::runtime_error("Photoshop returned no input pixels");int col=f->inColumnBytes?f->inColumnBytes:channels,plane=f->inPlaneBytes?f->inPlaneBytes:1;
 for(int row=y;row<bottom;row++)for(int x=0;x<s.w;x++){auto in=(uint8_t*)f->inData+ptrdiff_t(row-y)*f->inRowBytes+ptrdiff_t(x)*col;auto out=s.input.data()+(size_t(row)*s.w+x)*4;for(int c=0;c<channels;c++)out[c]=in[c*plane];}}
 ClearRects(f);Check(f->advanceState());s.centerX=s.w/2.0;s.centerY=s.h/2.0;
}
State* Get(HWND h){return (State*)GetWindowLongPtrW(h,GWLP_USERDATA);}
void Launch(State& s);
void ClampView(State& s,int width,int height){double halfW=width/(2*s.zoom),halfH=height/(2*s.zoom);s.centerX=halfW>=s.w/2.0?s.w/2.0:std::clamp(s.centerX,halfW,s.w-halfW);s.centerY=halfH>=s.h/2.0?s.h/2.0:std::clamp(s.centerY,halfH,s.h-halfH);}
LRESULT CALLBACK Preview(HWND h,UINT msg,WPARAM wp,LPARAM lp){auto s=Get(GetParent(h));if(!s)return DefWindowProcW(h,msg,wp,lp);
 switch(msg){case WM_ERASEBKGND:return 1;
 case WM_LBUTTONDOWN:s->dragging=true;s->drag={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};SetCapture(h);return 0;
 case WM_LBUTTONUP:s->dragging=false;ReleaseCapture();SetTimer(s->dialog,1,120,nullptr);return 0;
 case WM_CAPTURECHANGED:s->dragging=false;return 0;
 case WM_MOUSEMOVE:if(s->dragging){POINT now{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};s->centerX-=(now.x-s->drag.x)/s->zoom;s->centerY-=(now.y-s->drag.y)/s->zoom;s->drag=now;s->ready=false;SetTimer(s->dialog,1,120,nullptr);InvalidateRect(h,nullptr,false);}return 0;
 case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT rc;GetClientRect(h,&rc);int w=rc.right,hg=rc.bottom;ClampView(*s,w,hg);std::vector<uint32_t> display(size_t(w)*hg);bool after=s->ready&&IsDlgButtonChecked(s->dialog,122)==BST_CHECKED;
 for(int y=0;y<hg;y++)for(int x=0;x<w;x++){int ix=int(floor(s->centerX+(x-w/2.0)/s->zoom)),iy=int(floor(s->centerY+(y-hg/2.0)/s->zoom));uint32_t color=0x303030;if(ix>=0&&iy>=0&&ix<s->w&&iy<s->h){size_t k=(size_t(iy)*s->w+ix)*4;auto src=s->input.data()+k;if(after&&s->cache&&s->cache->pixels[k+3])src=s->cache->pixels.data()+k;int a=s->input[k+3],bg=((ix/8+iy/8)&1)?192:224;int r=(src[0]*a+bg*(255-a)+127)/255,g=(src[1]*a+bg*(255-a)+127)/255,b=(src[2]*a+bg*(255-a)+127)/255;color=(r<<16)|(g<<8)|b;}display[size_t(y)*w+x]=color;}
 BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=w;bi.bmiHeader.biHeight=-hg;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;SetDIBitsToDevice(dc,0,0,w,hg,0,0,0,hg,display.data(),&bi,DIB_RGB_COLORS);EndPaint(h,&ps);return 0;}}
 return DefWindowProcW(h,msg,wp,lp);
}
bool ReadControls(State& s){BOOL a,b;int strength=GetDlgItemInt(s.dialog,120,&a,false),area=GetDlgItemInt(s.dialog,121,&b,false);if(!a||!b||strength>100||area<1||area>1000){SetDlgItemTextW(s.dialog,124,L"识别强度 0–100；最大面积 1–1000。");return false;}s.settings={strength,area};return true;}
void Launch(State& s){
 if(s.busy){s.pending=true;return;}s.pending=false;if(!ReadControls(s)){EnableWindow(GetDlgItem(s.dialog,IDOK),false);return;}
 if(!s.cache)s.cache=std::make_unique<PreviewCache>(s.w,s.h);s.cache->Configure(s.settings);
 RECT rc;GetClientRect(GetDlgItem(s.dialog,110),&rc);ClampView(s,rc.right,rc.bottom);
 ImageRect visible{std::max(0,int(std::floor(s.centerX-rc.right/(2*s.zoom)))),std::max(0,int(std::floor(s.centerY-rc.bottom/(2*s.zoom)))),std::min(s.w,int(std::ceil(s.centerX+rc.right/(2*s.zoom)))),std::min(s.h,int(std::ceil(s.centerY+rc.bottom/(2*s.zoom))))};
 auto ids=s.cache->Missing(s.applyRequested?ImageRect{0,0,s.w,s.h}:visible);
 if(ids.empty()){if(s.applyRequested){s.ready=false;s.patch=std::move(s.cache->pixels);EndDialog(s.dialog,IDOK);return;}s.ready=true;EnableWindow(GetDlgItem(s.dialog,IDOK),true);SetDlgItemTextW(s.dialog,124,L"预览就绪。拖动查看其他位置；已计算区域会复用。");InvalidateRect(GetDlgItem(s.dialog,110),nullptr,false);return;}
 s.ready=false;s.cancel=false;s.busy=true;s.error.clear();s.batch.clear();s.percent=0;s.jobRevision=s.revision;EnableWindow(GetDlgItem(s.dialog,IDOK),!s.applyRequested);
 SetDlgItemTextW(s.dialog,124,s.applyRequested?L"正在计算剩余画布，已完成的预览会复用…":L"正在计算当前视野…");auto p=s.settings;
 s.worker=std::thread([&s,p,ids]{try{s.batch=CleanTiles(s.input,s.w,s.h,p,s.cancel,ids,[&s](int percent){PostMessageW(s.dialog,Progress,percent,0);});}catch(const std::exception& e){s.error=e.what();}PostMessageW(s.dialog,Done,0,0);});
}
INT_PTR CALLBACK Dialog(HWND h,UINT msg,WPARAM wp,LPARAM lp){auto s=Get(h);if(msg==WM_INITDIALOG){s=(State*)lp;SetWindowLongPtrW(h,GWLP_USERDATA,(LONG_PTR)s);s->dialog=h;s->initializing=true;SetDlgItemInt(h,120,s->settings.strength,false);SetDlgItemInt(h,121,s->settings.area,false);CheckDlgButton(h,122,BST_CHECKED);for(auto text:{L"50%",L"100%",L"200%",L"400%"})SendDlgItemMessageW(h,123,CB_ADDSTRING,0,(LPARAM)text);SendDlgItemMessageW(h,123,CB_SETCURSEL,1,0);s->initializing=false;Launch(*s);return TRUE;}if(!s)return FALSE;
 if(msg==WM_TIMER){KillTimer(h,1);Launch(*s);return TRUE;}
 if(msg==Progress){std::wstring text=(s->applyRequested?L"正在计算剩余画布… ":L"正在计算当前视野… ")+std::to_wstring(wp)+L"%";SetDlgItemTextW(h,124,text.c_str());return TRUE;}
 if(msg==Done){if(s->worker.joinable())s->worker.join();s->busy=false;if(s->closing){EndDialog(h,IDCANCEL);return TRUE;}
  if(!s->cancel&&s->error.empty()&&s->jobRevision==s->revision)s->cache->Merge(s->batch);s->batch.clear();
  if(!s->cancel&&!s->error.empty()){s->applyRequested=false;EnableWindow(GetDlgItem(h,120),true);EnableWindow(GetDlgItem(h,121),true);EnableWindow(GetDlgItem(h,IDOK),false);SetDlgItemTextW(h,124,L"计算未完成，请调整参数重试或取消。");return TRUE;}
  Launch(*s);return TRUE;}
 if(msg==WM_CLOSE||(msg==WM_COMMAND&&LOWORD(wp)==IDCANCEL)){KillTimer(h,1);if(s->busy){s->closing=true;s->pending=false;s->cancel=true;SetDlgItemTextW(h,124,L"正在取消…");}else EndDialog(h,IDCANCEL);return TRUE;}
 if(msg==WM_COMMAND){int id=LOWORD(wp),code=HIWORD(wp);if((id==120||id==121)&&code==EN_CHANGE&&!s->initializing){s->revision++;s->ready=false;EnableWindow(GetDlgItem(h,IDOK),false);s->cancel=true;SetTimer(h,1,300,nullptr);return TRUE;}if(id==122){InvalidateRect(GetDlgItem(h,110),nullptr,false);return TRUE;}if(id==123&&code==CBN_SELCHANGE){int index=(int)SendDlgItemMessageW(h,123,CB_GETCURSEL,0,0);s->zoom=index==0?.5:index==1?1:index==2?2:4;s->ready=false;InvalidateRect(GetDlgItem(h,110),nullptr,false);SetTimer(h,1,120,nullptr);return TRUE;}if(id==IDOK&&!s->applyRequested){if(!ReadControls(*s))return TRUE;KillTimer(h,1);s->applyRequested=true;EnableWindow(GetDlgItem(h,120),false);EnableWindow(GetDlgItem(h,121),false);EnableWindow(GetDlgItem(h,IDOK),false);Launch(*s);return TRUE;}}
 return FALSE;
}
void Compute(State& s){if(s.show){WNDCLASSW wc{};wc.lpfnWndProc=Preview;wc.hInstance=instance;wc.lpszClassName=L"InkPreview07";wc.hCursor=LoadCursorW(nullptr,IDC_HAND);RegisterClassW(&wc);HWND parent=s.f->platformData?(HWND)((PlatformData*)s.f->platformData)->hwnd:nullptr;auto result=DialogBoxParamW(instance,MAKEINTRESOURCEW(101),parent,Dialog,(LPARAM)&s);if(result==-1)throw std::runtime_error("Cannot open filter dialog");if(result!=IDOK)throw int16(userCanceledErr);}
 else {std::atomic<bool> done=false;s.worker=std::thread([&s,&done]{try{s.patch=Clean(s.input,s.w,s.h,s.settings,s.cancel,[&s](int p){s.percent=p;});}catch(const std::exception& e){s.error=e.what();}done=true;});while(!done){if(s.f->abortProc&&s.f->abortProc())s.cancel=true;if(s.f->progressProc)s.f->progressProc(s.percent,100);std::this_thread::sleep_for(std::chrono::milliseconds(30));}s.worker.join();if(s.cancel)throw int16(userCanceledErr);if(!s.error.empty())throw std::runtime_error(s.error);}
}
void WriteImage(State& s){auto f=s.f;auto big=f->bigDocumentData;auto bounds=big->filterRect32;f->outLoPlane=0;f->outHiPlane=2;
 for(int y=bounds.top;y<bounds.bottom;y+=256){if(f->abortProc&&f->abortProc())throw int16(userCanceledErr);ClearRects(f);int bottom=std::min<int>(bounds.bottom,y+256);big->outRect32={y,bounds.left,bottom,bounds.right};Check(f->advanceState());if(!f->outData)throw std::runtime_error("Photoshop returned no output pixels");int col=f->outColumnBytes?f->outColumnBytes:3,plane=f->outPlaneBytes?f->outPlaneBytes:1;
 for(int row=y;row<bottom;row++)for(int x=bounds.left;x<bounds.right;x++){if(x<0||row<0||x>=s.w||row>=s.h)throw std::runtime_error("Output outside canvas");size_t k=(size_t(row)*s.w+x)*4;auto in=(s.patch[k+3]?s.patch.data():s.input.data())+k;auto out=(uint8_t*)f->outData+ptrdiff_t(row-y)*f->outRowBytes+ptrdiff_t(x-bounds.left)*col;for(int c=0;c<3;c++)out[c*plane]=in[c];}}
 ClearRects(f);Check(f->advanceState());
}
}
extern "C" __declspec(dllexport) void PluginMain(const int16 selector,FilterRecord* f,intptr_t* data,int16* result){
 *result=noErr;try {if(selector==filterSelectorAbout){auto about=(AboutRecord*)f;HWND owner=about->platformData?(HWND)((PlatformData*)about->platformData)->hwnd:nullptr;MessageBoxW(owner,L"全颜色碎点清理 0.9.0\n清理算法与已确认的 0.7 版相同。",L"碎点清理",MB_OK);return;}
 State* s=(State*)*data;
 if(selector==filterSelectorParameters||selector==filterSelectorPrepare){if(!s){s=new State;*data=(intptr_t)s;}s->f=f;if(selector==filterSelectorParameters)s->show=true;else {f->bufferSpace=0;if(f->bigDocumentData)f->bigDocumentData->PluginUsing32BitCoordinates=1;}return;}
 if(selector==filterSelectorStart){if(!s){s=new State;*data=(intptr_t)s;}s->f=f;ReadSettings(*s);ReadImage(*s);Compute(*s);WriteImage(*s);SaveSettings(*s);ClearRects(f);return;}
 if(selector==filterSelectorContinue){ClearRects(f);return;}
 if(selector==filterSelectorFinish){delete s;*data=0;return;}
 }catch(int16 error){*result=error;}catch(const std::bad_alloc&){*result=memFullErr;}catch(const std::exception& e){*result=filterBadParameters;auto s=(State*)*data;if(s&&s->show){std::string text=e.what();std::wstring wide(text.begin(),text.end());MessageBoxW(s->dialog,wide.c_str(),L"碎点清理失败",MB_OK|MB_ICONERROR);}}catch(...){*result=filterBadParameters;}
 if(selector!=filterSelectorAbout&&data){delete (State*)*data;*data=0;if(f)ClearRects(f);}
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
