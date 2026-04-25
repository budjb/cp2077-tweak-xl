#pragma once

namespace Red::TweakDBUtil
{
CClass* GetRecordType(CName aTypeName);
CClass* GetRecordType(const char* aTypeName);

bool IsRecordType(CName aTypeName);
bool IsRecordType(const CClass* aType);

template<typename T>
T GetRecordFullName(const std::string& aName);

template<typename T>
T GetRecordFullName(const char* aName);

template<typename T>
T GetRecordFullName(CName aName);

template<typename T>
T GetRecordAliasName(const std::string& aName);

template<typename T>
T GetRecordAliasName(const char* aName);

template<typename T>
T GetRecordAliasName(CName aName);

template<>
[[nodiscard]] std::string GetRecordFullName(const std::string& aName);

template<>
std::string GetRecordFullName(const char* aName);

template<>
[[nodiscard]] std::string GetRecordFullName(CName aName);

template<>
[[nodiscard]] std::string GetRecordAliasName(const std::string& aName);

template<>
[[nodiscard]] std::string GetRecordAliasName(const char* aName);

template<>
[[nodiscard]] std::string GetRecordAliasName(CName aName);

template<>
[[nodiscard]] CName GetRecordFullName(const std::string& aName);

template<>
[[nodiscard]] CName GetRecordFullName(const char* aName);

template<>
[[nodiscard]] CName GetRecordFullName(CName aName);

template<>
[[nodiscard]] CName GetRecordAliasName(const std::string& aName);

template<>
[[nodiscard]] CName GetRecordAliasName(const char* aName);

template<>
[[nodiscard]] CName GetRecordAliasName(CName aName);

template<typename T>
T GetRecordShortName(const std::string& aName);

template<typename T>
T GetRecordShortName(CName aName);

template<typename T>
T GetRecordShortName(const char* aName);

template<>
std::string GetRecordShortName(const std::string& aName);

template<>
std::string GetRecordShortName(CName aName);

template<>
std::string GetRecordShortName(const char* aName);

template<>
CName GetRecordShortName(const std::string& aName);

template<>
CName GetRecordShortName(CName aName);

template<>
CName GetRecordShortName(const char* aName);

std::string GetPropertyFunctionName(CName aName);

uint32_t GetRecordTypeHash(Red::CName aName);
uint32_t GetRecordTypeHash(const std::string& aName);
uint32_t GetRecordTypeHash(const char* aName);
uint32_t GetRecordTypeHash(const CClass* aType);

} // namespace Red::TweakDBUtil
