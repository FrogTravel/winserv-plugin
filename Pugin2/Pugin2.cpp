// Pugin2.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

extern "C" __declspec(dllexport) const char* __cdecl GetCopyright()
{
	return "Plugin 2 {00000000-0000-0000-0200-000000000000} Copyright";
}
