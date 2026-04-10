#include "CustomTweakRecord.hpp"
#include "App/Tweaks/TweakService.hpp"
#include "Core/Facades/Container.hpp"

namespace App::Schema
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

    if (auto* schema = Core::Resolve<App::TweakService>()->GetSchemaRegistry().GetSchema(type->GetName()))
    {
        return schema->GetHash();
    }

    return 0;
}

} // namespace App::Record
