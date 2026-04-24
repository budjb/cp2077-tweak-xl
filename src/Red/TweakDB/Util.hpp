#pragma once

namespace Red::TweakDBUtil
{
CBaseRTTIType* GetFlatType(uint64_t aType);
CBaseRTTIType* GetFlatType(CName aTypeName);
CClass* GetRecordType(CName aTypeName);
CClass* GetRecordType(const char* aTypeName);

CBaseRTTIType* GetArrayType(CName aTypeName);
CBaseRTTIType* GetArrayType(const CBaseRTTIType* aType);

CBaseRTTIType* GetElementType(CName aTypeName);
CBaseRTTIType* GetElementType(const CBaseRTTIType* aType);

bool IsFlatType(CName aTypeName);
bool IsFlatType(const CBaseRTTIType* aType);

bool IsRecordType(CName aTypeName);
bool IsRecordType(const CClass* aType);

bool IsArrayType(CName aTypeName);
bool IsArrayType(const CBaseRTTIType* aType);

bool IsForeignKey(CName aTypeName);
bool IsForeignKey(const CBaseRTTIType* aType);

bool IsForeignKeyArray(CName aTypeName);
bool IsForeignKeyArray(const CBaseRTTIType* aType);

bool IsResRefToken(CName aTypeName);
bool IsResRefToken(const CBaseRTTIType* aType);

bool IsResRefTokenArray(CName aTypeName);
bool IsResRefTokenArray(const CBaseRTTIType* aType);

CName GetArrayTypeName(CName aTypeName);
CName GetArrayTypeName(const CBaseRTTIType* aType);

CName GetElementTypeName(CName aTypeName);
CName GetElementTypeName(const CBaseRTTIType* aType);

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
std::string GetRecordShortName(const std::string& aName);
std::string GetRecordShortName(CName aName);
std::string GetRecordShortName(const char* aName);

std::string GetPropertyFunctionName(CName aName);

uint32_t GetRecordTypeHash(Red::CName aName);
uint32_t GetRecordTypeHash(const std::string& aName);
uint32_t GetRecordTypeHash(const char* aName);
uint32_t GetRecordTypeHash(const CClass* aType);

InstancePtr<> Construct(CName aTypeName);
InstancePtr<> Construct(const CBaseRTTIType* aType);

} // namespace Red::TweakDBUtil
