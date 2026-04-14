#include "CustomTweakDBRecord.hpp"

#include "Red/TweakDB/Reflection.hpp"

namespace
{
constexpr Red::ClassLocator<App::CustomTweakDBRecord> s_CustomTweakDBRecordType;
} // namespace

namespace App
{

using namespace Red;

CustomTweakDBRecord::CustomTweakDBRecord(const TweakDBRecordInfo& aRecordInfo, const TweakDBID aTweakDBID)
    : m_tweakBaseHash(aRecordInfo.GetTypeHash())
{
    this->recordID = aTweakDBID;
}

CClass* CustomTweakDBRecord::GetNativeType()
{
    return s_CustomTweakDBRecordType;
}

uint32_t CustomTweakDBRecord::GetTweakBaseHash() const
{
    return m_tweakBaseHash;
}

} // namespace App
