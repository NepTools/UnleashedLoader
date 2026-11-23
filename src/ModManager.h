#pragma once

#include "FileMap.h"

namespace ModManager
{
	void Init();
	const FileMap &GetFileMap();
	const std::string &GetGameRoot();
}
