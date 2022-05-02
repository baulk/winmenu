#ifndef BELA_ERROR_CODE_HPP
#define BELA_ERROR_CODE_HPP
#pragma once
#include <SDKDDKVer.h>
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <windows.h>
#endif
#include <string>
#include <system_error>
#include <compare>
#include <format>

namespace bela {
constexpr long ErrNone = 0;
constexpr long ErrEOF = ERROR_HANDLE_EOF;
constexpr long ErrGeneral = 0x4001;
constexpr long ErrSkipParse = 0x4002;
constexpr long ErrParseBroken = 0x4003;
constexpr long ErrFileTooSmall = 0x4004;
constexpr long ErrFileAlreadyOpened = 0x4005;
constexpr long ErrEnded = 654320;
constexpr long ErrCanceled = 654321;
constexpr long ErrUnimplemented = 654322; // feature not implemented
// bela::error_code is a platform-dependent error code
struct error_code {
  std::wstring message;
  long code{ErrNone};
  // -----------
  error_code() noexcept = default;
  error_code(std::wstring &&message_, long code_) noexcept {
    message = std::move(message_);
    code = code_;
  }
  error_code(std::wstring_view message_, long code_) noexcept {
    message = message_;
    code = code_;
  }
  error_code(const error_code &) = default;
  error_code &operator=(const error_code &) = default;
  error_code(error_code &&) = default;
  error_code &operator=(error_code &&) = default;
  error_code &assgin(error_code &&o) {
    *this = std::move(o);
    return *this;
  }
  void clear() {
    code = ErrNone;
    message.clear();
  }
  [[nodiscard]] const std::wstring &native() const { return message; }
  [[nodiscard]] const wchar_t *data() const { return message.data(); }
  [[nodiscard]] explicit operator bool() const noexcept { return code != ErrNone; }
  [[nodiscard]] friend bool operator==(const error_code &_Left, const error_code &_Right) noexcept {
    return _Left.code == _Right.code;
  }
  [[nodiscard]] friend bool operator==(const error_code &_Left, const long _Code) noexcept {
    return _Left.code == _Code;
  }
  [[nodiscard]] friend std::strong_ordering operator<=>(const error_code &_Left, long _Code) noexcept {
    return _Left.code <=> _Code;
  }
  [[nodiscard]] friend std::strong_ordering operator<=>(const error_code &_Left, const error_code &_Right) noexcept {
    return _Left.code <=> _Right.code;
  }
};

inline std::wstring resolve_system_error_message(DWORD ec, std::wstring_view prefix) {
  LPWSTR buf = nullptr;
  auto rl = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, ec,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buf, 0, nullptr);
  if (rl == 0) {
    return std::format(L"{}GetLastError={:#08x}", prefix, ec);
  }
  if (buf[rl - 1] == '\n') {
    rl--;
  }
  if (rl > 0 && buf[rl - 1] == '\r') {
    rl--;
  }
  auto msg = std::format(L"{}{}", prefix, std::wstring_view(buf, rl));
  LocalFree(buf);
  return msg;
}

// make_error_code_from_system convert from Windows error code convert to error_code
[[nodiscard]] inline error_code make_error_code_from_system(DWORD e, std::wstring_view prefix) {
  return error_code{resolve_system_error_message(e, prefix), static_cast<long>(e)};
}
// make_system_error_code call GetLastError get error code convert to error_code
[[nodiscard]] inline error_code make_system_error_code(std::wstring_view prefix = L"") {
  return make_error_code_from_system(GetLastError(), prefix);
}

template <class F> class final_act {
public:
  explicit final_act(F f) noexcept : f_(std::move(f)), invoke_(true) {}

  final_act(final_act &&other) noexcept : f_(std::move(other.f_)), invoke_(std::exchange(other.invoke_, false)) {}

  final_act(const final_act &) = delete;
  final_act &operator=(const final_act &) = delete;
  ~final_act() noexcept {
    if (invoke_) {
      f_();
    }
  }

private:
  F f_;
  bool invoke_{true};
};

// finally() - convenience function to generate a final_act
template <class F> inline final_act<F> finally(const F &f) noexcept { return final_act<F>(f); }

template <class F> inline final_act<F> finally(F &&f) noexcept { return final_act<F>(std::forward<F>(f)); }

} // namespace bela

#endif