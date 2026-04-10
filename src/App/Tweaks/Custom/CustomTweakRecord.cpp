#include "CustomTweakRecord.hpp"

namespace App {


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
    if (const auto* type = const_cast<CustomTweakRecord*>(this)->GetType(); !type)
    {
        return 0;
    }

    // if (auto* schema = Core::Resolve<App::TweakService>()->GetSchemaRegistry().GetSchema(type->GetName()))
    // {
    //     return schema->GetHash();
    // }

    return 0;
}

} // App
