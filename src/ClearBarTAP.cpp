#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// windows.h defines GetCurrentTime() as a macro; undefine it before
// including XAML SDK headers that declare a GetCurrentTime member.
#ifdef GetCurrentTime
#undef GetCurrentTime
#endif
#include <oleauto.h>
#include <ocidl.h>
#include <inspectable.h>
#include <Windows.UI.Xaml.h>
#include <xamlom.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string_view>
#include <utility>
#include "common.h"
#include "TaskbarAppearance.h"

namespace wux=winrt::Windows::UI::Xaml;
namespace wuxm=winrt::Windows::UI::Xaml::Media;
namespace wuxc=winrt::Windows::UI::Xaml::Controls;
namespace wuxs=winrt::Windows::UI::Xaml::Shapes;
namespace wf=winrt::Windows::Foundation;

namespace {
HMODULE tapModule{};

void LogTap(std::wstring_view message) noexcept {
    try {
        std::wofstream file(std::filesystem::path(ModuleDirectory(tapModule)) / L"clearbar.log", std::ios::app);
        SYSTEMTIME time{};
        GetLocalTime(&time);
        file << L"[" << time.wHour << L":" << time.wMinute << L":" << time.wSecond
             << L"] TAP: " << message << L"\n";
    } catch (...) {}
}

void WriteState(TaskbarMode mode) noexcept {
    try {
        std::wofstream file(std::filesystem::path(ModuleDirectory(tapModule)) / L"clearbar.state", std::ios::trunc);
        file << GetCurrentProcessId() << L" " << Preset(mode).name << L"\n";
    } catch (...) { LogTap(L"Could not write applied state"); }
}

struct BStr {
    BSTR p{};
    explicit BStr(BSTR v=nullptr):p(v){}
    ~BStr(){ if(p) SysFreeString(p); }
    std::wstring_view view() const { return p?std::wstring_view(p,SysStringLen(p)):std::wstring_view(); }
};

struct Watcher:winrt::implements<Watcher,IVisualTreeServiceCallback2,winrt::non_agile>{
    struct Element {
        winrt::weak_ref<wux::UIElement> target;
        wuxm::Brush originalBrush{nullptr};
        double originalOpacity{};
        bool fill{};
        bool brushWasLocal{};
        bool opacityWasLocal{};
    };

    winrt::com_ptr<IXamlDiagnostics> diag;
    winrt::com_ptr<IVisualTreeService3> svc;
    std::vector<Element> elements;
    wux::DispatcherTimer timer{nullptr};
    winrt::event_token tickToken{};
    TaskbarMode mode{};
    bool advised{};

    explicit Watcher(winrt::com_ptr<IXamlDiagnostics> d):diag(std::move(d)),mode(LoadMode(tapModule).mode){
        svc=diag.as<IVisualTreeService3>();
        if (!LoadMode(tapModule).valid) LogTap(L"Missing or invalid configuration; using safe fallback");
    }

    HRESULT Start() noexcept {
        try {
            if (!svc) return E_NOINTERFACE;
            // Register on the same COM/XAML thread that supplied IXamlDiagnostics.
            // Calling this from a detached, uninitialized worker thread can prevent
            // callbacks and leave the installer waiting forever for an applied state.
            const HRESULT hr=svc->AdviseVisualTreeChange(this);
            advised=SUCCEEDED(hr);
            if (FAILED(hr)) LogTap(L"Visual tree subscription failed: " + std::to_wstring(static_cast<unsigned long>(hr)));
            return hr;
        } catch (...) {
            const HRESULT hr=winrt::to_hresult();
            LogTap(L"Visual tree subscription raised an exception");
            return hr;
        }
    }

    void Stop() noexcept {
        try {
            if (timer) { timer.Stop(); timer.Tick(tickToken); timer=nullptr; }
        } catch (...) { LogTap(L"Could not stop appearance timer cleanly"); }
        if(svc && advised) {
            try { svc->UnadviseVisualTreeChange(this); }
            catch (...) { LogTap(L"Could not unregister visual tree callback cleanly"); }
        }
        advised=false;
        svc=nullptr; diag=nullptr;
    }

