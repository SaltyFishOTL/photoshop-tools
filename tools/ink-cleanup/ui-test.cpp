// Exercise this plugin's dialog on a private, non-visible desktop, never Photoshop.
#define main HostFixtureMain
#include "host-test.cpp"
#undef main
#include <string>
int mode=0,phase=0,ticks=0;bool timedOut=false;
void CALLBACK Drive(HWND,UINT,UINT_PTR,DWORD){
 HWND dialog=FindWindowW(L"#32770",L"全颜色碎点清理 0.11.0");if(!dialog)return;
 if(++ticks>200){timedOut=true;PostMessageW(dialog,WM_COMMAND,IDCANCEL,0);return;}
 if(mode==1){PostMessageW(dialog,WM_COMMAND,IDCANCEL,0);return;}
 if(phase==0){SetDlgItemTextW(dialog,120,L"35");SetDlgItemTextW(dialog,121,L"4");SetDlgItemTextW(dialog,120,L"60");SetDlgItemTextW(dialog,121,L"1");phase++;return;}
 if(phase==1){for(int checked:{BST_CHECKED,BST_UNCHECKED,BST_CHECKED}){CheckDlgButton(dialog,125,checked);SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(125,BN_CLICKED),(LPARAM)GetDlgItem(dialog,125));}SendDlgItemMessageW(dialog,123,CB_SETCURSEL,2,0);SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(123,CBN_SELCHANGE),(LPARAM)GetDlgItem(dialog,123));CheckDlgButton(dialog,122,BST_UNCHECKED);SendMessageW(dialog,WM_COMMAND,122,0);CheckDlgButton(dialog,122,BST_CHECKED);SendMessageW(dialog,WM_COMMAND,122,0);phase++;return;}
 if(IsWindowEnabled(GetDlgItem(dialog,IDOK))){PostMessageW(dialog,WM_COMMAND,IDOK,0);}
}
int main(int argc,char** argv){try{require(argc==2,"plugin path required");auto original=GetThreadDesktop(GetCurrentThreadId());HDESK desktop=CreateDesktopW(L"InkCleanupPrivateTest",nullptr,nullptr,0,GENERIC_ALL,nullptr);require(desktop!=nullptr,"Cannot create private test desktop");require(SetThreadDesktop(desktop)!=0,"Cannot enter private test desktop");HMODULE dll=LoadLibraryA(argv[1]);require(dll!=nullptr,"Cannot load plugin");auto entry=(Entry)GetProcAddress(dll,"PluginMain");require(entry!=nullptr,"Missing entry");
 for(mode=0;mode<2;mode++){Host host;current=&host;host.f.advanceState=Advance;host.f.abortProc=Abort;host.f.progressProc=Progress;host.dp.playInfo=plugInDialogDisplay;intptr_t data=0;int16 error=0;phase=0;ticks=0;timedOut=false;UINT_PTR timer=SetTimer(nullptr,0,30,Drive);require(timer!=0,"No UI test timer");entry(filterSelectorParameters,&host.f,&data,&error);entry(filterSelectorPrepare,&host.f,&data,&error);entry(filterSelectorStart,&host.f,&data,&error);KillTimer(nullptr,timer);require(!timedOut,"Dialog failed to settle");if(mode==0){require(error==0,"Apply failed");require(host.destination[(30*80+20)*4]==170,"Preview apply lost patch");Desc d;memcpy(&d,Lock(host.dp.descriptor,1),sizeof d);require(d.strength==60&&d.area==1&&d.precise==1,"Stale parameter job applied");}else{require(error==userCanceledErr,"Cancel failed");require(host.destination==host.source&&host.outputRequests==0,"Cancel changed document");}entry(filterSelectorFinish,&host.f,&data,&error);}
 FreeLibrary(dll);SetThreadDesktop(original);CloseDesktop(desktop);std::cout<<"PASS hidden native dialog: rapid parameter edits, zoom, before/after, apply current result, cancellation without writes\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
