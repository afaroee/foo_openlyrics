#include "stdafx.h"
#include "japanese_processor.h"
#include "win32_util.h"
#include "logging.h"

// Note: IFELanguage and IID_IFELanguage are defined in msime.h
// CLSID_VERSION_DEPENDENT_MSIME_JAPANESE is also in msime.h
#include <initguid.h>
#include <msime.h>

struct ComScope
{
    HRESULT hr;
    bool finalized = false;
    ComScope() {
        hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (hr == RPC_E_CHANGED_MODE) {
            finalized = false;
        } else if (SUCCEEDED(hr)) {
            finalized = true;
        }
    }
    ~ComScope() { if (finalized) CoUninitialize(); }
};

struct FELanguageScope
{
    IFELanguage* p = nullptr;
    FELanguageScope()
    {
        HRESULT hr = CoCreateInstance(CLSID_VERSION_DEPENDENT_MSIME_JAPANESE, nullptr, CLSCTX_INPROC_SERVER, IID_IFELanguage, (void**)&p);
        if (SUCCEEDED(hr) && p)
        {
            hr = p->Open();
            if (FAILED(hr))
            {
                LOG_ERROR("Failed to open IFELanguage: 0x%08X", hr);
                p->Release();
                p = nullptr;
            }
        }
        else if (FAILED(hr))
        {
            LOG_ERROR("Failed to create IFELanguage instance: 0x%08X", hr);
        }
    }
    ~FELanguageScope()
    {
        if (p)
        {
            p->Close();
            p->Release();
        }
    }
    operator IFELanguage*() { return p; }
};

