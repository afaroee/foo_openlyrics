#pragma once

#include <string>
#include <vector>
#include <map>

class JapaneseProcessor
{
public:
    static bool ContainsJapanese(const std::string& text);
    static std::string ToRomaji(const std::string& text);

private:
    static std::wstring ToRomajiInternal(const std::wstring& text);
    static std::wstring KanjiToKana(const std::wstring& text);
    static std::wstring KanaToRomaji(const std::wstring& text);

    static const std::map<std::wstring, std::wstring> m_kana_romaji_map;
};
