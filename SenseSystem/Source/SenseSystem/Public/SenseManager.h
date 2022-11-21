//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Containers/Map.h"
#include "Templates/UniquePtr.h"
#include "Stats/Stats2.h"
#include "Tickable.h"
#include "UObject/NoExportTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "SenseSysHelpers.h"
#include "SensedStimulStruct.h"
#include "Sensors/SensorBase.h"


#include "SenseManager.generated.h"


struct FStimulusTagResponse;
class USenseStimulusBase;
class USenseReceiverComponent;
class IContainerTree;
class UObject;

/**
* RegisteredSensorTags struct
*/
struct SENSESYSTEM_API FRegisteredSensorTags : FNoncopyable
{
	FRegisteredSensorTags();
	~FRegisteredSensorTags();


	bool AddSenseStimulus(USenseStimulusBase* Ssc);
	bool RemoveSenseStimulus(USenseStimulusBase* Ssc);

	bool Add_SenseStimulus(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str);
	bool Remove_SenseStimulus(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str);

	const IContainerTree* GetContainerTree(const FName& Tag) const;
	IContainerTree* GetContainerTree(const FName& Tag);

	void Empty();
	void CollapseAllTrees();
	bool IsValidTag(const FName& SensorTag) const;
	const TMap<FName, TUniquePtr<IContainerTree>>& GetMap() const { return SenseRegChannels; }

private:
	TMap<FName, TUniquePtr<IContainerTree>> SenseRegChannels;

	bool AddSenseStimulus_Internal(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str);
	bool RemoveSenseStimulus_Internal(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str);

	static TUniquePtr<IContainerTree> MakeTree(const FName Tag);

	//todo you need to make sure that a new element is not added when we take another
	mutable FCriticalSection CriticalSection;

	void Empty_Internal();
};


/**
* Class for:
* managing all sensing components
* Communication between all sensing components
* Managing Sense Thread
*/
UCLASS(BlueprintType, ClassGroup = (SenseSystem))
class SENSESYSTEM_API USenseManager
	: public UWorldSubsystem
	, public FTickableGameObject
{
	GENERATED_BODY()
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReport_StimulusEvent, uint16, StimulusID, FName, SensorTag);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjStatusChanged, UObject*, Obj);

	USenseManager();
	virtual ~USenseManager() override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void BeginDestroy() override;
	virtual void Cleanup();

	void OnWorldCleanup(class UWorld* World, bool bSessionEnded, bool bCleanupResources);


	/**static SenseManager getter*/
	static USenseManager* GetSenseManager(const UObject* WorldContext);


public:
	FTickingTimer TickingTimer = FTickingTimer(0.5f);

	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual bool IsTickable() const override { return true; /*GetWorld();*/ }
	virtual bool IsTickableInEditor() const override { return false; }
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USenseManager, STATGROUP_Tickables); };

public:
	bool RequestAsyncSenseUpdate(USensorBase* InSensor, bool bHighPriority) const;


	IContainerTree* GetNamedContainerTree(const FName SensorTag) { return RegisteredSensorTags.GetContainerTree(SensorTag); }
	const IContainerTree* GetNamedContainerTree(const FName SensorTag) const { return RegisteredSensorTags.GetContainerTree(SensorTag); }

	UFUNCTION(BlueprintCallable, Category = "QuadTree")
	FBox GetIntersectTreeBox(FName SensorTag, FBox InBox);


	UFUNCTION(BlueprintCallable, Category = "QuadTree")
	void DrawTree(FName SensorTag, FDrawElementSetup TreeNode, FDrawElementSetup Link, FDrawElementSetup ElemNode, float LifeTime) const;

	/*********************************************/

	/**Delegate for Communication passive sense events "Event channel"*/
	UPROPERTY(/*BlueprintAssignable*/)
	FReport_StimulusEvent ReportStimulus_Event;

	/** Delegate UnregisterStimulus */
	UPROPERTY(BlueprintAssignable)
	FOnObjStatusChanged On_UnregisterStimulus;

	/** Delegate UnregisterReceiver */
	UPROPERTY(BlueprintAssignable)
	FOnObjStatusChanged On_UnregisterReceiver;

	UPROPERTY()
	TSet<USenseReceiverComponent*> Receivers;


	bool RegisterSenseReceiver(USenseReceiverComponent* Receiver);
	bool UnRegisterSenseReceiver(USenseReceiverComponent* Receiver);

	void Add_ReceiverSensor(USenseReceiverComponent* Receiver, USensorBase* Sensor, const bool bThreadCountUpdt);
	void Remove_ReceiverSensor(USenseReceiverComponent* Receiver, USensorBase* Sensor, const bool bThreadCountUpdt);


	bool RegisterSenseStimulus(USenseStimulusBase* Stimulus);
	bool UnRegisterSenseStimulus(USenseStimulusBase* Stimulus);

	void Add_SenseStimulus(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str);
	void Remove_SenseStimulus(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str);


	void ChangeSensorThreadType(const USenseReceiverComponent* Receiver, const ESensorThreadType NewSensorThreadType, const ESensorThreadType OldSensorThreadType);

	bool HaveReceivers() const;
	bool HaveSenseStimulus() const;

	bool IsHaveStimulusTag(const FName Tag) const;
	bool IsHaveReceiverTag(const FName Tag) const;

