#include "stdafx.h"
#include "japanese_processor.h"
#include "win32_util.h"
#include "logging.h"
#include "lyric_data.h"

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
    {L"だ", L"da"}, {L"ぢ", L"ji"}, {L"づ", L"zu"}, {L"で", L"de"}, {L"ど", L"do"},
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
    {L"ダ", L"da"}, {L"ヂ", L"ji"}, {L"ヅ", L"zu"}, {L"デ", L"de"}, {L"ド", L"do"},
    {L"バ", L"ba"}, {L"ビ", L"bi"}, {L"ブ", L"bu"}, {L"ベ", L"be"}, {L"ボ", L"bo"},
    {L"パ", L"pa"}, {L"ピ", L"pi"}, {L"プ", L"pu"}, {L"ペ", L"pe"}, {L"ポ", L"po"},
    {L"キャ", L"kya"}, {L"キュ", L"kyu"}, {L"キョ", L"kyo"},
    {L"シャ", L"sha"}, {L"シュ", L"shu"}, {L"ショ", L"sho"},
    {L"チャ", L"cha"}, {L"チュ", L"chu"}, {L"チョ", L"cho"},
    {L"ニャ", L"nya"}, {L"ニュ", L"nyu"}, {L"ニョ", L"nyo"},
    {L"ヒャ", L"hya"}, {L"ヒュ", L"hyu"}, {L"ヒョ", L"hyo"},
    {L"ミャ", L"mya"}, {L"ミュ", L"myu"}, {L"ミョ", L"myo"},
    {L"リャ", L"rya"}, {L"リュ", L"ryu"}, {L"リョ", L"ryo"},
    {L"ギャ", L"gya"}, {L"ギュ", L"gyu"}, {L"ギョ", L"gyo"},
    {L"ジャ", L"ja"}, {L"ジュ", L"ju"}, {L"ジョ", L"jo"},
    {L"ビャ", L"bya"}, {L"ビュ", L"byu"}, {L"ビョ", L"byo"},
    {L"ピャ", L"pya"}, {L"ピュ", L"pyu"}, {L"ピョ", L"pyo"},
    // Modern Katakana combinations
    {L"チェ", L"che"}, {L"ちぇ", L"che"},
    {L"ディ", L"di"}, {L"でぃ", L"di"},
    {L"デュ", L"dyu"}, {L"でゅ", L"dyu"},
    {L"ドゥ", L"du"}, {L"どぅ", L"du"},
    {L"ティ", L"ti"}, {L"てぃ", L"ti"},
    {L"テュ", L"tyu"}, {L"てゅ", L"tyu"},
    {L"トゥ", L"tu"}, {L"とぅ", L"tu"},
    {L"ファ", L"fa"}, {L"ふぁ", L"fa"},
    {L"フィ", L"fi"}, {L"ふぃ", L"fi"},
    {L"フェ", L"fe"}, {L"ふぇ", L"fe"},
    {L"フォ", L"fo"}, {L"ふぉ", L"fo"},
    {L"ウィ", L"wi"}, {L"うぃ", L"wi"},
    {L"ウェ", L"we"}, {L"うぇ", L"we"},
    {L"ウォ", L"wo"}, {L"うぉ", L"wo"},
    {L"ヴァ", L"va"}, {L"ヴィ", L"vi"}, {L"ヴェ", L"ve"}, {L"ヴォ", L"vo"},
    {L"シェ", L"she"}, {L"じぇ", L"je"}, {L"ジェ", L"je"},
    // Small kana fallbacks (for cases where they are not part of a recognized combination)
    {L"ぁ", L"a"}, {L"ぃ", L"i"}, {L"ぅ", L"u"}, {L"ぇ", L"e"}, {L"ぉ", L"o"},
    {L"ァ", L"a"}, {L"ィ", L"i"}, {L"ゥ", L"u"}, {L"ェ", L"e"}, {L"ォ", L"o"},
    {L"ゃ", L"ya"}, {L"ゅ", L"yu"}, {L"ょ", L"yo"},
    {L"ャ", L"ya"}, {L"ュ", L"yu"}, {L"ョ", L"yo"},
    {L"っ", L"t"}, {L"ッ", L"t"}
};

bool JapaneseProcessor::ContainsJapanese(const std::string& text)
{
    return HasJapanese(text);
}

