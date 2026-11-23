#pragma once

#include <string>
#include <unordered_map>

class FileMap
{
public:
	void SetGameRoot(const std::string &gameRoot);
	void AddFile(const std::string &key, const std::string &absolutePath);
	bool TryReplace(const std::string &originalPath, std::string &outReplacement) const;
	size_t Count() const { return m_map.size(); }
	std::string Normalize(const std::string &path) const;

private:
	std::string m_gameRoot;
	std::unordered_map<std::string, std::string> m_map;
};
