///
#include <shlwapi.h>
#include <shobjidl_core.h>
#include <Shobjidl.h>
#include <ShlObj.h>
#include <wrl.h>
#include <wrl/module.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <wrl/module.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Resources.Core.h>
#include <filesystem>
#include <bela.hpp>

constexpr auto ok = ERROR_SUCCESS;

inline std::optional<std::wstring> GitForWindowsInstallPath(bela::error_code &ec) {
  HKEY hkey = nullptr;
  if (RegOpenKeyW(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\GitForWindows)", &hkey) != ok) {
    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\WOW6432Node\GitForWindows)", &hkey) != ok) {
      if (RegOpenKeyW(HKEY_CURRENT_USER, LR"(SOFTWARE\GitForWindows)", &hkey) != ok) {
        if (RegOpenKeyW(HKEY_CURRENT_USER, LR"(SOFTWARE\WOW6432Node\GitForWindows)", &hkey) != ok) {
          ec = bela::make_system_error_code();
          return std::nullopt;
        }
      }
    }
  }
  auto closer = bela::finally([&] { RegCloseKey(hkey); });
  wchar_t buffer[4096];
  DWORD type = 0;
  DWORD bufsize = sizeof(buffer);
  if (RegQueryValueExW(hkey, L"InstallPath", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufsize) != ok) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (type != REG_SZ) {
    ec = bela::error_code(std::format(L"InstallPath not REG_SZ: {}", type), bela::ErrGeneral);
    return std::nullopt;
  }
  return std::make_optional<std::wstring>(buffer);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
  if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
  }
  return TRUE;
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
inline HINSTANCE GetModuleInstanceHandle() { return reinterpret_cast<HINSTANCE>(&__ImageBase); }

std::optional<std::filesystem::path> LookupGitBashExe(bela::error_code &ec) {
  auto installPath_ = GitForWindowsInstallPath(ec);
  if (!installPath_) {
    return std::nullopt;
  }
  std::filesystem::path installPath(*installPath_);
  auto gitBashExe = installPath / L"git-bash.exe";
  std::error_code e;
  if (std::filesystem::exists(gitBashExe, e)) {
    return std::make_optional(std::move(gitBashExe));
  }
  return std::nullopt;
}

class __declspec(uuid("C6475E81-139F-4FD9-B758-20B68BA7F60C")) OpenGitBashHere
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom | Microsoft::WRL::InhibitFtmBase>,
          IExplorerCommand> {
public:
  // IExplorerCommand
  IFACEMETHODIMP GetTitle(_In_opt_ IShellItemArray *items, _Outptr_result_nullonfailure_ PWSTR *ppszTitle) {
    return SHStrDup(L"Git Bash Here", ppszTitle);
  }
  IFACEMETHODIMP GetIcon(_In_opt_ IShellItemArray *, _Outptr_result_nullonfailure_ PWSTR *ppszIcon) {
    bela::error_code ec;
    auto gitBashExe = LookupGitBashExe(ec);
    if (!gitBashExe) {
      return 0;
    }
    return SHStrDupW(gitBashExe->c_str(), ppszIcon);
  }

  IFACEMETHODIMP GetToolTip(_In_opt_ IShellItemArray *, _Outptr_result_nullonfailure_ PWSTR *ppszInfoTip) {
    *ppszInfoTip = nullptr;
    return E_NOTIMPL;
  }

  IFACEMETHODIMP GetCanonicalName(_Out_ GUID *guidCommandName) {
    *guidCommandName = GUID_NULL;
    return S_OK;
  }

  HRESULT GetState(IShellItemArray * /*psiItemArray*/, BOOL /*fOkToBeSlow*/, EXPCMDSTATE *pCmdState) {
    // compute the visibility of the verb here, respect "fOkToBeSlow" if this is
    // slow (does IO for example) when called with fOkToBeSlow == FALSE return
    // E_PENDING and this object will be called back on a background thread with
    // fOkToBeSlow == TRUE

    // We however don't need to bother with any of that, so we'll just return
    // ECS_ENABLED.

    *pCmdState = ECS_ENABLED;
    return S_OK;
  }

  IFACEMETHODIMP Invoke(_In_opt_ IShellItemArray *psiItemArray, _In_opt_ IBindCtx *) noexcept {
    std::wstring path;
    if (psiItemArray == nullptr) {
      // get the current path from explorer.exe
      path = this->_GetPathFromExplorer();
      // no go, unable to get a reasonable path
      if (path.empty()) {
        return S_FALSE;
      }
    } else {
      DWORD count;
      psiItemArray->GetCount(&count);
      winrt::com_ptr<IShellItem> psi;
      if (!SUCCEEDED(psiItemArray->GetItemAt(0, psi.put()))) {
        return S_FALSE;
      }
      LPWSTR pszName;
      if (!SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszName))) {
        return S_FALSE;
      }
      path = pszName;
      CoTaskMemFree(pszName);
    }
    bela::error_code ec;
    auto gitBaseExe = LookupGitBashExe(ec);
    if (!gitBaseExe) {
      return S_FALSE;
    }
    bela::EscapeArgv ea;
    ea.Assign(gitBaseExe->native()).Append(L"--cd=" + path);
    PROCESS_INFORMATION pi;
    STARTUPINFOEXW siEx{0};
    siEx.StartupInfo.cb = sizeof(STARTUPINFOEX);

    if (CreateProcessW(gitBaseExe->c_str(), // lpApplicationName
                       ea.data(),
                       nullptr,                                                   // lpProcessAttributes
                       nullptr,                                                   // lpThreadAttributes
                       false,                                                     // bInheritHandles
                       EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT, // dwCreationFlags
                       nullptr,                                                   // lpEnvironment
                       path.data(),
                       &siEx.StartupInfo, // lpStartupInfo
                       &pi                // lpProcessInformation
                       ) != TRUE) {
      return S_FALSE;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return S_OK;
  }

  HRESULT GetFlags(EXPCMDFLAGS *pFlags) {
    *pFlags = ECF_DEFAULT;
    return S_OK;
  }

  HRESULT EnumSubCommands(IEnumExplorerCommand **ppEnum) {
    *ppEnum = nullptr;
    return E_NOTIMPL;
  }

