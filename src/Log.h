#pragma once

#include <string>

// simple logger
namespace Log
{
	void Init(const std::string &gameRoot);
	void Write(const char *fmt, ...);
}
