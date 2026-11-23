#include "common.h"
#include "Log.h"

#include <cstdio>
#include <cstdarg>

namespace
{
	FILE *g_logFile = nullptr;
}

namespace Log
{
	void Init(const std::string &gameRoot)
	{
		std::string path = gameRoot + "\\UnleashedLoader.log";
		g_logFile = fopen(path.c_str(), "w");
	}

	void Write(const char *fmt, ...)
	{
		if (!g_logFile)
			return;

		va_list args;
		va_start(args, fmt);
		vfprintf(g_logFile, fmt, args);
		va_end(args);

		fputc('\n', g_logFile);
		fflush(g_logFile);
	}
}
