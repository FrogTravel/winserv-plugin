// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "..\api.h"
#include "..\plugin_id.h"

int main(int argc, char* argv[])
{
	printf("Hello, world! (%s) \n", argv[0]);

	if (argc != 3) {
		printf("usage: %s server-name GUID \n", argv[0]);
		return 1;
	}

#if 0
	//simple test to check GUID string representation
	wchar_t* string;
	StringFromIID(plugin1_id, &string);
	printf("GUID is %S \n", string);
	//{00000000-0000-0000-0100-000000000000}
#endif

	#define MAX_CHARS_FOR_GUID 50
	wchar_t guidString[MAX_CHARS_FOR_GUID];
	IID guid;

	const auto res = MultiByteToWideChar(CP_ACP, 0, argv[2], -1, guidString, sizeof(guidString));
	if (res == 0) {
		printf("Enable to convert %s to Unicode \n", argv[2]);
		return 1;
	}

	const auto res2 = IIDFromString(guidString, &guid);
	if (res2 != S_OK) {
		printf("Enable to convert %S to GUID \n", guidString);
		return 1;
	}

	if (!SetServerEndpoint(argv[1])) {
		printf("Failed to set server endpoint %s \n", argv[1]);
		return 1;
	}

	printf("Copyright = %s \n", GetCopyright(guid));

	while (true);
	return 0;
}