    void StartTimer() {
        if (timer) return;
        timer=wux::DispatcherTimer{};
        timer.Interval(std::chrono::seconds(1));
        tickToken=timer.Tick([weak=get_weak()](auto&&, auto&&) {
            if (auto self=weak.get()) self->Refresh();
        });
        timer.Start();
    }

    void SetBrush(wux::UIElement target, wuxm::Brush brush) {
        if (auto shape=target.try_as<wuxs::Shape>()) shape.Fill(brush);
        else if (auto border=target.try_as<wuxc::Border>()) border.Background(brush);
        else if (auto panel=target.try_as<wuxc::Panel>()) panel.Background(brush);
    }

    wuxm::Brush GetBrush(wux::UIElement target) {
        if (auto shape=target.try_as<wuxs::Shape>()) return shape.Fill();
        if (auto border=target.try_as<wuxc::Border>()) return border.Background();
        if (auto panel=target.try_as<wuxc::Panel>()) return panel.Background();
        return nullptr;
    }

    bool CanSetBrush(wux::UIElement target) {
        return target.try_as<wuxs::Shape>() || target.try_as<wuxc::Border>() || target.try_as<wuxc::Panel>();
    }

    wux::DependencyProperty BrushProperty(wux::UIElement target) {
        if (target.try_as<wuxs::Shape>()) return wuxs::Shape::FillProperty();
        if (target.try_as<wuxc::Border>()) return wuxc::Border::BackgroundProperty();
        return wuxc::Panel::BackgroundProperty();
    }

    bool HasLocalValue(wux::DependencyObject target, wux::DependencyProperty property) {
        return target.ReadLocalValue(property)!=wux::DependencyProperty::UnsetValue();
    }

    void Apply(Element& element) {
        auto target=element.target.get();
        if (!target) return;
        if (mode==TaskbarMode::Default) {
            if (element.fill) {
                if (element.brushWasLocal) SetBrush(target, element.originalBrush);
                else target.ClearValue(BrushProperty(target));
            }
            if (element.opacityWasLocal) target.Opacity(element.originalOpacity);
            else target.ClearValue(wux::UIElement::OpacityProperty());
        } else if (!element.fill || mode==TaskbarMode::Clear) {
            // Clear follows v2.2.2 exactly: hide both background elements.
            target.Opacity(0.0);
        } else {
            const auto& preset=Preset(mode);
            wuxm::AcrylicBrush brush;
            // Backdrop samples behind the taskbar's transparent XAML window.
            brush.BackgroundSource(wuxm::AcrylicBackgroundSource::Backdrop);
            brush.Opacity(preset.brushOpacity);
            brush.TintColor(winrt::Windows::UI::Color{255, AcrylicTintChannel, AcrylicTintChannel, AcrylicTintChannel});
            brush.TintOpacity(preset.tintOpacity);
            brush.TintLuminosityOpacity(preset.luminosityOpacity);
            brush.FallbackColor(winrt::Windows::UI::Color{preset.fallbackAlpha,
                AcrylicTintChannel, AcrylicTintChannel, AcrylicTintChannel});
            SetBrush(target, brush);
            target.Opacity(element.originalOpacity);
        }
        if (element.fill) WriteState(mode);
    }

    void Refresh() {
        const auto loaded=LoadMode(tapModule);
        if (loaded.mode==mode) return;
        mode=loaded.mode;
        for (auto it=elements.begin(); it!=elements.end();) {
            if (!it->target.get()) it=elements.erase(it);
            else {
                try { Apply(*it); }
                catch (...) { LogTap(L"Could not apply appearance to a taskbar element"); }
                ++it;
            }
        }
        LogTap(L"Appearance changed to " + std::wstring(Preset(mode).name));
        if (!loaded.valid) LogTap(L"Missing or invalid configuration; using safe fallback");
    }

    template<class T> T From(InstanceHandle h){
        wf::IInspectable o{nullptr};
        winrt::check_hresult(diag->GetIInspectableFromHandle(
            h,reinterpret_cast<::IInspectable**>(winrt::put_abi(o))));
        return o.as<T>();
    }