const std::map<std::wstring, std::wstring> JapaneseProcessor::m_kana_romaji_map = {
    {L"あ", L"a"}, {L"い", L"i"}, {L"う", L"u"}, {L"え", L"e"}, {L"お", L"o"},
    {L"か", L"ka"}, {L"き", L"ki"}, {L"く", L"ku"}, {L"け", L"ke"}, {L"こ", L"ko"},
    {L"さ", L"sa"}, {L"し", L"shi"}, {L"す", L"su"}, {L"せ", L"se"}, {L"そ", L"so"},
    {L"た", L"ta"}, {L"ち", L"chi"}, {L"つ", L"tsu"}, {L"て", L"te"}, {L"と", L"to"},
    {L"な", L"na"}, {L"に", L"ni"}, {L"ぬ", L"nu"}, {L"ね", L"ne"}, {L"の", L"no"},
    {L"は", L"ha"}, {L"ひ", L"hi"}, {L"ふ", L"fu"}, {L"へ", L"he"}, {L"ほ", L"ho"},
    {L"ま", L"ma"}, {L"み", L"mi"}, {L"む", L"mu"}, {L"め", L"me"}, {L"も", L"mo"},
    {L"や", L"ya"}, {L"ゆ", L"yu"}, {L"よ", L"yo"},
    {L"ら", L"ra"}, {L"り", L"ri"}, {L"る", L"ru"}, {L"れ", L"re"}, {L"ろ", L"ro"},
    {L"わ", L"wa"}, {L"を", L"wo"}, {L"ん", L"n"},
    {L"が", L"ga"}, {L"ぎ", L"gi"}, {L"ぐ", L"gu"}, {L"げ", L"ge"}, {L"ご", L"go"},
    {L"ざ", L"za"}, {L"じ", L"ji"}, {L"ず", L"zu"}, {L"ぜ", L"ze"}, {L"ぞ", L"zo"},
    {L"だ", L"da"}, {L"ぢ", L"ji"}, {L"づ", L"zu"}, {L"で", L"te"}, {L"ど", L"do"},
    {L"ば", L"ba"}, {L"び", L"bi"}, {L"ぶ", L"bu"}, {L"べ", L"be"}, {L"ぼ", L"bo"},
    {L"ぱ", L"pa"}, {L"ぴ", L"pi"}, {L"ぷ", L"pu"}, {L"ぺ", L"pe"}, {L"ぽ", L"po"},
    {L"きゃ", L"kya"}, {L"きゅ", L"kyu"}, {L"きょ", L"kyo"},
    {L"しゃ", L"sha"}, {L"しゅ", L"shu"}, {L"しょ", L"sho"},
    {L"ちゃ", L"cha"}, {L"ちゅ", L"chu"}, {L"ちょ", L"cho"},
    {L"にゃ", L"nya"}, {L"にゅ", L"nyu"}, {L"にょ", L"nyo"},
    {L"ひゃ", L"hya"}, {L"ひゅ", L"hyu"}, {L"ひょ", L"hyo"},
    {L"みゃ", L"mya"}, {L"みゅ", L"myu"}, {L"みょ", L"myo"},
    {L"りゃ", L"rya"}, {L"りゅ", L"ryu"}, {L"りょ", L"ryo"},
    {L"ぎゃ", L"gya"}, {L"ぎゅ", L"gyu"}, {L"ぎょ", L"gyo"},
    {L"じゃ", L"ja"}, {L"じゅ", L"ju"}, {L"じょ", L"jo"},
    {L"びゃ", L"bya"}, {L"びゅ", L"byu"}, {L"びょ", L"byo"},
    {L"ぴゃ", L"pya"}, {L"ぴゅ", L"pyu"}, {L"ぴょ", L"pyo"},
    {L"ア", L"a"}, {L"イ", L"i"}, {L"ウ", L"u"}, {L"エ", L"e"}, {L"オ", L"o"},
    {L"カ", L"ka"}, {L"キ", L"ki"}, {L"ク", L"ku"}, {L"ケ", L"ke"}, {L"コ", L"ko"},
    {L"サ", L"sa"}, {L"シ", L"shi"}, {L"ス", L"su"}, {L"セ", L"se"}, {L"ソ", L"so"},
    {L"タ", L"ta"}, {L"チ", L"chi"}, {L"ツ", L"tsu"}, {L"テ", L"te"}, {L"ト", L"to"},
    {L"ナ", L"na"}, {L"ニ", L"ni"}, {L"ヌ", L"nu"}, {L"ネ", L"ne"}, {L"ノ", L"no"},
    {L"ハ", L"ha"}, {L"ヒ", L"hi"}, {L"フ", L"fu"}, {L"ヘ", L"he"}, {L"ホ", L"ho"},
    {L"マ", L"ma"}, {L"ミ", L"mi"}, {L"ム", L"mu"}, {L"メ", L"me"}, {L"モ", L"mo"},
    {L"ヤ", L"ya"}, {L"ユ", L"yu"}, {L"ヨ", L"yo"},
    {L"ラ", L"ra"}, {L"リ", L"ri"}, {L"ル", L"ru"}, {L"レ", L"re"}, {L"ロ", L"ro"},
    {L"ワ", L"wa"}, {L"ヲ", L"wo"}, {L"ン", L"n"},
    {L"ガ", L"ga"}, {L"ギ", L"gi"}, {L"グ", L"gu"}, {L"ゲ", L"ge"}, {L"ゴ", L"go"},
    {L"ザ", L"za"}, {L"ジ", L"ji"}, {L"ズ", L"zu"}, {L"ゼ", L"ze"}, {L"ゾ", L"zo"},
    {L"ダ", L"da"}, {L"ヂ", L"ji"}, {L"ヅ", L"zu"}, {L"デ", L"te"}, {L"ド", L"do"},
    {L"バ", L"ba"}, {L"ビ", L"bi"}, {L"ブ", L"bu"}, {L"ベ", L"be"}, {L"ボ", L"bo"},
    {L"パ", L"pa"}, {L"ピ", L"pi"}, {L"プ", L"pu"}, {L"ペ", L"pe"}, {L"ポ", L"po"},
    {L"キャ", L"kya"}, {L"キュ", L"kyu"}, {L"キョ", L"kyo"},
    {L"シャ", L"sha"}, {L"シュ", L"shu"}, {L"ショ", L"sho"},
    {L"チャ", L"cha"}, {L"チュ", L"chu"}, {L"チョ", L"cho"},
    {L"ニャ", L"nya"}, {L"ニュ", L"nyu"}, {L"ニョ", L"nyo"},
    {L"ヒャ", L"hya"}, {L"ヒュ", L"hyu"}, {L"ヒョ", L"hyo"},
    {L"ミャ", L"mya"}, {L"ミュ", L"myu"}, {L"ミョ", L"myo"},
    {L"リャ", L"rya"}, {L"リュ", L"ryu"}, {L"リョ", L"ryo"},
    {L"ギャ", L"gya"}, {L"ギュ", L"kyu"}, {L"ギョ", L"gyo"},
    {L"ジャ", L"ja"}, {L"ジュ", L"ju"}, {L"ジョ", L"jo"},
    {L"ビャ", L"bya"}, {L"ビュ", L"byu"}, {L"ビョ", L"byo"},
    {L"ピャ", L"pya"}, {L"ピュ", L"pyu"}, {L"ピョ", L"pyo"}
};