bool JapaneseProcessor::HasJapanese(const std::string& text)
{
    std::tstring wtext = to_tstring(text);
    for (wchar_t c : wtext)
    {
        // Hiragana: 3040–309F
        // Katakana: 30A0–30FF
        // Kanji: 4E00–9FAF
        if ((c >= 0x3040 && c <= 0x309F) ||
            (c >= 0x30A0 && c <= 0x30FF) ||
            (c >= 0x4E00 && c <= 0x9FAF))
        {
            return true;
        }
    }
    return false;
}

bool JapaneseProcessor::HasJapanese(const LyricData& lyrics)
{
    for (const LyricDataLine& l : lyrics.lines)
    {
        if (HasJapanese(from_tstring(l.text)))
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
    if (text.empty()) return L"";

    auto process_with_fe = [&](IFELanguage* fe) -> std::wstring {
        // First, try morphological analysis to detect particles and segments
        MORRSLT* pResult = nullptr;
        HRESULT hr = fe->GetJMorphResult(
            FELANG_REQ_REV,            // reverse conversion to get morphemes
            FELANG_CMODE_HIRAGANAOUT,  // output in hiragana
            (INT)text.length(),
            text.c_str(),
            nullptr,                   // no per-character info
            &pResult);

        if (SUCCEEDED(hr) && pResult && pResult->pWDD && pResult->cWDD > 0)
        {
            std::wstring result;
            
            // Process each word descriptor (WDD) segment
            for (WORD i = 0; i < pResult->cWDD; ++i)
            {
                const WDD& wdd = pResult->pWDD[i];
                
                // Get the original surface text for this segment
                // WDD offsets are relative to pResult->pwchOutput (the conversion result string)
                std::wstring surface;
                if (pResult->pwchOutput && wdd.cchDisp > 0)
                {
                    surface.assign(pResult->pwchOutput + wdd.wDispPos, wdd.cchDisp);
                }
                else
                {
                    // Fallback to original text if pwchOutput is somehow missing
                    surface = text.substr(wdd.wDispPos, wdd.cchDisp);
                }
                
                // Get the reading for this segment
                std::wstring reading;
                if (pResult->pwchRead && wdd.cchRead > 0)
                {
                    reading.assign(pResult->pwchRead + wdd.wReadPos, wdd.cchRead);
                }
                
                // If reading still contains Kanji, or is empty, use GetPhonetic for this specific segment
                bool hasKanjiInReading = false;
                for (wchar_t c : reading)
                {
                    if (c >= 0x4E00 && c <= 0x9FAF) { hasKanjiInReading = true; break; }
                }

                if (reading.empty() || hasKanjiInReading)
                {
                    // Call GetPhonetic for just this segment to ensure we get Kana
                    BSTR bstrInput = SysAllocString(surface.c_str());
                    if (bstrInput)
                    {
                        BSTR bstrOutput = nullptr;
                        HRESULT hrP = fe->GetPhonetic(bstrInput, 1, (LONG)surface.length(), &bstrOutput);
                        if (SUCCEEDED(hrP) && bstrOutput)
                        {
                            reading = bstrOutput;
                            SysFreeString(bstrOutput);
                        }
                        SysFreeString(bstrInput);
                    }
                }

                // If still empty fallback to surface
                if (reading.empty()) reading = surface;

                // Special handling for particles "は" (wa) and "へ" (e)
                // These should be 'wa' and 'e' when used as particles.
                // We check if:
                // 1. The POS has the particle bit set.
                // 2. The reading is EXACTLY 'は' or 'へ' and it's NOT a noun/verb/adjective.
                // 3. The segment ends with 'は' or 'へ' and matches a common word+particle pattern.
                
                bool isLikelyWaParticle = (reading == L"\u306f") && 
                    ((wdd.nPos & IFED_POS_PARTICLE) || !(wdd.nPos & (IFED_POS_NOUN | IFED_POS_VERB | IFED_POS_ADJECTIVE)));
                
                bool isLikelyEParticle = (reading == L"\u3078") && 
                    ((wdd.nPos & IFED_POS_PARTICLE) || !(wdd.nPos & (IFED_POS_NOUN | IFED_POS_VERB | IFED_POS_ADJECTIVE)));

                if (isLikelyWaParticle)
                {
                    result += L"wa";
                }
                else if (isLikelyEParticle)
                {
                    result += L"e";
                }
                else if (reading.length() > 1 && reading.back() == L'\u306f' && (wdd.nPos & (IFED_POS_PARTICLE | IFED_POS_NOUN | IFED_POS_ADVERB)))
                {
                    // This handles cases like "それは", "君は", "僕は" if they are one WDD
                    std::wstring base = reading.substr(0, reading.length() - 1);
                    result += KanaToRomaji(base) + L"wa";
                }
                else if (reading.length() > 1 && reading.back() == L'\u3078' && (wdd.nPos & (IFED_POS_PARTICLE | IFED_POS_NOUN | IFED_POS_ADVERB)))
                {
                    // This handles cases like "どこへ", "あそこへ"
                    std::wstring base = reading.substr(0, reading.length() - 1);
                    result += KanaToRomaji(base) + L"e";
                }
                else
                {
                    // Not a identified particle at end, convert normally
                    result += KanaToRomaji(reading);
                }
            }

            CoTaskMemFree(pResult);
            return result;
        }

        if (pResult) CoTaskMemFree(pResult);

        // Fallback: use simple phonetic conversion for the whole string if morphological analysis failed
        // We still use ToRomaji on it line by line if we want to be safe, but let's try to improve GetJMorphResult first.
        std::wstring rawKana = KanjiToKanaInternal(text, fe);
        // Even in fallback, try to detect some very common particles at the end of the line or before spaces
        // but this is risky, so we mostly rely on GetJMorphResult.
        return KanaToRomaji(rawKana);
    };

    if (pFE) return process_with_fe(pFE);

    ComScope com;
    if (FAILED(com.hr) && com.hr != RPC_E_CHANGED_MODE) return KanaToRomaji(KanjiToKana(text));

    FELanguageScope fe;
    if (!fe.p) return KanaToRomaji(KanjiToKana(text));

    return process_with_fe(fe.p);
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
                    // Special case for Hepburn: 'っ' before 'ch' is 't'
                    if (it->second.size() >= 2 && it->second[0] == 'c' && it->second[1] == 'h')
                    {
                        result += 't';
                    }
                    else
                    {
                        result += it->second[0];
                    }
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

        // Handle long vowel mark (extend previous vowel)
        if (text[i] == L'ー' || text[i] == L'-')
        {
            if (!result.empty())
            {
                wchar_t last = result.back();
                if (last == L'a' || last == L'i' || last == L'u' || last == L'e' || last == L'o')
                {
                    result += last;
                    continue;
                }
            }
            // If we couldn't match a vowel or it's at start, just add a dash or ignore
            // For lyrics, ignoring or adding nothing is common, but let's keep it clean
            continue; 
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

MVTF_TEST(japanese_processor_to_romaji_handles_small_kana_combinations)
{
    ASSERT(JapaneseProcessor::ToRomaji("チェンジ") == "chenji");
    ASSERT(JapaneseProcessor::ToRomaji("ドゥ") == "du");
}

MVTF_TEST(japanese_processor_to_romaji_handles_long_vowel_mark)
{
    ASSERT(JapaneseProcessor::ToRomaji("メロディー") == "merodii");
    ASSERT(JapaneseProcessor::ToRomaji("ダカーポ") == "dakaapo");
}

MVTF_TEST(japanese_processor_to_romaji_handles_lone_small_kana)
{
    // Safety fallback for lone small kana
    ASSERT(JapaneseProcessor::ToRomaji(" chiェnji ") == " chenji ");
    ASSERT(JapaneseProcessor::ToRomaji("っ") == "t");
}

MVTF_TEST(japanese_processor_to_romaji_handles_sokuon_at_boundaries)
{
    // "笑っちゃ" -> "waratcha"
    // Even if split into "わらっ" and "ちゃ", it should result in "waratcha"
    // (fallback 't' from map + 'cha' = 'tcha')
    ASSERT(JapaneseProcessor::ToRomaji("笑っちゃ") == "waratcha");
}

MVTF_TEST(japanese_processor_to_romaji_handles_small_tsu)
{
    ASSERT(JapaneseProcessor::ToRomaji("がっこう") == "gakkou");
}

MVTF_TEST(japanese_processor_to_romaji_handles_combinations)
{
    ASSERT(JapaneseProcessor::ToRomaji("きょう") == "kyou");
}

MVTF_TEST(japanese_processor_to_romaji_fixes_misconversions)
{
    ASSERT(JapaneseProcessor::ToRomaji("で") == "de");
    ASSERT(JapaneseProcessor::ToRomaji("デ") == "de");
    ASSERT(JapaneseProcessor::ToRomaji("ギュ") == "gyu");
}

MVTF_TEST(japanese_processor_to_romaji_handles_particle_wa)
{
    ASSERT(JapaneseProcessor::ToRomaji("私は") == "watashiwa");
    ASSERT(JapaneseProcessor::ToRomaji("おはよう") == "ohayou");
    ASSERT(JapaneseProcessor::ToRomaji("はじめまして") == "hajimemashite");
}
#endif
