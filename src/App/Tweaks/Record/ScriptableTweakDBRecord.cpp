#include "App/Tweaks/Record/ScriptableTweakDBRecord.hpp"

#include "App/Tweaks/Record/ScriptableRecordManager.hpp"

namespace Red
{

ScriptableTweakDBRecord::ScriptableTweakDBRecord(const App::RecordClass* aClass)
{
    this->nativeType = const_cast<App::RecordClass*>(aClass);
}

void ScriptableTweakDBRecord::sub_108()
{
}

Red::CClass* ScriptableTweakDBRecord::GetNativeType()
{
    return this->nativeType;
}

uint32_t ScriptableTweakDBRecord::GetTweakBaseHash() const
{
    return reinterpret_cast<App::RecordClass*>(this->nativeType)->tweakBaseHash;
}

} // namespace Red