bool JapaneseProcessor::ContainsJapanese(const std::string& text)
{
    std::vector<wchar_t> wide_text_vec;
    narrow_to_wide_string(CP_UTF8, text, wide_text_vec);
    std::wstring wide_text(wide_text_vec.data(), wide_text_vec.size());

    for (wchar_t c : wide_text)
    {
        if ((c >= 0x3040 && c <= 0x309F) || // Hiragana
            (c >= 0x30A0 && c <= 0x30FF) || // Katakana
            (c >= 0x4E00 && c <= 0x9FAF) || // Kanji
            (c >= 0xFF66 && c <= 0xFF9F))   // Half-width Katakana
        {
            return true;
        }
    }
    return false;
}

std::string JapaneseProcessor::ToRomaji(const std::string& text)
{
    std::vector<wchar_t> wide_text_vec;
    narrow_to_wide_string(CP_UTF8, text, wide_text_vec);
    std::wstring wide_text(wide_text_vec.data(), wide_text_vec.size());

    std::wstring result = ToRomajiInternal(wide_text);

    std::vector<char> narrow_result_vec;
    wide_to_narrow_string(CP_UTF8, result, narrow_result_vec);
    return std::string(narrow_result_vec.data(), narrow_result_vec.size());
}

std::wstring JapaneseProcessor::ToRomajiInternal(const std::wstring& text, IFELanguage* pFE)
{
    std::wstring kana = KanjiToKana(text, pFE);
    return KanaToRomaji(kana);
}

std::vector<std::string> JapaneseProcessor::BatchToRomaji(const std::vector<std::string>& lines)
{
    std::vector<std::string> results;
    results.reserve(lines.size());

    ComScope com;
    if (FAILED(com.hr) && com.hr != RPC_E_CHANGED_MODE)
    {
        LOG_ERROR("BatchToRomaji: CoInitializeEx failed with 0x%08X", com.hr);
        return lines;
    }

    FELanguageScope fe;
    if (!fe.p)
    {
        return lines;
    }

    for (const auto& line : lines)
    {
        std::vector<wchar_t> wide_text_vec;
        narrow_to_wide_string(CP_UTF8, line, wide_text_vec);
        std::wstring wide_text(wide_text_vec.data(), wide_text_vec.size());

        std::wstring result_wide = ToRomajiInternal(wide_text, fe.p);

        std::vector<char> narrow_result_vec;
        wide_to_narrow_string(CP_UTF8, result_wide, narrow_result_vec);
        results.push_back(std::string(narrow_result_vec.data(), narrow_result_vec.size()));
    }

    return results;
}