    bool InTaskbar(wux::DependencyObject n){
        for(int i=0;n&&i<32;i++){
            if(auto e=n.try_as<wux::FrameworkElement>()){
                if(e.Name()==L"TaskbarFrame" || winrt::get_class_name(e)==L"Taskbar.TaskbarFrame") return true;
            }
            n=wuxm::VisualTreeHelper::GetParent(n);
        }
        return false;
    }

    HRESULT STDMETHODCALLTYPE OnVisualTreeChange(ParentChildRelation r,VisualElement e,VisualMutationType m) noexcept override {
        BStr f(e.SrcInfo.FileName),h(e.SrcInfo.Hash),n(e.Name),t(e.Type);
        try{
            if(m!=Add) return S_OK;
            auto name=n.view();
            if(name!=L"BackgroundFill" && name!=L"BackgroundStroke") return S_OK;
            if(!r.Parent) return S_OK;
            auto parent=From<wux::DependencyObject>(r.Parent);
            if(!InTaskbar(parent)) return S_OK;
            auto target=From<wux::UIElement>(e.Handle);
            const bool fill=name==L"BackgroundFill";
            if (fill && !CanSetBrush(target)) {
                LogTap(L"BackgroundFill has an unsupported XAML type");
                return E_NOINTERFACE;
            }
            StartTimer();
            elements.push_back({winrt::make_weak(target), fill?GetBrush(target):nullptr,
                target.Opacity(), fill, fill && HasLocalValue(target, BrushProperty(target)),
                HasLocalValue(target, wux::UIElement::OpacityProperty())});
            Apply(elements.back());
            LogTap(fill?L"BackgroundFill appearance applied":L"BackgroundStroke appearance applied");
            return S_OK;
        }catch(...){ LogTap(L"Failed to modify taskbar XAML element"); return winrt::to_hresult(); }
    }

    HRESULT STDMETHODCALLTYPE OnElementStateChanged(InstanceHandle,VisualElementState,LPCWSTR) noexcept override {
        return S_OK;
    }
};

struct Tap:winrt::implements<Tap,IObjectWithSite,winrt::non_agile>{
    winrt::com_ptr<IUnknown> site;
    winrt::com_ptr<Watcher> watcher;

    HRESULT STDMETHODCALLTYPE SetSite(IUnknown* s) noexcept override{
        try{
            if(watcher){ watcher->Stop(); watcher=nullptr; }
            site=nullptr;
            if(s){
                site.copy_from(s);
                watcher=winrt::make_self<Watcher>(site.as<IXamlDiagnostics>());
                const HRESULT hr=watcher->Start();
                if(FAILED(hr)) { watcher->Stop(); watcher=nullptr; return hr; }
            }
            return S_OK;
        }catch(...){ return winrt::to_hresult(); }
    }

    HRESULT STDMETHODCALLTYPE GetSite(REFIID iid,void** out) noexcept override{
        if(!out) return E_POINTER;
        *out=nullptr;
        return site?site->QueryInterface(iid,out):E_FAIL;
    }
};

template<class T>
struct Factory:winrt::implements<Factory<T>,IClassFactory,winrt::non_agile>{
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer,REFIID iid,void** out) noexcept override{
        if(!out) return E_POINTER;
        *out=nullptr;
        if(outer) return CLASS_E_NOAGGREGATION;
        try{
            auto object = winrt::make_self<T>();
            return object->QueryInterface(iid, out);
        }
        catch(...){ return winrt::to_hresult(); }
    }
    HRESULT STDMETHODCALLTYPE LockServer(BOOL) noexcept override{ return S_OK; }
};
}

STDAPI DllGetClassObject(REFCLSID c, REFIID i, LPVOID* o)
{
    if(!o) return E_POINTER;
    *o = nullptr;

    if(!IsEqualCLSID(c, CLSID_ClearBarTAP))
        return CLASS_E_CLASSNOTAVAILABLE;

    try
    {
        auto factory = winrt::make_self<Factory<Tap>>();
        return factory->QueryInterface(i, o);
    }
    catch(...)
    {
        return winrt::to_hresult();
    }
}

STDAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason==DLL_PROCESS_ATTACH) tapModule=instance;
    return TRUE;
}
