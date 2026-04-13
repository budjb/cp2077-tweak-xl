#include "CustomTweakRecord.hpp"

#include "Red/TweakDB/Reflection.hpp"

namespace
{
using namespace Red;

constexpr ClassLocator<App::CustomTweakRecord> s_customTweakRecordType;

uint32_t GetTweakBaseHash(const App::CustomTweakRecord* aRecord)
{
    return TweakDBReflection::GetRecordTypeHash(TweakDBReflection::GetRecordShortName<std::string>(
        const_cast<App::CustomTweakRecord*>(aRecord)->GetType()->GetName()));
}

} // namespace

namespace App
{

using namespace Red;

CClass* CustomTweakRecord::GetNativeType()
{
    return s_customTweakRecordType;
}

uint32_t CustomTweakRecord::GetTweakBaseHash() const
{
    if (!m_tweakBaseHash)
    {
        m_tweakBaseHash = ::GetTweakBaseHash(this);
    }
    return m_tweakBaseHash;
}

} // namespace App