std::wstring JapaneseProcessor::KanjiToKana(const std::wstring& text, IFELanguage* pFE)
{
    if (pFE)
    {
        return KanjiToKanaInternal(text, pFE);
    }

    ComScope com;
    if (FAILED(com.hr) && com.hr != RPC_E_CHANGED_MODE)
    {
        return text;
    }

    FELanguageScope fe;
    if (!fe.p)
    {
        return text;
    }

    return KanjiToKanaInternal(text, fe.p);
}

std::wstring JapaneseProcessor::KanjiToKanaInternal(const std::wstring& text, IFELanguage* pFE)
{
    std::wstring result = text;
    BSTR bstrInput = SysAllocString(text.c_str());
    if (bstrInput)
    {
        BSTR bstrOutput = nullptr;
        HRESULT hr = pFE->GetPhonetic(bstrInput, 1, (LONG)text.length(), &bstrOutput);
        if (SUCCEEDED(hr) && bstrOutput)
        {
            result = bstrOutput;
            SysFreeString(bstrOutput);
        }
        SysFreeString(bstrInput);
    }

    return result;
}

std::wstring JapaneseProcessor::KanaToRomaji(const std::wstring& text)
{
    std::wstring result;
    for (size_t i = 0; i < text.length(); ++i)
    {
        // Handle double consonants (small 'tsu')
        if (text[i] == L'っ' || text[i] == L'ッ')
        {
            if (i + 1 < text.length())
            {
                std::wstring next_kana = text.substr(i + 1, 1);
                auto it = m_kana_romaji_map.find(next_kana);
                if (it != m_kana_romaji_map.end() && !it->second.empty())
                {
                    result += it->second[0]; // Add first char of next romaji
                    continue;
                }
            }
        }

        // Handle combinations (e.g., きゃ, しゃ)
        if (i + 1 < text.length())
        {
            std::wstring combo = text.substr(i, 2);
            auto it = m_kana_romaji_map.find(combo);
            if (it != m_kana_romaji_map.end())
            {
                result += it->second;
                i++;
                continue;
            }
        }

        // Handle single characters
        std::wstring single = text.substr(i, 1);
        auto it = m_kana_romaji_map.find(single);
        if (it != m_kana_romaji_map.end())
        {
            result += it->second;
        }
        else
        {
            result += text[i];
        }
    }
    return result;
}

// ============
// Tests
// ============
#if MVTF_TESTS_ENABLED
MVTF_TEST(japanese_processor_contains_japanese_detects_hiragana)
{
    ASSERT(JapaneseProcessor::ContainsJapanese("あいうえお"));
}

MVTF_TEST(japanese_processor_contains_japanese_detects_katakana)
{
    ASSERT(JapaneseProcessor::ContainsJapanese("アイウエオ"));
}

MVTF_TEST(japanese_processor_contains_japanese_detects_kanji)
{
    ASSERT(JapaneseProcessor::ContainsJapanese("漢字"));
}

MVTF_TEST(japanese_processor_contains_japanese_false_for_ascii)
{
    ASSERT(!JapaneseProcessor::ContainsJapanese("Hello World"));
}

MVTF_TEST(japanese_processor_to_romaji_converts_hiragana)
{
    ASSERT(JapaneseProcessor::ToRomaji("あいうえお") == "aiueo");
}

MVTF_TEST(japanese_processor_to_romaji_converts_katakana)
{
    ASSERT(JapaneseProcessor::ToRomaji("アイウエオ") == "aiueo");
}

MVTF_TEST(japanese_processor_to_romaji_handles_small_tsu)
{
    ASSERT(JapaneseProcessor::ToRomaji("がっこう") == "gakkou");
}

MVTF_TEST(japanese_processor_to_romaji_handles_combinations)
{
    ASSERT(JapaneseProcessor::ToRomaji("きょう") == "kyou");
}
#endif
