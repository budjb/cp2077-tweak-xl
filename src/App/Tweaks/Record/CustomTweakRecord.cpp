#include "CustomTweakRecord.hpp"
#include "Core/Facades/Container.hpp"
#include "CustomRecordService.hpp"

namespace App::Record
{

Red::CClass* CustomTweakRecord::GetNativeType()
{
    if (nativeType)
    {
        return nativeType;
    }
    return Red::CRTTISystem::Get()->GetClass(NAME);
}

uint32_t CustomTweakRecord::GetTweakBaseHash() const
{
    const auto* type = const_cast<CustomTweakRecord*>(this)->GetType();

    if (!type)
    {
        return 0;
    }

    if (auto* schema = Core::Resolve<CustomRecordService>()->GetSchema(type->GetName()))
    {
        return schema->GetTweakBaseHash();
    }

    return 0;
}

} // namespace App::Record
