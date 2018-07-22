
#include "lib7zip.h"

#if !defined(_WIN32) && !defined(_OS2)
#include "CPP/myWindows/StdAfx.h"
#include "CPP/include_windows/windows.h"
#endif

#include "C/7zVersion.h"
#include "CPP/Common/Common.h"
#include "CPP/7zip/Archive/IArchive.h"
#include "CPP/Windows/PropVariant.h"
#include "CPP/Common/MyCom.h"
#include "CPP/7zip/ICoder.h"
#include "CPP/7zip/IPassword.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
using namespace NWindows;

#ifdef __linux__
#include <unistd.h> // readlink
#include <limits.h> // realpath
#endif // __linux__

#ifdef __APPLE__
#include <mach-o/dyld.h> // _NSGetExecutablePath
#include <sys/param.h> // realpath
#endif

#ifdef _WIN32
const char *kSeparator = "\\";
#else // _WIN32
const char *kSeparator = "/";
#endif // !_WIN32

#include "HelperFuncs.h" // WidenString
#include "SelfPath.h"

std::string SelfPath()
{
  const size_t file_name_characters = 16 * 1024;
#if defined(_WIN32)
  wchar_t *file_name = new wchar_t[file_name_characters];
  GetModuleFileNameW(NULL, file_name, static_cast<DWORD>(file_name_characters));
  auto self_path = NarrowString(file_name);
  delete[] file_name;
#elif defined(__APPLE__)
  // style: have to use uint32_t for _NSGetExecutablePath
  uint32_t mac_file_name_characters = file_name_characters;
  char *utf8_file_name = new char[file_name_characters];
  if (_NSGetExecutablePath(utf8_file_name, &mac_file_name_characters) != 0) {
    return "";
  }
  char *utf8_absolute_file_name = realpath(utf8_file_name, nullptr);
  delete[] utf8_file_name;
  auto self_path = std::string(utf8_absolute_file_name);
  free(utf8_absolute_file_name);
#else // !_WIN32 && !__APPLE__
  char *utf8_file_name = new char[file_name_characters];
  ssize_t dest_num_characters = readlink("/proc/self/exe", utf8_file_name, file_name_characters);
  // readlink does not append a null character
  utf8_file_name[dest_num_characters] = '\0';
  char *utf8_absolute_file_name = realpath(utf8_file_name, nullptr);
  delete[] utf8_file_name;
  auto self_path = std::string(utf8_absolute_file_name);
  free(utf8_absolute_file_name);
#endif // !__APPLE_

  return self_path;
}

std::string DirName(const std::string path)
{
  size_t slash_index = path.find_last_of(kSeparator);
  return path.substr(0, slash_index);
}
