#pragma once

#ifdef SERVICE
  #define DECLARE_PREFIX typedef
  #define DECLARE_API(conv, name) \
	(conv *pfn_##name)
  #define DEFINE_API(name) pfn_##name name;
#endif

#ifndef SERVICE
  extern "C" __declspec(dllimport) const char* __cdecl GetCopyright(GUID pluginId);
  extern "C" __declspec(dllimport) bool __cdecl SetServerEndpoint(char* ip);
#else
  //typedef const char* (__cdecl *pfn_GetCopyright)();
  DECLARE_PREFIX
  const char*
  DECLARE_API(__cdecl, GetCopyright)();

  //don't forget to add in .cpp DEFINE_API(GetCopyright)
#endif