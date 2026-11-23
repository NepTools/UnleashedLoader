#pragma once

#include <string>
#include <vector>
#include <utility>

// simple ini handler
class IniFile
{
public:
	bool Load(const std::string &path);
	std::vector<std::pair<std::string, std::string>> GetSection(const std::string &section) const;

private:
	struct Section
	{
		std::string name;
		std::vector<std::pair<std::string, std::string>> entries;
	};

	std::vector<Section> m_sections;
};
