#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <filesystem>
#endif
namespace Depthorator::Licensing {
inline constexpr std::uint64_t kExpectedLicenseHash=0xfe974096bdc33915ULL;
inline std::uint64_t fnv1a64(const std::string& text) noexcept {std::uint64_t h=14695981039346656037ULL;for(const unsigned char c:text){h^=static_cast<std::uint64_t>(c);h*=1099511628211ULL;}return h;}
inline bool isLicensed() noexcept {
#if defined(_WIN32)
 HMODULE m=nullptr;const auto a=reinterpret_cast<LPCWSTR>(&isLicensed);
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,a,&m)||!m)return false;
 wchar_t path[32768]{};constexpr DWORD cap=static_cast<DWORD>(sizeof(path)/sizeof(path[0]));const DWORD len=GetModuleFileNameW(m,path,cap);if(len==0||len>=cap)return false;
 try{const std::filesystem::path p(path);std::ifstream f(p.parent_path().parent_path()/L"Resources"/L"125A_Depthorator.license",std::ios::binary);if(!f)return false;std::string v;std::getline(f,v);if(!v.empty()&&v.back()=='\r')v.pop_back();return fnv1a64(v)==kExpectedLicenseHash;}catch(...){return false;}
#else
 return false;
#endif
}
}