protected:
  Microsoft::WRL::ComPtr<IUnknown> m_site;

private:
  std::wstring _GetPathFromExplorer() const;
};

std::wstring OpenGitBashHere::_GetPathFromExplorer() const {
  using namespace winrt;

  std::wstring path;
  HRESULT hr = NOERROR;

  auto hwnd = ::GetForegroundWindow();
  if (hwnd == nullptr) {
    return path;
  }

  WCHAR szName[MAX_PATH] = {0};
  ::GetClassNameW(hwnd, szName, MAX_PATH);
  if (0 == StrCmp(szName, L"WorkerW") || 0 == StrCmp(szName, L"Progman")) {
    // special folder: desktop
    hr = ::SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, szName);
    if (FAILED(hr)) {
      return path;
    }

    path = szName;
    return path;
  }

  if (0 != StrCmpW(szName, L"CabinetWClass")) {
    return path;
  }

  com_ptr<IShellWindows> shell;
  try {
    shell = create_instance<IShellWindows>(CLSID_ShellWindows, CLSCTX_ALL);
  } catch (...) {
    // look like try_create_instance is not available no more
  }

  if (shell == nullptr) {
    return path;
  }

  com_ptr<IDispatch> disp;
  VARIANT variant;
  VariantInit(&variant);
  variant.vt = VT_I4;
  auto closer = bela::finally([&] { VariantClear(&variant); });

  com_ptr<IWebBrowserApp> browser;
  // look for correct explorer window
  for (variant.intVal = 0; shell->Item(variant, disp.put()) == S_OK; variant.intVal++) {
    com_ptr<IWebBrowserApp> tmp;
    if (FAILED(disp->QueryInterface(tmp.put()))) {
      disp = nullptr; // get rid of DEBUG non-nullptr warning
      continue;
    }

    HWND tmpHWND = NULL;
    hr = tmp->get_HWND(reinterpret_cast<SHANDLE_PTR *>(&tmpHWND));
    if (hwnd == tmpHWND) {
      browser = tmp;
      disp = nullptr; // get rid of DEBUG non-nullptr warning
      break;          // found
    }

    disp = nullptr; // get rid of DEBUG non-nullptr warning
  }

  if (browser != nullptr) {
    BSTR url = nullptr;
    hr = browser->get_LocationURL(&url);
    if (FAILED(hr)) {
      return path;
    }

    std::wstring sUrl(url, SysStringLen(url));
    SysFreeString(url);
    DWORD size = MAX_PATH;
    hr = ::PathCreateFromUrlW(sUrl.c_str(), szName, &size, NULL);
    if (SUCCEEDED(hr)) {
      path = szName;
    }
  }

  return path;
}

CoCreatableClass(OpenGitBashHere) CoCreatableClassWrlCreatorMapInclude(OpenGitBashHere); //

STDAPI DllGetActivationFactory(_In_ HSTRING activatableClassId, _COM_Outptr_ IActivationFactory **factory) {
  return Microsoft::WRL::Module<Microsoft::WRL::ModuleType::InProc>::GetModule().GetActivationFactory(
      activatableClassId, factory);
}

STDAPI DllCanUnloadNow() {
  return Microsoft::WRL::Module<Microsoft::WRL::InProc>::GetModule().GetObjectCount() == 0 ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _COM_Outptr_ void **instance) {
  return Microsoft::WRL::Module<Microsoft::WRL::InProc>::GetModule().GetClassObject(rclsid, riid, instance);
}
