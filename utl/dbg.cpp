#include "dbg.h"
#include "algo.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace utl {

void AssertFailed(char const *message, char const *file, unsigned line)
{
	std::string msg = std::format("%s(%u): Assert failed: %s", file, line, message);
	LogLine(std::cerr, "%s", msg);

#if defined(_WIN32)
	int res = MessageBoxA(nullptr, msg.c_str(), "Assert failed", MB_ABORTRETRYIGNORE | MB_DEFBUTTON2 | MB_ICONEXCLAMATION | MB_TASKMODAL | MB_SETFOREGROUND);
	switch (res) {
		case IDABORT:
			std::abort();
			break;
		case IDRETRY:
			DebugBreak();
			break;
		case IDIGNORE:
			break;
	}
#endif
}

}