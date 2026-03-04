#pragma once

#include <string>
#include <vector>
#include <map>

struct LyricData;

class JapaneseProcessor
{
public:
    static bool ContainsJapanese(const std::string& text);
    static std::string KanjiToKana(const std::string& kanji);
    static std::string ToRomaji(const std::string& text);
    static std::vector<std::string> BatchToRomaji(const std::vector<std::string>& lines);

    static bool HasJapanese(const std::string& text);
    static bool HasJapanese(const LyricData& lyrics);

private:
    static std::wstring ToRomajiInternal(const std::wstring& text, struct IFELanguage* pFE = nullptr);
    static std::wstring KanjiToKana(const std::wstring& text, struct IFELanguage* pFE = nullptr);
    static std::wstring KanjiToKanaInternal(const std::wstring& text, struct IFELanguage* pFE);
    static std::wstring KanaToRomaji(const std::wstring& text);

    static const std::map<std::wstring, std::wstring> m_kana_romaji_map;
};
