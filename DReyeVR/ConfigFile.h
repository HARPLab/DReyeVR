#pragma once
#include <fstream> // std::ifstream
#include <istream> // std::istream
#include <sstream> // std::istringstream
#include <string>
#include <unordered_map>

const static FString CarlaUE4Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

struct ConfigFile
{
    ConfigFile() = default; // empty constructor (no params yet)
    ConfigFile(const FString &Path, bool bVerbose = true) : FilePath(Path)
    {
        /// TODO: add feature to "hot-reload" new params during runtime
        bSuccessfulUpdate = ReadFile(bVerbose); // ensures all the variables are updated upon construction

        // simple sanity check to ensure exporting and importing the same config file works as intended
        // (exporting self and creating a new import should be equal to self)
        static bool bSanityCheck = this->IsEqual(ConfigFile::Import(this->Export()));
        ensureMsgf(bSanityCheck, TEXT("Sanity check for ConfigFile import/export failed!"));
    }

    template <typename T> bool Get(const FString &Section, const FString &Variable, T &Value) const
    {
        const std::string SectionStdStr(TCHAR_TO_UTF8(*Section));
        const std::string VariableStdStr(TCHAR_TO_UTF8(*Variable));
        return GetValue(SectionStdStr, VariableStdStr, Value);
    }

    template <typename T> T Get(const FString &Section, const FString &Variable) const
    {
        T Value;
        /// TODO: implement exception when Get returns false?
        Get(Section, Variable, Value);
        return Value;
    }

    template <typename T>
    T GetConstrained(const FString &Section, const FString &Variable, const std::unordered_set<T> &Options,
                     const T &DefaultValue) const
    {
        T Value = Get<T>(Section, Variable);
        if (Options.find(Value) == Options.end())
        {
            // not found within the constrained available options
            Value = DefaultValue;
        }
        return Value;
    }

    bool bIsValid() const
    {
        return bSuccessfulUpdate;
    }

    bool IsEqual(const ConfigFile &Other, bool bPrintWarning = false) const
    {
        // calculates if A subset B and B subset A
        return this->IsSubset(Other, bPrintWarning) && Other.IsSubset(*this, bPrintWarning);
    }

    bool IsSubset(const ConfigFile &Other, bool bPrintWarning = false) const
    {
        // only checking that this is a perfect subset of Other
        // => Other can contain data that this config does not have
        // (if you want perfect equality, do A.CompareEqual(B) && B.CompareEqual(A))
        struct Comparison
        {
            Comparison(const std::string &Section, const std::string &Variable, const FString &Expected,
                       const FString &Other)
                : SectionName(Section), VariableName(Variable), ThisValue(Expected), OtherValue(Other)
            {
            }

            Comparison(const std::string &Section, const std::string &Variable)
                : SectionName(Section), VariableName(Variable), bIsMissing(true)
            {
            }
            const std::string SectionName, VariableName;
            const FString ThisValue, OtherValue;
            const bool bIsMissing = false;
        };
        std::vector<Comparison> Diff = {};
        for (const auto &SectionData : Sections)
        {
            const std::string &SectionName = SectionData.first;
            const IniSection &Section = SectionData.second;
            for (const auto &EntryData : Section.Entries)
            {
                const std::string &VariableName = EntryData.first;
                const ParamString &Value = EntryData.second;
                ParamString OtherValue;
                if (Other.Find(SectionName, VariableName, OtherValue))
                {
                    // compare equality
                    if (!Value.DataStr.Equals(OtherValue.DataStr, ESearchCase::IgnoreCase))
                    {
                        Diff.push_back({SectionName, VariableName, Value.DataStr, OtherValue.DataStr});
                    }
                }
                else // did not find, missing
                {
                    Diff.push_back({SectionName, VariableName});
                }
            }
        }

        bool bIsDifferent = (Diff.size() > 0);
        // print differences
        if (bPrintWarning && bIsDifferent)
        {
            LOG_WARN("Found config differences this {\"%s\"} and Other {\"%s\"}", *FilePath, *Other.FilePath);
            for (const Comparison &Comp : Diff)
            {
                if (Comp.bIsMissing)
                {
                    LOG_WARN("Missing [%s] \"%s\"", *FString(Comp.SectionName.c_str()),
                             *FString(Comp.VariableName.c_str()));
                }
                else
                {
                    LOG_WARN("This [%s] \"%s\" {%s} does not match {%s}", *FString(Comp.SectionName.c_str()),
                             *FString(Comp.VariableName.c_str()), *Comp.ThisValue, *Comp.OtherValue);
                }
            }
        }
        return !bIsDifferent;
    }

    void Insert(const ConfigFile &Other)
    {
        FilePath += ";" + Other.FilePath;
        Sections.insert(Other.Sections.begin(), Other.Sections.end());
        bSuccessfulUpdate = true;
    }

    static ConfigFile Import(const std::string &Configuration)
    {
        // takes a flattened INI configuration file as parameter and reads it into a ConfigFile class
        return ConfigFile(Configuration);
    }

    std::string Export() const
    {
        std::ostringstream oss;
        oss << std::endl
            << "# This is an exported config file originally from \"" << TCHAR_TO_UTF8(*FilePath) << "\"" << std::endl
            << std::endl;
        for (const auto &SectionData : Sections)
        {
            const std::string &SectionName = SectionData.first;
            const IniSection &Section = SectionData.second;
            oss << "[" << SectionName << "]" << std::endl; // The overarching section first
            for (const auto &EntryData : Section.Entries)
            {
                const std::string &VariableName = EntryData.first;
                const ParamString &Data = EntryData.second;
                const std::string DataUTF = TCHAR_TO_UTF8(*Data.DataStr); // convert to UTF-8 format
                const std::string DataStdStr = Data.bHasQuotes ? "\"" + DataUTF + "\"" : DataUTF;
                oss << VariableName << "=" << DataStdStr << std::endl;
            }
            oss << std::endl;
        }
        return oss.str();
    }

