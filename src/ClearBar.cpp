#define WIN32_LEAN_AND_MEAN
#include <windows.h>
<<<<<<< HEAD

// windows.h defines GetCurrentTime() as a macro; undefine it before
// including XAML SDK headers that declare a GetCurrentTime member.
#ifdef GetCurrentTime
#undef GetCurrentTime
#endif
=======
>>>>>>> origin/main
#include <Windows.UI.Xaml.h>
#include <xamlom.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include "common.h"
#include "TaskbarAppearance.h"

namespace {
constexpr UINT_PTR TIMER_ID=1;
DWORD injectedPid=0;
UINT taskbarCreated=0;

std::filesystem::path Dir(){
    wchar_t p[MAX_PATH]{};
    DWORD n=GetModuleFileNameW(nullptr,p,MAX_PATH);
    return (!n||n==MAX_PATH)?std::filesystem::path{}:std::filesystem::path(p).parent_path();
}
void Log(const std::wstring& s){
    try{
        std::wofstream f(Dir()/L"clearbar.log",std::ios::app);
        SYSTEMTIME t{}; GetLocalTime(&t);
        f<<L"["<<std::setfill(L'0')<<std::setw(2)<<t.wHour<<L":"
         <<std::setw(2)<<t.wMinute<<L":"<<std::setw(2)<<t.wSecond<<L"] "<<s<<L"\n";
    }catch(...){}
}
std::wstring Hex(HRESULT hr){
    std::wstringstream s; s<<L"0x"<<std::hex<<std::uppercase<<std::setw(8)
    <<std::setfill(L'0')<<(unsigned long)hr; return s.str();
}
DWORD ExplorerPid(){
    HWND h=FindWindowW(L"Shell_TrayWnd",nullptr);
    DWORD p=0; if(h) GetWindowThreadProcessId(h,&p); return p;
}
bool SupportedWindows(){
    using VersionFn=LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
    auto ntdll=GetModuleHandleW(L"ntdll.dll");
    auto version=ntdll?reinterpret_cast<VersionFn>(GetProcAddress(ntdll,"RtlGetVersion")):nullptr;
    RTL_OSVERSIONINFOW info{}; info.dwOSVersionInfoSize=sizeof(info);
    return version && version(&info)==0 && info.dwMajorVersion==10 && info.dwBuildNumber>=22000;
}
bool Applied(TaskbarMode mode){
    std::wifstream file(Dir()/L"clearbar.state");
    DWORD pid{}; std::wstring name;
    file>>pid>>name;
    return file && pid==ExplorerPid() && name==Preset(mode).name;
}
HRESULT Inject(DWORD pid){
    auto tap=Dir()/L"ClearBarTAP.dll";
    if(!std::filesystem::exists(tap)) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    HMODULE x=LoadLibraryExW(L"Windows.UI.Xaml.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!x) return HRESULT_FROM_WIN32(GetLastError());

    using Fn=decltype(&InitializeXamlDiagnosticsEx);
    auto fn=reinterpret_cast<Fn>(GetProcAddress(x,"InitializeXamlDiagnosticsEx"));
    if(!fn){ HRESULT hr=HRESULT_FROM_WIN32(GetLastError()); FreeLibrary(x); return hr; }

    HRESULT hr=HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
<<<<<<< HEAD
    for(int i=1;i<=10000;i++){
=======
    for(int i=1;i<=64;i++){
>>>>>>> origin/main
        wchar_t name[64]{};
        swprintf_s(name,L"VisualDiagConnection%d",i);
        hr=fn(name,pid,L"Windows.UI.Xaml.dll",tap.c_str(),CLSID_ClearBarTAP,nullptr);
        if(hr!=HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) break;
    }
    FreeLibrary(x);
    return hr;
}
void TryInject(){
    DWORD pid=ExplorerPid();
    if(!pid){ injectedPid=0; return; }
    if(pid==injectedPid) return;
    HRESULT hr=Inject(pid);
    if(SUCCEEDED(hr)){
        injectedPid=pid;
        Log(L"Injected into Explorer PID "+std::to_wstring(pid));
    }else{
        Log(L"Injection failed for Explorer PID "+std::to_wstring(pid)+L" with "+Hex(hr));
    }
}
LRESULT CALLBACK Proc(HWND h,UINT m,WPARAM w,LPARAM l){
    if(m==taskbarCreated){ injectedPid=0; TryInject(); return 0; }
    if(m==WM_TIMER && w==TIMER_ID){ TryInject(); return 0; }
    if(m==WM_DESTROY){ KillTimer(h,TIMER_ID); PostQuitMessage(0); return 0; }
    return DefWindowProcW(h,m,w,l);
}
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,PWSTR args,int){
    std::wstring_view command=args?args:L"";
    while(!command.empty() && (command.front()==L' ' || command.front()==L'\t')) command.remove_prefix(1);
    while(!command.empty() && (command.back()==L' ' || command.back()==L'\t')) command.remove_suffix(1);
    if(command==L"--supported") return SupportedWindows()?0:1;
    if(command.starts_with(L"--set ")){
        TaskbarMode requested{};
        return ParseMode(command.substr(6),requested) && SaveMode(nullptr,requested)?0:1;
    }
    if(command.starts_with(L"--expect-config ")){
        TaskbarMode expected{};
        const auto loaded=LoadMode(nullptr);
        return ParseMode(command.substr(16),expected) && loaded.valid && loaded.mode==expected?0:1;
    }
    if(command.starts_with(L"--check ")){
        TaskbarMode requested{};
        return ParseMode(command.substr(8),requested) && Applied(requested)?0:1;
    }
    if(!command.empty()) return 1;
    if(!SupportedWindows()) { Log(L"Unsupported Windows version; Windows 11 is required"); return 3; }
    HANDLE mutex=CreateMutexW(nullptr,TRUE,L"Local\\ClearBarV2Controller");
    if(!mutex) return 1;
    if(GetLastError()==ERROR_ALREADY_EXISTS){ CloseHandle(mutex); return 0; }

    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    const auto loaded=LoadMode(nullptr);
    Log(L"ClearBar v2.3.0 started with " + std::wstring(Preset(loaded.mode).name));
    if(!loaded.valid) Log(L"Missing or invalid configuration; using safe fallback");
    taskbarCreated=RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc); wc.lpfnWndProc=Proc;
    wc.hInstance=hi; wc.lpszClassName=L"ClearBarV2HiddenWindow";
    RegisterClassExW(&wc);

    HWND h=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,wc.lpszClassName,L"ClearBar",
        WS_POPUP,-32000,-32000,1,1,nullptr,nullptr,hi,nullptr);
    if(!h){ Log(L"Hidden window creation failed"); CoUninitialize(); CloseHandle(mutex); return 2; }

    SetTimer(h,TIMER_ID,5000,nullptr);
    TryInject();

    MSG msg{};
    while(GetMessageW(&msg,nullptr,0,0)>0){ TranslateMessage(&msg); DispatchMessageW(&msg); }

    CoUninitialize();
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return 0;
}
