#include "IniFile.h"

#include <cstdio>
#include <cctype>

namespace
{
	std::string Trim(const std::string &s)
	{
		size_t start = s.find_first_not_of(" \t\r\n");
		if (start == std::string::npos)
			return "";
		size_t end = s.find_last_not_of(" \t\r\n");
		return s.substr(start, end - start + 1);
	}

	bool EqualsIgnoreCase(const std::string &a, const std::string &b)
	{
		if (a.size() != b.size())
			return false;
		for (size_t i = 0; i < a.size(); ++i)
			if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
				return false;
		return true;
	}
}

bool IniFile::Load(const std::string &path)
{
	FILE *f = fopen(path.c_str(), "r");
	if (!f)
		return false;

	m_sections.clear();
	Section *current = nullptr;

	char lineBuf[1024];
	while (fgets(lineBuf, sizeof(lineBuf), f))
	{
		std::string line = Trim(lineBuf);

		if (line.empty() || line[0] == ';' || line[0] == '#')
			continue;

		if (line.front() == '[' && line.back() == ']')
		{
			Section s;
			s.name = Trim(line.substr(1, line.size() - 2));
			m_sections.push_back(s);
			current = &m_sections.back();
			continue;
		}

		if (!current)
			continue;

		size_t eq = line.find('=');
		if (eq == std::string::npos)
			continue;

		std::string key = Trim(line.substr(0, eq));
		std::string value = Trim(line.substr(eq + 1));
		current->entries.emplace_back(key, value);
	}

	fclose(f);
	return true;
}

std::vector<std::pair<std::string, std::string>> IniFile::GetSection(const std::string &section) const
{
	for (const auto &s : m_sections)
	{
		if (EqualsIgnoreCase(s.name, section))
			return s.entries;
	}
	return {};
}
