// Supplier.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <windows.h>
#include <string> 
#include <string.h> 
#include <list>
#define SERVICE
#include "../api.h"
#include "../plugin_id.h"

typedef struct _DllListElement
{
	GUID pluginId;
	const wchar_t* DllName;
	pfn_GetCopyright GetCopyright;
} DllListElement;

std::list<DllListElement> g_DllList = {
	{ plugin1_id, L"Plugin1_2.dll", nullptr },
	{ plugin2_id, L"Pugin2.dll", nullptr }
};


int main() {
	/*
	Convertion GUID to string to pass through register
	*/
	wchar_t* string;
	IID guid;
	wchar_t register_value[256] = L"\0";

	for (DllListElement element : g_DllList) {
		StringFromIID(element.pluginId, &string);
		printf("GUID is %S \n", string);


		const auto res2 = IIDFromString(string, &guid);
		if (res2 != S_OK) {
			printf("Enable to convert %S to GUID \n", string);
			return NULL;
		}

		wcscat_s(register_value, 256, element.DllName);
		LPCTSTR value = (LPCTSTR)string;

		/*
		Open registry or create if not exist
		*/
		HKEY hKey;
		long lRes = RegCreateKeyEx(
			HKEY_CURRENT_USER,
			L"SOFTWARE\\PluginSystem",
			NULL,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&hKey,
			NULL
		);

		if (lRes != ERROR_SUCCESS) {
			printf("ERROR %d %lu\n", lRes, GetLastError());
			return -1;
		}

		/*
		Write data to registry
		*/
		long result = RegSetValueEx(
			hKey,
			value,
			0,
			REG_SZ,
			(LPBYTE)register_value,
			wcslen(register_value) * sizeof(wchar_t)
		);

		if (result != ERROR_SUCCESS) {
			printf("ERROR %d\n", result);
			return -1;
		}
	}

	return 0;
}
