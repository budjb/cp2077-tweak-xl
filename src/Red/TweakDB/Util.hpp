#pragma once

namespace Red::TweakDBUtil
{
CBaseRTTIType* GetType(const uint64_t aType);
CBaseRTTIType* GetType(Red::CName aTypeName);
CBaseRTTIType* GetArrayType(Red::CName aTypeName);
CBaseRTTIType* GetArrayType(const CBaseRTTIType* aType);
CBaseRTTIType* GetElementType(Red::CName aTypeName);
CBaseRTTIType* GetElementType(const CBaseRTTIType* aType);
bool IsFlatType(Red::CName aTypeName);
bool IsFlatType(const CBaseRTTIType* aType);
bool IsArrayType(Red::CName aTypeName);
bool IsArrayType(const CBaseRTTIType* aType);
bool IsForeignKey(Red::CName aTypeName);
bool IsForeignKey(const CBaseRTTIType* aType);
bool IsForeignKeyArray(Red::CName aTypeName);
bool IsForeignKeyArray(const CBaseRTTIType* aType);
bool IsResRefToken(Red::CName aTypeName);
bool IsResRefToken(const CBaseRTTIType* aType);
bool IsResRefTokenArray(Red::CName aTypeName);
bool IsResRefTokenArray(const CBaseRTTIType* aType);
Red::CName GetArrayTypeName(Red::CName aTypeName);
Red::CName GetArrayTypeName(const CBaseRTTIType* aType);
Red::CName GetElementTypeName(Red::CName aTypeName);
Red::CName GetElementTypeName(const CBaseRTTIType* aType);
InstancePtr<> Construct(Red::CName aTypeName);
InstancePtr<> Construct(const CBaseRTTIType* aType);
ValuePtr<> ConstructValue(const CBaseRTTIType* aType);
ValuePtr<> ConstructValue(Red::CName aTypeName);

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

TweakDBID GetRTDBFlatID(CName aRecord, CName aProp);
TweakDBID GetRTDBFlatID(CName aRecord, const std::string& aProp);
TweakDBID GetRTDBFlatID(CName aRecord, const char* aProp);
TweakDBID GetRTDBRecordID(CName aRecord);

std::string Capitalize(Red::CName aName);
std::string Capitalize(const std::string& aName);
std::string Capitalize(const char* aName);
std::string Decapitalize(Red::CName aName);
std::string Decapitalize(const std::string& aName);
std::string Decapitalize(const char* aName);

} // namespace Red::TweakDBUtil
