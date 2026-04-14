#pragma once

#include "App/Tweaks/Batch/TweakChangelog.hpp"
#include "Core/Logging/LoggingAgent.hpp"
#include "Red/TweakDB/Manager.hpp"

namespace App
{

class CClassProxy
{
public:
    CClassProxy() = default;

    explicit CClassProxy(const Red::CClass* aType)
        : name(aType->GetName().ToString())
        , cname(aType->GetName())
        , type(aType)
        , parent(aType->parent ? aType->parent->GetName() : Red::CName())
    {
    }

    explicit CClassProxy(const std::string& aName)
        : name(aName)
        , cname(aName.c_str())
    {

    }

    std::string name{};
    Red::CName cname{};
    const Red::CClass* type = nullptr;
    Red::CName parent{};

    CClassProxy& operator=(const Red::CClass* aRhs) noexcept
    {
        *this = CClassProxy(aRhs);
        return *this;
    }

    CClassProxy& operator=(const std::string& aRhs) noexcept
    {
        *this = CClassProxy(aRhs);
        return *this;
    }

    operator Red::CName() const
    {
        return cname;
    }

    operator const std::string&() const
    {
        return name;
    }

    operator const Red::CClass*() const
    {
        return type;
    }

    operator bool() const
    {
        return !cname.IsNone();
    }
};

class TweakChangeset
    : public Core::LoggingAgent
    , public Core::ShareFromThis<TweakChangeset>
{
public:
    struct FlatEntry
    {
        const Red::CBaseRTTIType* type;
        Red::InstancePtr<> value;
    };

    struct RecordSchemaEntry
    {
        Red::TweakDBRecordInfo recordInfo;
    };

    struct RecordEntry
    {
        const CClassProxy* type;
        Red::TweakDBID sourceId;
    };

    struct InsertionEntry
    {
        const Red::CBaseRTTIType* type;
        Red::InstancePtr<> value;
        bool unique;
    };

    struct DeletionEntry
    {
        const Red::CBaseRTTIType* type; // TODO: this is unused
        Red::InstancePtr<> value;
    };

    struct MergingEntry
    {
        Red::TweakDBID sourceId;
    };

    struct MutationEntry
    {
        Core::Vector<DeletionEntry> deletions;
        Core::Vector<InsertionEntry> appendings;
        Core::Vector<InsertionEntry> prependings;
        Core::Vector<MergingEntry> appendingMerges;
        Core::Vector<MergingEntry> prependingMerges;
        Red::TweakDBID baseId;
        bool deleteAll;
    };

    struct ReinheritanceEntry
    {
        Red::TweakDBID sourceId;
        std::string appendix;
    };

    bool SetFlat(Red::TweakDBID aFlatId, const Red::CBaseRTTIType* aType, const Red::InstancePtr<>& aValue);
    bool ReinheritFlat(Red::TweakDBID aFlatId, Red::TweakDBID aSourceId, const std::string& aAppendix);

    bool MakeRecord(Red::TweakDBID aRecordId, const Red::CClass* aType, Red::TweakDBID aSourceId = {});
    bool UpdateRecord(Red::TweakDBID aRecordId);

    bool AppendElement(Red::TweakDBID aFlatId, const Red::CBaseRTTIType* aType,
                       const Red::InstancePtr<>& aValue, bool aUnique = false);
    bool PrependElement(Red::TweakDBID aFlatId, const Red::CBaseRTTIType* aType,
                        const Red::InstancePtr<>& aValue, bool aUnique = false);
    bool RemoveElement(Red::TweakDBID aFlatId, const Red::CBaseRTTIType* aType,
                       const Red::InstancePtr<>& aValue);
    bool RemoveAllElements(Red::TweakDBID aFlatId);
    bool AppendFrom(Red::TweakDBID aFlatId, Red::TweakDBID aSourceId);
    bool PrependFrom(Red::TweakDBID aFlatId, Red::TweakDBID aSourceId);

    bool RegisterName(Red::TweakDBID aId, const std::string& aName);

    const FlatEntry* GetFlat(Red::TweakDBID aFlatId);
    const RecordEntry* GetRecord(Red::TweakDBID aRecordId);
    const Red::CClass* GetRecordType(Red::TweakDBID aRecordId);
    bool HasRecord(Red::TweakDBID aRecordId);

    bool IsEmpty() const;

    void Commit(const Core::SharedPtr<Red::TweakDBManager>& aManager,
                const Core::SharedPtr<TweakChangelog>& aChangelog);

    void AddClass(const CClassProxy& aClass);
    const CClassProxy* GetClass(const std::string& aName);
    const CClassProxy* GetClass(const Red::CClass* aClass);
    const CClassProxy* GetClass(const Red::CName& aName);

private:
    using ElementChange = std::pair<int32_t, Core::SharedPtr<void>>;

    struct ElementIndexComparator
    {
        bool operator()(const ElementChange& a, const ElementChange& b) const
        {
            return a.first < b.first;
        }
    };

    bool IsCommitFinished();
    void StartCommitJob();
    void FinishCommitJob();

    template<typename J>
    void StartAsyncCommitJob(J&& aJob)
    {
        StartCommitJob();
        Red::JobQueue jobQueue;
        jobQueue.Dispatch(std::forward<J>(aJob));
        jobQueue.Dispatch([self = ToShared()]{ self->FinishCommitJob(); });
    }

    static int32_t FindElement(const Red::CRTTIArrayType* aArrayType, void* aArray, void* aValue);
    static bool InArray(const Red::CRTTIArrayType* aArrayType, void* aArray, void* aValue);
    static bool IsSkip(const Red::CRTTIArrayType* aArrayType, void* aValue, int32_t aLevel,
                       const Core::Vector<ElementChange>& aChanges);

    Core::WeakPtr<TweakChangeset> m_self;
    Core::Vector<Red::TweakDBID> m_orderedRecords;
    Core::Map<Red::TweakDBID, RecordEntry> m_pendingRecords;
    Core::Map<Red::TweakDBID, MutationEntry> m_pendingMutations;
    Core::Map<Red::TweakDBID, FlatEntry> m_pendingFlats;
    Core::Map<Red::TweakDBID, ReinheritanceEntry> m_reinheritedProps;
    Core::Map<Red::TweakDBID, std::string> m_pendingNames;
    Core::Map<Red::CName, RecordSchemaEntry> m_pendingSchemas;

    std::mutex m_commitMutex;
    int32_t m_totalCommitChunks{0};
    int32_t m_finishedCommitChunks{0};

    std::mutex m_cclassProxyMutex;
    Core::Map<Red::CName, CClassProxy> m_classProxies{};
};
}
