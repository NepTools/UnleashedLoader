#include "FileMap.h"

#include <cctype>

namespace
{
	std::string LowerSlashes(const std::string &in)
	{
		std::string out = in;
		for (char &c : out)
		{
			if (c == '/')
				c = '\\';
			c = (char)tolower((unsigned char)c);
		}
		return out;
	}
}

void FileMap::SetGameRoot(const std::string &gameRoot)
{
	m_gameRoot = LowerSlashes(gameRoot);
	while (!m_gameRoot.empty() && m_gameRoot.back() == '\\')
		m_gameRoot.pop_back();
}

std::string FileMap::Normalize(const std::string &path) const
{
	std::string p = LowerSlashes(path);

	if (p.size() >= 2 && p[0] == '.' && p[1] == '\\')
		p = p.substr(2);

	if (!m_gameRoot.empty() && p.size() > m_gameRoot.size() &&
		p.compare(0, m_gameRoot.size(), m_gameRoot) == 0 &&
		p[m_gameRoot.size()] == '\\')
	{
		p = p.substr(m_gameRoot.size() + 1);
	}

	while (!p.empty() && p.front() == '\\')
		p.erase(p.begin());

	return p;
}

void FileMap::AddFile(const std::string &key, const std::string &absolutePath)
{
	m_map[Normalize(key)] = absolutePath;
}

bool FileMap::TryReplace(const std::string &originalPath, std::string &outReplacement) const
{
	auto it = m_map.find(Normalize(originalPath));
	if (it == m_map.end())
		return false;

	outReplacement = it->second;
	return true;
}