#if WITH_EDITORONLY_DATA
	UPROPERTY(BlueprintReadWrite, Category = "SenseSystem|SenseManager")
	bool bSenseThreadPauseLog = false;
	UPROPERTY(BlueprintReadWrite, Category = "SenseManager|SenseSystem")
	bool bSenseThreadStateLog = true;
#endif

protected:
	/**StimulusContainer*/
	FRegisteredSensorTags RegisteredSensorTags;

	bool SenseThread_CreateIfNeed();
	bool SenseThread_DeleteIfNeed();

	/**Destroy Sense Thread*/
	void Close_SenseThread();

private:
	double WaitTime = 0.0001f;
	int32 CounterLimit = 10;

	/**Receivers with ContainsThread counter*/
	uint32 ContainsThreadCount = 0;

	TMap<FName, int32> TagReceiversCount;

	uint32 StimulusCount = 0;

	/** Deleter for forward class FSenseRunnable */
	struct FDeleterSR
	{
		FDeleterSR() = default;
		FDeleterSR(const FDeleterSR&) = default;
		~FDeleterSR() = default;
		FDeleterSR& operator=(const FDeleterSR&) = default;
		void operator()(class FSenseRunnable* Ptr) const;
		template<typename U, typename = typename TEnableIf<TPointerIsConvertibleFromTo<U, class FSenseRunnable>::Value>::Type>
		FDeleterSR(const TDefaultDelete<U>&)
		{}
		template<typename U, typename = typename TEnableIf<TPointerIsConvertibleFromTo<U, class FSenseRunnable>::Value>::Type>
		FDeleterSR& operator=(const TDefaultDelete<U>&)
		{
			return *this;
		}
	};
	/**SenseThread ptr*/
	TUniquePtr<class FSenseRunnable, FDeleterSR> SenseThread = nullptr;

	/**Create Sense Thread*/
	void Create_SenseThread();

#if WITH_EDITORONLY_DATA
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SenseSystem|SenseManager")
	FSenseSysDebugDraw SenseSysDebugDraw;

	FSenseSysDebugDraw GetSenseSysDebugDraw(const FName& SensorTag = NAME_None) const;
	void SetSenseSysDebugDraw(FSenseSysDebugDraw DebugDrawParam);

	void DebugDrawSenseSys(const class FSceneView* View, class FPrimitiveDrawInterface* PDI) const;
	void DebugDrawSenseSysHUD(const class FViewport* Viewport, const class FSceneView* View, class FCanvas* Canvas) const;

#endif
};

FORCEINLINE bool FRegisteredSensorTags::IsValidTag(const FName& SensorTag) const
{
	return SenseRegChannels.Contains(SensorTag);
}

FORCEINLINE bool USenseManager::IsHaveStimulusTag(const FName Tag) const
{
	return RegisteredSensorTags.IsValidTag(Tag);
}
FORCEINLINE bool USenseManager::IsHaveReceiverTag(const FName Tag) const
{
	return TagReceiversCount.Contains(Tag);
}

FORCEINLINE void USenseManager::Add_SenseStimulus(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str)
{
	RegisteredSensorTags.Add_SenseStimulus(Ssc, SensorTag, Str);
}
FORCEINLINE void USenseManager::Remove_SenseStimulus(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str)
{
	RegisteredSensorTags.Remove_SenseStimulus(Ssc, SensorTag, Str);
}

FORCEINLINE bool USenseManager::HaveReceivers() const
{
	return Receivers.Num() > 0;
}
FORCEINLINE bool USenseManager::HaveSenseStimulus() const
{
	return RegisteredSensorTags.GetMap().Num() > 0;
}

FORCEINLINE bool USenseManager::SenseThread_CreateIfNeed()
{
	if (!SenseThread.IsValid() && ContainsThreadCount != 0 && HaveSenseStimulus())
	{
		Create_SenseThread();
		return true;
	}
	return false;
}

FORCEINLINE bool USenseManager::SenseThread_DeleteIfNeed() //
{
	if (ContainsThreadCount == 0 || !HaveSenseStimulus())
	{
		Close_SenseThread();
		return true;
	}
	return false;
}
