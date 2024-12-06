#include "Strings.hpp"
#include "FsLib.hpp"
#include "JSON.hpp"
#include <map>
#include <string>
#include <unordered_map>

namespace
{
    // This is the actual map where the strings are.
    std::map<std::pair<std::string, int>, std::string> s_StringMap;
    // This map is for matching files to the language value
    std::unordered_map<SetLanguage, std::string_view> s_FileMap = {{SetLanguage_JA, "JA.json"},
                                                                   {SetLanguage_ENUS, "ENUS.json"},
                                                                   {SetLanguage_FR, "FR.json"},
                                                                   {SetLanguage_DE, "DE.json"},
                                                                   {SetLanguage_IT, "IT.json"},
                                                                   {SetLanguage_ES, "ES.json"},
                                                                   {SetLanguage_ZHCN, "ZHCN.json"},
                                                                   {SetLanguage_KO, "KO.json"},
                                                                   {SetLanguage_NL, "NL.json"},
                                                                   {SetLanguage_PT, "PT.json"},
                                                                   {SetLanguage_RU, "RU.json"},
                                                                   {SetLanguage_ZHTW, "ZHTW.json"},
                                                                   {SetLanguage_ENGB, "ENGB.json"},
                                                                   {SetLanguage_FRCA, "FRCA.json"},
                                                                   {SetLanguage_ES419, "ES419.json"},
                                                                   {SetLanguage_ZHHANS, "ZHCN.json"},
                                                                   {SetLanguage_ZHHANT, "ZHTW.json"},
                                                                   {SetLanguage_PTBR, "PTBR.json"}};
} // namespace

static FsLib::Path GetStringFilePath(void)
{
    FsLib::Path ReturnPath = "romfs:/Text";

    uint64_t LanguageCode = 0;
    Result SetError = setGetLanguageCode(&LanguageCode);
    if (R_FAILED(SetError))
    {
        return ReturnPath / s_FileMap.at(SetLanguage_ENUS);
    }

    SetLanguage Language;
    SetError = setMakeLanguage(LanguageCode, &Language);
    if (R_FAILED(SetError))
    {
        return ReturnPath / s_FileMap.at(SetLanguage_ENUS);
    }
    return ReturnPath / s_FileMap.at(Language);
}

bool Strings::Initialize()
{
    FsLib::Path StringsPath = GetStringFilePath();

    JSON::Object TextJSON = JSON::NewObject(json_object_from_file, StringsPath.CString());
    if (!TextJSON)
    {
        return false;
    }

    json_object_iterator StringIterator = json_object_iter_begin(TextJSON.get());
    json_object_iterator StringEnd = json_object_iter_end(TextJSON.get());
    while (!json_object_iter_equal(&StringIterator, &StringEnd))
    {
        // Get name of string(s) and pointer to array
        const char *StringName = json_object_iter_peek_name(&StringIterator);
        json_object *StringArray = json_object_iter_peek_value(&StringIterator);

        // Loop through array and add them to map so I can be lazier and not have to edit code or do shit to add more strings.
        size_t ArrayLength = json_object_array_length(StringArray);
        for (size_t i = 0; i < ArrayLength; i++)
        {
            json_object *String = json_object_array_get_idx(StringArray, i);
            s_StringMap[std::make_pair(StringName, static_cast<int>(i))] = json_object_get_string(String);
        }
        json_object_iter_next(&StringIterator);
    }
    return true;
}

const char *Strings::GetByName(std::string_view Name, int Index)
{
    if (s_StringMap.find(std::make_pair(Name.data(), Index)) == s_StringMap.end())
    {
        return nullptr;
    }
    return s_StringMap.at(std::make_pair(Name.data(), Index)).c_str();
}
