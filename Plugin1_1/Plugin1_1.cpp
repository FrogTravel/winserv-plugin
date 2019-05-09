// Plugin1_1.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

extern "C" __declspec(dllexport) const char* __cdecl GetCopyright()
{
	return "Plugin 1_1 {00000000-0000-0000-0100-000000000000} Copyright";
}
