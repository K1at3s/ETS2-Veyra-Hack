#include <windows.h>
#include <tlhelp32.h>
#include <dwmapi.h>
#include <objidl.h>
#include <gdiplus.h>
#include <array>
#include <deque>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include "core.hpp"
#include "local_pass_patch.hpp"

namespace {
constexpr COLORREF bg=RGB(15,13,23), side=RGB(21,17,32), card=RGB(29,24,42),
    border=RGB(49,40,66), ink=RGB(244,239,255), muted=RGB(163,151,184),
    accent=RGB(185,156,255), amber=RGB(218,190,143);
constexpr int ncz_id=106, top_id=108, refresh_id=109, tr_id=111, en_id=112;
struct LogEntry { std::wstring stamp, tr, en; };
struct State {
    std::vector<DWORD> pids;
    DWORD scan_error=0;
    std::array<DWORD,2> hotkey_errors{};
    bool topmost=false;
    bool english=false;
    std::wstring settings_path;
    UINT dpi=96;
    std::array<HFONT,5> fonts{};
    HWND ncz_button=nullptr, top_button=nullptr, refresh_button=nullptr;
    HWND tr_button=nullptr, en_button=nullptr;
    std::deque<LogEntry> events;
    std::wstring last_action=L"Hazır. Oyun süreci otomatik aranıyor.";
} state;
foundation::LocalPassPatch ncz;
const wchar_t* t(const wchar_t* tr,const wchar_t* en) { return state.english?en:tr; }
void load_language() {
    wchar_t local[MAX_PATH]{};
    DWORD size=GetEnvironmentVariableW(L"LOCALAPPDATA",local,MAX_PATH);
    if(!size||size>=MAX_PATH)return;
    std::filesystem::path directory=std::filesystem::path(local)/L"Veyra";
    std::error_code error;std::filesystem::create_directories(directory,error);
    if(error)return;
    state.settings_path=(directory/L"settings.ini").wstring();
    state.english=GetPrivateProfileIntW(L"UI",L"English",0,state.settings_path.c_str())==1;
}
int px(int v) { return MulDiv(v,static_cast<int>(state.dpi),96); }
RECT box(int x,int y,int w,int h) { return {px(x),px(y),px(x+w),px(y+h)}; }
void fonts() {
    for(auto f:state.fonts) if(f) DeleteObject(f);
    constexpr std::array<int,5> sizes{12,14,18,26,34};
    for(std::size_t i=0;i<sizes.size();++i)
        state.fonts[i]=CreateFontW(-px(sizes[i]),0,0,0,i>=2?FW_SEMIBOLD:FW_NORMAL,
            FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
}
void panel(HDC dc,RECT r,COLORREF fill,COLORREF stroke=border,int radius=18) {
    using namespace Gdiplus;
    Graphics graphics(dc);graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    const REAL x=static_cast<REAL>(r.left)+0.5f,y=static_cast<REAL>(r.top)+0.5f;
    const REAL w=static_cast<REAL>(r.right-r.left)-1.0f,h=static_cast<REAL>(r.bottom-r.top)-1.0f;
    const REAL d=(std::min)(static_cast<REAL>(px(radius)*2),(std::min)(w,h));
    if(w<=0||h<=0)return;
    GraphicsPath path;
    path.AddArc(x,y,d,d,180,90);path.AddArc(x+w-d,y,d,d,270,90);
    path.AddArc(x+w-d,y+h-d,d,d,0,90);path.AddArc(x,y+h-d,d,d,90,90);path.CloseFigure();
    SolidBrush brush(Color(255,GetRValue(fill),GetGValue(fill),GetBValue(fill)));
    Pen pen(Color(255,GetRValue(stroke),GetGValue(stroke),GetBValue(stroke)),1.0f);
    graphics.FillPath(&brush,&path);graphics.DrawPath(&pen,&path);
}
void label(HDC dc,const std::wstring& text,int x,int y,int w,int h,int font=1,
           COLORREF color=ink,UINT flags=DT_LEFT|DT_TOP|DT_WORDBREAK) {
    RECT r=box(x,y,w,h);auto old=SelectObject(dc,state.fonts[static_cast<std::size_t>(font)]);
    SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);
    DrawTextW(dc,text.c_str(),static_cast<int>(text.size()),&r,flags|DT_NOPREFIX);
    SelectObject(dc,old);
}
void event(const std::wstring& text,const std::wstring& english=L"") {
    SYSTEMTIME now{};GetLocalTime(&now);wchar_t stamp[16]{};
    swprintf_s(stamp,L"%02u:%02u:%02u",now.wHour,now.wMinute,now.wSecond);
    state.last_action=text;state.events.push_front({stamp,text,english.empty()?text:english});
    if(state.events.size()>30)state.events.pop_back();
}
void scan() {
    state.pids.clear();state.scan_error=0;
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(snapshot==INVALID_HANDLE_VALUE){state.scan_error=GetLastError();return;}
    PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);
    BOOL more=Process32FirstW(snapshot,&entry);
    while(more){if(foundation::is_game(entry.szExeFile))state.pids.push_back(entry.th32ProcessID);more=Process32NextW(snapshot,&entry);}
    DWORD error=GetLastError();if(error!=ERROR_NO_MORE_FILES)state.scan_error=error;CloseHandle(snapshot);
}
void controls() {
    SetWindowTextW(state.ncz_button,ncz.needs_restore()?t(L"Kapat ve geri yükle   ·   F6",L"Disable and restore   ·   F6"):t(L"Yerel geçişi aç   ·   F6",L"Enable local pass-through   ·   F6"));
    SetWindowTextW(state.top_button,state.topmost?t(L"Üstte: Açık · F8",L"Pinned: On · F8"):t(L"Üstte tut · F8",L"Pin window · F8"));
    SetWindowTextW(state.refresh_button,t(L"Yenile",L"Refresh"));
    SetWindowTextW(GetParent(state.ncz_button),t(L"Veyra | Yerel araçlar",L"Veyra | Local tools"));
    InvalidateRect(state.tr_button,nullptr,TRUE);InvalidateRect(state.en_button,nullptr,TRUE);
    InvalidateRect(state.ncz_button,nullptr,TRUE);InvalidateRect(state.top_button,nullptr,TRUE);
}
void layout(HWND window) {
    RECT r{};GetClientRect(window,&r);
    int w=MulDiv(r.right,96,static_cast<int>(state.dpi)),h=MulDiv(r.bottom,96,static_cast<int>(state.dpi));
    int cw=w-48;
    MoveWindow(state.tr_button,px(w-140),px(25),px(48),px(28),TRUE);
    MoveWindow(state.en_button,px(w-86),px(25),px(48),px(28),TRUE);
    MoveWindow(state.top_button,px(w-168),px(h-45),px(144),px(30),TRUE);
    MoveWindow(state.ncz_button,px(42),px(310),px(cw-36),px(38),TRUE);
    MoveWindow(state.refresh_button,px(w-115),px(391),px(72),px(27),TRUE);
}
void action(HWND window,int id) {
    if(id==tr_id||id==en_id) {
        state.english=id==en_id;
        if(!state.settings_path.empty()&&!WritePrivateProfileStringW(L"UI",L"English",state.english?L"1":L"0",state.settings_path.c_str()))
            event(L"Dil tercihi kaydedilemedi.",L"Language preference could not be saved.");
    } else if(id==ncz_id) {
        std::wstring message;
        if(ncz.needs_restore()) {
            if(ncz.restore(message))event(L"Yerel filtre denemesi kapatıldı; orijinal kod geri yüklendi.",L"Local filter disabled; original code restored.");else event(message);
        } else {
            scan();
            if(state.scan_error)event(L"Oyun taraması başarısız. Hata: "+std::to_wstring(state.scan_error),L"Game scan failed. Error: "+std::to_wstring(state.scan_error));
            else if(state.pids.size()!=1)event(L"İşlem yapılmadı. Tam olarak bir ETS2 süreci açık olmalı.",L"No change made. Exactly one ETS2 process must be running.");
            else if(ncz.enable(state.pids.front(),message))event(L"Yerel filtre denemesi açık. NCZ durumu yazılmıyor; geçiş doğrulanmadı.",L"Local filter enabled. Published NCZ state is not written.");
            else event(message);
        }
    } else if(id==top_id) {
        bool desired=!state.topmost;
        if(SetWindowPos(window,desired?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE)) {
            state.topmost=desired;event(desired?L"Panel her zaman üstte.":L"Panel normal pencere düzeninde.",desired?L"Window pinned on top.":L"Window unpinned.");
        } else event(L"Pencere konumu değiştirilemedi.",L"Could not change window position.");
    } else if(id==refresh_id){scan();event(L"Oyun durumu yeniden tarandı.",L"Game status refreshed.");}
    controls();InvalidateRect(window,nullptr,FALSE);
}
void render(HDC dc,int w,int h) {
    RECT full=box(0,0,w,h);auto brush=CreateSolidBrush(bg);FillRect(dc,&full,brush);DeleteObject(brush);
    int x=24,cw=w-48;
    static HICON mark=static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(101),IMAGE_ICON,64,64,LR_SHARED));
    if(mark)DrawIconEx(dc,px(x),px(22),mark,px(36),px(36),0,nullptr,DI_NORMAL);
    label(dc,L"Veyra",x+49,23,cw-200,34,3);
    label(dc,t(L"Sürüş araçların, daha sade.",L"Your driving tools, simplified."),x,76,cw-170,22,1,muted);
    int tile=(cw-10)/2;
    for(int i=0;i<2;++i)panel(dc,box(x+i*(tile+10),111,tile,62),card,border,14);
    label(dc,t(L"OYUN",L"GAME"),x+14,121,tile-25,16,0,muted);
    std::wstring game=state.scan_error?t(L"Tarama hatası",L"Scan error"):state.pids.empty()?t(L"Bekleniyor",L"Waiting"):state.pids.size()==1?t(L"ETS2 açık",L"ETS2 running"):t(L"Birden çok süreç",L"Multiple processes");
    label(dc,game,x+14,143,tile-25,24,2,state.pids.size()==1?accent:ink);
    label(dc,t(L"BAĞLANTI",L"CONNECTION"),x+tile+24,121,tile-25,16,0,muted);
    label(dc,state.pids.size()==1?L"PID "+std::to_wstring(state.pids.front()):t(L"Otomatik",L"Automatic"),x+tile+24,143,tile-25,24,2);
    panel(dc,box(x,188,cw,178),RGB(37,27,53),RGB(77,55,103),20);
    panel(dc,box(x+18,205,35,25),RGB(61,42,85),RGB(61,42,85),8);
    label(dc,L"F6",x+18,210,35,18,0,accent,DT_CENTER|DT_SINGLELINE);
    panel(dc,box(x+cw-98,205,80,25),RGB(46,34,63),RGB(65,46,85),12);
    label(dc,ncz.active()?t(L"AÇIK",L"ON"):t(L"KAPALI",L"OFF"),x+cw-98,210,80,18,0,ncz.active()?accent:muted,DT_CENTER|DT_SINGLELINE);
    label(dc,t(L"Yerel geçiş",L"Local pass-through"),x+18,242,cw-36,28,2);
    label(dc,t(L"NCZ durumunu zorlamayan deneysel temas filtresi.",L"Experimental contact filter without forcing NCZ status."),x+18,277,cw-36,28,1,muted);
    int logh=(std::max)(90,h-444);
    panel(dc,box(x,376,cw,logh),card,border,18);
    label(dc,t(L"Son işlemler",L"Recent activity"),x+18,394,cw-130,25,2);
    int y=433;
    for(const auto& line:state.events){
        if(y+27>376+logh-10)break;
        label(dc,line.stamp,x+18,y,72,19,0,accent);
        label(dc,state.english?line.en:line.tr,x+95,y,cw-113,29,0,muted);y+=34;
    }
    std::wstring keys;
    for(std::size_t i=0;i<state.hotkey_errors.size();++i)if(state.hotkey_errors[i]){
        if(!keys.empty())keys+=L", ";keys+=L"F"+std::to_wstring(i==0?6:8);
    }
    std::wstring footer=keys.empty()?t(L"Deneysel 0.9 · Kararlılık doğrulanmadı",L"Experimental 0.9 · Stability unverified"):keys+t(L" başka uygulamada kullanımda",L" already in use by another app");
    label(dc,footer,x,h-37,cw-165,24,0,muted);
}
void draw_button(const DRAWITEMSTRUCT& item) {
    const bool onWindow=item.CtlID==top_id||item.CtlID==tr_id||item.CtlID==en_id;
    const COLORREF surface=onWindow?bg:item.CtlID==ncz_id?RGB(37,27,53):card;
    auto background=CreateSolidBrush(surface);
    FillRect(item.hDC,&item.rcItem,background);DeleteObject(background);
    wchar_t text[120]{};GetWindowTextW(item.hwndItem,text,120);
    bool language=item.CtlID==tr_id||item.CtlID==en_id;
    bool selected=(item.CtlID==tr_id&&!state.english)||(item.CtlID==en_id&&state.english);
    bool primary=item.CtlID==ncz_id;bool down=(item.itemState&ODS_SELECTED)!=0;
    COLORREF fill=primary?(ncz.active()?RGB(216,185,255):accent):RGB(41,32,57);
    if(language)fill=selected?RGB(63,43,87):RGB(28,22,40);
    if(down)fill=primary?RGB(163,126,237):RGB(58,43,78);
    const bool focused=(item.itemState&ODS_FOCUS)&&!(item.itemState&ODS_NOFOCUSRECT);
    panel(item.hDC,item.rcItem,fill,focused?accent:primary?fill:border,18);
    auto old=SelectObject(item.hDC,state.fonts[0]);SetBkMode(item.hDC,TRANSPARENT);
    SetTextColor(item.hDC,primary?bg:selected?accent:ink);RECT r=item.rcItem;
    DrawTextW(item.hDC,text,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);SelectObject(item.hDC,old);
}
HWND button(HWND parent,int id,const wchar_t* name) {
    return CreateWindowExW(0,L"BUTTON",name,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,0,0,parent,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
}
LRESULT CALLBACK proc(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    switch(message) {
    case WM_CREATE: {
        load_language();state.dpi=GetDpiForWindow(window);fonts();
        state.ncz_button=button(window,ncz_id,L"Yerel geçişi dene   ·   F6");
        state.top_button=button(window,top_id,L"Üstte tut   ·   F8");
        state.refresh_button=button(window,refresh_id,L"Yenile");
        state.tr_button=button(window,tr_id,L"TR");state.en_button=button(window,en_id,L"EN");
        if(!state.tr_button||!state.en_button)return -1;
        if(!state.ncz_button||!state.top_button||!state.refresh_button)return -1;
        for(int i=0;i<2;++i){
            const int key=i==0?6:8;
            if(!RegisterHotKey(window,key,MOD_NOREPEAT,VK_F6+static_cast<UINT>(key-6)))state.hotkey_errors[static_cast<std::size_t>(i)]=GetLastError();
        }
        scan();event(L"Panel hazır. NCZ başlangıçta kapalı.",L"Panel ready. Local filter starts disabled.");
        if(!SetTimer(window,1,1000,nullptr))return -1;
        controls();BOOL dark=TRUE;DwmSetWindowAttribute(window,DWMWA_USE_IMMERSIVE_DARK_MODE,&dark,sizeof(dark));
        DWM_WINDOW_CORNER_PREFERENCE corners=DWMWCP_ROUND;
        DwmSetWindowAttribute(window,DWMWA_WINDOW_CORNER_PREFERENCE,&corners,sizeof(corners));
        COLORREF caption=side,text=ink;
        DwmSetWindowAttribute(window,DWMWA_CAPTION_COLOR,&caption,sizeof(caption));
        DwmSetWindowAttribute(window,DWMWA_TEXT_COLOR,&text,sizeof(text));
        layout(window);return 0;
    }
    case WM_SIZE:layout(window);InvalidateRect(window,nullptr,FALSE);return 0;
    case WM_DPICHANGED: {
        state.dpi=HIWORD(wp);fonts();auto r=reinterpret_cast<RECT*>(lp);
        SetWindowPos(window,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);
        layout(window);InvalidateRect(window,nullptr,FALSE);return 0;
    }
    case WM_TIMER:
        if(ncz.needs_restore()&&!ncz.alive()){std::wstring ignored;ncz.restore(ignored);event(L"Oyun kapandı; deneysel işlem artık etkin değil.",L"Game exited; the experimental change no longer exists.");}
        scan();controls();InvalidateRect(window,nullptr,FALSE);return 0;
    case WM_COMMAND:if(HIWORD(wp)==BN_CLICKED)action(window,LOWORD(wp));return 0;
    case WM_HOTKEY:if(wp==6||wp==8)action(window,static_cast<int>(wp)+100);return 0;
    case WM_DRAWITEM:draw_button(*reinterpret_cast<DRAWITEMSTRUCT*>(lp));return TRUE;
    case WM_ERASEBKGND:return 1;
    case WM_PAINT: {
        PAINTSTRUCT p{};HDC target=BeginPaint(window,&p);RECT r{};GetClientRect(window,&r);
        HDC dc=CreateCompatibleDC(target);HBITMAP bitmap=CreateCompatibleBitmap(target,(std::max)(r.right,1L),(std::max)(r.bottom,1L));
        if(dc&&bitmap){auto old=SelectObject(dc,bitmap);render(dc,MulDiv(r.right,96,static_cast<int>(state.dpi)),MulDiv(r.bottom,96,static_cast<int>(state.dpi)));BitBlt(target,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);SelectObject(dc,old);}
        if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);EndPaint(window,&p);return 0;
    }
    case WM_GETMINMAXINFO: {
        auto info=reinterpret_cast<MINMAXINFO*>(lp);info->ptMinTrackSize={px(740),px(570)};return 0;
    }
    case WM_CLOSE: {
        std::wstring result;
        if(!ncz.restore(result)){event(result);InvalidateRect(window,nullptr,FALSE);MessageBoxW(window,result.c_str(),t(L"Geri yükleme başarısız",L"Restore failed"),MB_ICONERROR);return 0;}
        DestroyWindow(window);return 0;
    }
    case WM_DESTROY:
        KillTimer(window,1);UnregisterHotKey(window,6);UnregisterHotKey(window,8);
        for(auto f:state.fonts)if(f)DeleteObject(f);state.fonts.fill(nullptr);PostQuitMessage(0);return 0;
    default:return DefWindowProcW(window,message,wp,lp);
    }
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int show) {
    Gdiplus::GdiplusStartupInput startupInput;ULONG_PTR token=0;
    if(Gdiplus::GdiplusStartup(&token,&startupInput,nullptr)!=Gdiplus::Ok)return 4;
    struct GraphicsLifetime { ULONG_PTR token; ~GraphicsLifetime(){Gdiplus::GdiplusShutdown(token);} } graphicsLifetime{token};
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    constexpr auto name=L"VeyraPanelV9";WNDCLASSW wc{};
    wc.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(101));
    wc.lpfnWndProc=proc;wc.hInstance=instance;wc.lpszClassName=name;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    if(!RegisterClassW(&wc))return 1;
    HWND window=CreateWindowExW(0,name,L"Veyra",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,780,610,nullptr,nullptr,instance,nullptr);
    if(!window)return 2;
    MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
    int width=px(780),height=px(610);
    if(GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor)) {
        width=(std::min)(width,static_cast<int>(monitor.rcWork.right-monitor.rcWork.left)-px(24));
        height=(std::min)(height,static_cast<int>(monitor.rcWork.bottom-monitor.rcWork.top)-px(24));
    }
    SetWindowPos(window,nullptr,0,0,width,height,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    ShowWindow(window,show);MSG message{};BOOL result=0;
    while((result=GetMessageW(&message,nullptr,0,0))>0)if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}
    return result==-1?3:static_cast<int>(message.wParam);
}