  private:
    bool ReadFile(bool bVerbose)
    {
        check(!FilePath.IsEmpty());
        if (bVerbose)
        {
            LOG("Reading config from %s", *FilePath);
        }
        std::ifstream MatchingFile(TCHAR_TO_ANSI(*FilePath), std::ios::in);
        if (MatchingFile)
        {
            return Update(MatchingFile);
        }
        LOG_ERROR("Unable to open the config file \"%s\"", *FilePath);
        return false;
    }

    bool Update(std::istream &InputStream) // reload the internally tracked table of params
    {
        /// performs a single pass over the config stream to collect all variables into Params
        std::string Line;
        std::string Section = "";
        while (std::getline(InputStream, Line))
        {
            if (InputStream.bad()) // IO error
                return false;
            // std::string stdKey = std::string(TCHAR_TO_UTF8(*Key));
            if (Line[0] == '#' || Line[0] == ';') // ignore comments
                continue;
            std::istringstream iss_Line(Line);
            if (Line[0] == '[') // test section
            {
                std::getline(iss_Line, Section, ']');
                Section = Section.substr(1); // skip leading '['
                continue;
            }
            std::string Key;
            if (std::getline(iss_Line, Key, '=')) // gets left side of '=' into FileKey
            {
                std::string Value;
                if (std::getline(iss_Line, Value, '#')) // gets left side of '#' for comments
                {
                    // ensure there is a section (create one if necesary) to store this key:value pair
                    if (Sections.find(Section) == Sections.end())
                    {
                        Sections.insert({Section, IniSection(Section)});
                    }
                    check(Sections.find(Section) != Sections.end());
                    auto &CorrespondingSection = Sections.find(Section)->second;
                    CorrespondingSection.Entries.insert({Key, ParamString(Value)});
                }
            }
        }
        return true;
    }

  private:
    struct ParamString
    {
        ParamString() = default;
        ParamString(const std::string &Value)
        {
            DataStr = FString(Value.c_str()).TrimStartAndEnd().TrimQuotes(&bHasQuotes);
        }

        FString DataStr = ""; // string representation of the data to parse into primitives

        template <typename T> inline T DecipherToType() const
        {
            // supports FVector, FVector2D, FLinearColor, FQuat, and FRotator,
            // basically any UE4 type that has a ::InitFromString method
            T Ret;
            if (Ret.InitFromString(DataStr) == false)
            {
                LOG_ERROR("Unable to decipher \"%s\" to a type", *DataStr);
            }
            return Ret;
        }

        template <> inline bool DecipherToType<bool>() const
        {
            return DataStr.ToBool();
        }

        template <> inline int DecipherToType<int>() const
        {
            return FCString::Atoi(*DataStr);
        }

        template <> inline float DecipherToType<float>() const
        {
            return FCString::Atof(*DataStr);
        }

        template <> inline FString DecipherToType<FString>() const
        {
            return DataStr;
        }

        template <> inline FName DecipherToType<FName>() const
        {
            return FName(*DataStr);
        }
        bool bHasQuotes = false;
    };

    struct IniSection
    {
        IniSection(const std::string &SectionName) : SectionHeader(SectionName)
        {
        }

        std::string SectionHeader;                            // typically what is contained in [Sections]
        std::unordered_map<std::string, ParamString> Entries; // everything else
    };

  private:
    // construct a ConfigFile by passing in the entire contents of the config file (rather than the path to read)
    ConfigFile(const std::string &ConfigurationContents) : FilePath("")
    {
        std::istringstream iss(ConfigurationContents);
        bSuccessfulUpdate = Update(iss);
    }

    // using std::string variant for internal use (FString for user-facing)
    template <typename T> bool GetValue(const std::string &SectionName, const std::string &VariableName, T &Value) const
    {
        ParamString Param;
        bool bFound = Find(SectionName, VariableName, Param);
        if (bFound)
            Value = Param.DecipherToType<T>();
        return bFound;
    }

    bool Find(const std::string &SectionName, const std::string &VariableName, ParamString &Out) const
    {
        auto SectionIt = Sections.find(SectionName);
        if (SectionIt == Sections.end())
        {
            LOG_ERROR("No section in config file matches \"%s\"", *FString(SectionName.c_str()));
            return false;
        }
        const IniSection &Section = SectionIt->second;
        auto EntryIt = Section.Entries.find(VariableName);
        if (EntryIt == Section.Entries.end())
        {
            LOG_ERROR("No entry for \"%s\" in [%s] section found!", *FString(VariableName.c_str()),
                      *FString(SectionName.c_str()));
            return false;
        }
        Out = EntryIt->second;

        // enable this for debug purposes
        // LOG("Read [%s]\"%s\" -> %s", *FString(SectionName.c_str()), *FString(VariableName.c_str()), *Out.DataStr);
        return true; // found successfully!
    }

  private:
    FString FilePath; // const except for overwrite
    bool bSuccessfulUpdate = false;
    std::unordered_map<std::string, IniSection> Sections;
};

static ConfigFile GeneralParams(FPaths::Combine(CarlaUE4Path, TEXT("Config/DReyeVRConfig.ini")));
