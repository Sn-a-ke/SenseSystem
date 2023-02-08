//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "SenseManager.h"

#include "SenseSystem.h"
#include "SenseSysSettings.h"
#include "Sensors/SensorBase.h"
#include "SenseReceiverComponent.h"
#include "SenseStimulusBase.h"
#include "HashSorted.h"
#include "QtOtContainer.h"

//#include "UObject/UObjectGlobals.h"
#include "Async/ParallelFor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#include "DrawDebugHelpers.h"


TUniquePtr<IContainerTree> FRegisteredSensorTags::MakeTree(const FName Tag)
{
	const auto Settings = GetDefault<USenseSysSettings>();
	check(Settings);
	const TMap<FName, FSensorTagSettings>& TagSettings = Settings->SensorTagSettings;
	if (const FSensorTagSettings* STagSettings = TagSettings.Find(Tag))
	{
		const float MinSize = STagSettings->MinimumQuadTreeSize;
		const int32 NodeCantSplit = STagSettings->NodeCantSplit;
		const ESenseSys_QtOtSwitch QtOtSwitch = STagSettings->QtOtSwitch;
		switch (QtOtSwitch)
		{
			case ESenseSys_QtOtSwitch::OcTree: return MakeUnique<FSenseSys_OcTree>(MinSize);
			case ESenseSys_QtOtSwitch::QuadTree: return MakeUnique<FSenseSys_QuadTree>(MinSize);
		}
	}
	return MakeUnique<FSenseSys_OcTree>(500.f);
}


FRegisteredSensorTags::FRegisteredSensorTags() = default;
FRegisteredSensorTags::~FRegisteredSensorTags()
{
	Empty_Internal();
}

bool FRegisteredSensorTags::IsValidTag(const FName& SensorTag) const
{
	return SenseRegChannels.Contains(SensorTag);
}

void FRegisteredSensorTags::CollapseAllTrees()
{
	for (const auto& It : SenseRegChannels)
	{
		check(It.Value.Get());
		It.Value.Get()->Collapse();
	}
}


bool FRegisteredSensorTags::AddSenseStimulus_Internal(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str)
{
	if (SensorTag != NAME_None && Str.BitChannels.Value != 0 && Ssc->GetWorld())
	{
		IContainerTree* ContainerTree = GetContainerTree(SensorTag);
		if (ContainerTree == nullptr)
		{
			ContainerTree = SenseRegChannels.Add(SensorTag, MakeTree(SensorTag)).Get();
		}
		check(ContainerTree);
		{
			const float CurrentTime = Ssc->GetWorld()->GetTimeSeconds();
			FSensedStimulus NewElem;
			const FBox Box = NewElem.Init(SensorTag, Ssc, Str.Score, Str.Age, CurrentTime, Str.BitChannels.Value);
			if (NewElem.TmpHash != MAX_uint32 && NewElem.StimulusComponent.IsValid())
			{
				const uint16 ObjID = ContainerTree->Insert(MoveTemp(NewElem), Box);
				Str.SetObjID(ObjID);
				check(Str.GetObjID() != MAX_uint16);
				Str.ContainerTree = ContainerTree;
			}
		}
		return true;
	}
	return false;
}

bool FRegisteredSensorTags::RemoveSenseStimulus_Internal(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str)
{
	if (SensorTag != NAME_None && Str.BitChannels.Value != 0)
	{
		if (IContainerTree* ContainerTree = GetContainerTree(SensorTag))
		{
			ContainerTree->Remove(Str.GetObjID());

			Str.SetObjID(MAX_uint16);
			Str.ContainerTree = nullptr;
			if (ContainerTree->Num() <= 0)
			{
				return SenseRegChannels.Remove(SensorTag) != 0;
			}
		}
	}
	return false;
}


bool FRegisteredSensorTags::AddSenseStimulus(USenseStimulusBase* Ssc)
{
	bool bDone = false;
	if (IsValid(Ssc))
	{
		FScopeLock ScopeLock(&CriticalSection);

		for (auto& It : Ssc->TagResponse)
		{
			const bool bSuccess = AddSenseStimulus_Internal(Ssc, It.Key, It.Value);
			bDone = bSuccess || bDone;
		}
	}
	return bDone;
}

bool FRegisteredSensorTags::RemoveSenseStimulus(USenseStimulusBase* Ssc)
{
	bool bDone = false;
	if (Ssc)
	{
		FScopeLock ScopeLock(&CriticalSection);
		if (GetMap().Num())
		{
			for (auto& It : Ssc->TagResponse)
			{
				const bool bSuccess = RemoveSenseStimulus_Internal(Ssc, It.Key, It.Value);
				bDone = bSuccess || bDone;
			}
		}
	}
	return bDone;
}

bool FRegisteredSensorTags::Add_SenseStimulus(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str)
{
	if (Ssc)
	{
		FScopeLock ScopeLock(&CriticalSection);
		return AddSenseStimulus_Internal(Ssc, SensorTag, Str);
	}
	return false;
}

bool FRegisteredSensorTags::Remove_SenseStimulus(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str)
{
	if (Ssc)
	{
		FScopeLock ScopeLock(&CriticalSection);
		return RemoveSenseStimulus_Internal(Ssc, SensorTag, Str);
	}
	return false;
}

const IContainerTree* FRegisteredSensorTags::GetContainerTree(const FName& Tag) const
{
	if (Tag != NAME_None)
	{
		FScopeLock ScopeLock(&CriticalSection);
		if (const TUniquePtr<IContainerTree>* Ptr = SenseRegChannels.Find(Tag))
		{
			return (*Ptr).Get();
		}
	}
	return nullptr;
}

IContainerTree* FRegisteredSensorTags::GetContainerTree(const FName& Tag)
{
	if (Tag != NAME_None)
	{
		FScopeLock ScopeLock(&CriticalSection);
		if (const TUniquePtr<IContainerTree>* Ptr = SenseRegChannels.Find(Tag))
		{
			return (*Ptr).Get();
		}
	}
	return nullptr;
}

void FRegisteredSensorTags::Empty()
{
	FScopeLock ScopeLock(&CriticalSection);

	Empty_Internal();
}
void FRegisteredSensorTags::Empty_Internal()
{
	SenseRegChannels.Empty();
}
void FRegisteredSensorTags::Remove(const FName SensorTag)
{
	SenseRegChannels.Remove(SensorTag);
}


/*******************/


USenseManager::USenseManager()
{
	if (const auto Settings = GetDefault<USenseSysSettings>())
	{
		WaitTime = Settings->WaitTimeBetweenCyclesUpdate;
		CounterLimit = Settings->CountPerOneCyclesUpdate;
	}
	FCoreDelegates::PostWorldOriginOffset.AddUObject(this, &USenseManager::PostWorldOriginOffsetUpdt);
	FCoreDelegates::PreWorldOriginOffset.AddUObject(this, &USenseManager::PreWorldOriginOffsetUpdt);
}
USenseManager::USenseManager(FVTableHelper& Helper)
{
	if (const auto Settings = GetDefault<USenseSysSettings>())
	{
		WaitTime = Settings->WaitTimeBetweenCyclesUpdate;
		CounterLimit = Settings->CountPerOneCyclesUpdate;
	}
	FCoreDelegates::PostWorldOriginOffset.AddUObject(this, &USenseManager::PostWorldOriginOffsetUpdt);
	FCoreDelegates::PreWorldOriginOffset.AddUObject(this, &USenseManager::PreWorldOriginOffsetUpdt);
}

USenseManager::~USenseManager()
{
	USenseManager::Cleanup();
}

void USenseManager::Cleanup()
{
	Close_SenseThread();

	RegisteredSensorTags.Empty();
	Receivers.Empty();

	ReportStimulus_Event.Clear();
	On_UnregisterStimulus.Clear();
	On_UnregisterReceiver.Clear();

	StimulusCount = 0;
	ContainsThreadCount = 0;
}

void USenseManager::BeginDestroy()
{
	Close_SenseThread();
	Super::BeginDestroy();
}


void USenseManager::Add_SenseStimulus(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str)
{
	RegisteredSensorTags.Add_SenseStimulus(Ssc, SensorTag, Str);
}
void USenseManager::Remove_SenseStimulus(USenseStimulusBase* Ssc, const FName& SensorTag, FStimulusTagResponse& Str)
{
	RegisteredSensorTags.Remove_SenseStimulus(Ssc, SensorTag, Str);
}

bool USenseManager::IsHaveStimulusTag(const FName Tag) const
{
	return RegisteredSensorTags.IsValidTag(Tag);
}
bool USenseManager::IsHaveReceiverTag(const FName Tag) const
{
	return TagReceiversCount.Contains(Tag);
}

bool USenseManager::HaveReceivers() const
{
	return Receivers.Num() > 0;
}
bool USenseManager::HaveSenseStimulus() const
{
	return RegisteredSensorTags.GetMap().Num() > 0;
}

bool USenseManager::SenseThread_CreateIfNeed()
{
	if (!SenseThread.IsValid() && ContainsThreadCount != 0 && HaveSenseStimulus())
	{
		Create_SenseThread();
		return true;
	}
	return false;
}
bool USenseManager::SenseThread_DeleteIfNeed() //
{
	if (ContainsThreadCount == 0 || !HaveSenseStimulus())
	{
		Close_SenseThread();
		return true;
	}
	return false;
}


/* static */
USenseManager* USenseManager::GetSenseManager(const UObject* WorldContext)
{
	if (WorldContext)
	{
		if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull))
		{
			return Cast<USenseManager>(World->GetSubsystemBase(USenseManager::StaticClass()));
		}
	}
	return nullptr;
}

void USenseManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	//static delegate to detect world change
	FWorldDelegates::OnWorldCleanup.AddUObject(this, &USenseManager::OnWorldCleanup);
	//FWorldDelegates::OnPostWorldCreation.AddUObject(this, &USenseManager::OnWorldCreated);
	//FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &USenseManager::OnPostWorldInitialization);

	if (const auto Settings = GetDefault<USenseSysSettings>())
	{
		WaitTime = Settings->WaitTimeBetweenCyclesUpdate;
		CounterLimit = Settings->CountPerOneCyclesUpdate;
	}
}

void USenseManager::Deinitialize()
{
	Super::Deinitialize();
}


bool USenseManager::RegisterSenseStimulus(USenseStimulusBase* Stimulus)
{
	if (IsValid(Stimulus))
	{
		const bool bSuccess = RegisteredSensorTags.AddSenseStimulus(Stimulus);
		if (bSuccess)
		{
			StimulusCount++;
			On_UnregisterReceiver.AddUniqueDynamic(Stimulus, &USenseStimulusBase::OnUnregisterReceiver);
			SenseThread_CreateIfNeed();
		}
		return bSuccess;
	}
	return false;
}

bool USenseManager::UnRegisterSenseStimulus(USenseStimulusBase* Stimulus)
{
	if (Stimulus && StimulusCount > 0)
	{
		if (On_UnregisterStimulus.IsBound())
		{
			On_UnregisterStimulus.Broadcast(Stimulus); //Execute UnregisterStimulus
		}

		On_UnregisterReceiver.RemoveDynamic(Stimulus, &USenseStimulusBase::OnUnregisterReceiver);

		const bool bSuccess = RegisteredSensorTags.RemoveSenseStimulus(Stimulus);
		if (bSuccess)
		{
			--StimulusCount;
		}

		SenseThread_DeleteIfNeed();
		return bSuccess;
	}
	checkNoEntry();
	return false;
}


bool USenseManager::RegisterSenseReceiver(USenseReceiverComponent* Receiver)
{
	if (IsValid(Receiver))
	{
		Receivers.Add(Receiver);
		for (uint8 i = 1; i < 4; i++)
		{
			const auto& SensorsArr = Receiver->GetSensorsByType(static_cast<ESensorType>(i));
			for (const auto It : SensorsArr)
			{
				if (It)
				{
					int32& CountPtr = TagReceiversCount.FindOrAdd(It->SensorTag);
					++CountPtr;
				}
			}
		}
		if (Receiver->ContainsSenseThreadSensors())
		{
			ContainsThreadCount++;
		}

		ReportStimulus_Event.AddUniqueDynamic(Receiver, &USenseReceiverComponent::BindOnReportStimulusEvent);
		On_UnregisterStimulus.AddUniqueDynamic(Receiver, &USenseReceiverComponent::OnUnregisterStimulus); // Bind UnregisterStimulus in Receiver

		SenseThread_CreateIfNeed();
		return true;
	}
	return false;
}

bool USenseManager::UnRegisterSenseReceiver(USenseReceiverComponent* Receiver)
{
	if (Receiver && Receivers.Num() > 0)
	{
		for (uint8 i = 1; i < 4; i++)
		{
			const auto& SensorsArr = Receiver->GetSensorsByType(static_cast<ESensorType>(i));
			for (const auto It : SensorsArr)
			{
				if (It)
				{
					if (int32* CountPtr = TagReceiversCount.Find(It->SensorTag))
					{
						int32& Count = *CountPtr;
						--Count;
						if (Count == 0)
						{
							TagReceiversCount.Remove(It->SensorTag);
						}
					}
				}
			}
		}

		if (ContainsThreadCount && Receiver->ContainsSenseThreadSensors())
		{
			ContainsThreadCount--;
		}

		if (On_UnregisterReceiver.IsBound())
		{
			On_UnregisterReceiver.Broadcast(Receiver);
		}

		ReportStimulus_Event.RemoveDynamic(Receiver, &USenseReceiverComponent::BindOnReportStimulusEvent);
		On_UnregisterStimulus.RemoveDynamic(Receiver, &USenseReceiverComponent::OnUnregisterStimulus);
		Receivers.Remove(Receiver);

		SenseThread_DeleteIfNeed();
		return true;
	}
	return false;
}

void USenseManager::Add_ReceiverSensor(USenseReceiverComponent* Receiver, USensorBase* Sensor, const bool bThreadCountUpdt)
{
	if (Sensor && Receiver)
	{
		if (!Receivers.Contains(Receiver))
		{
			RegisterSenseReceiver(Receiver);
		}
		else
		{
			if (bThreadCountUpdt)
			{
				++ContainsThreadCount;
				checkSlow(
					Sensor->SensorThreadType == ESensorThreadType::Sense_Thread || Sensor->SensorThreadType == ESensorThreadType::Sense_Thread_HighPriority)
			}

			int32& CountPtr = TagReceiversCount.FindOrAdd(Sensor->SensorTag);
			++CountPtr;

			SenseThread_CreateIfNeed();
		}
	}
}
void USenseManager::Remove_ReceiverSensor(USenseReceiverComponent* Receiver, USensorBase* Sensor, const bool bThreadCountUpdt)
{
	if (Sensor && Receiver && Receivers.Contains(Receiver))
	{
		if (ContainsThreadCount && !Receiver->ContainsSenseThreadSensors() && bThreadCountUpdt)
		{
			if (Sensor->SensorThreadType == ESensorThreadType::Sense_Thread || Sensor->SensorThreadType == ESensorThreadType::Sense_Thread_HighPriority)
			{
				--ContainsThreadCount;
			}
		}
		if (int32* CountPtr = TagReceiversCount.Find(Sensor->SensorTag))
		{
			int32& Count = *CountPtr;
			--Count;
			if (Count == 0)
			{
				TagReceiversCount.Remove(Sensor->SensorTag);
			}
		}

		SenseThread_DeleteIfNeed();
	}
}


void USenseManager::Create_SenseThread()
{
	if (!SenseThread.IsValid())
	{
		SenseThread = MakeUnique<FSenseRunnable>(WaitTime, CounterLimit);
#if WITH_EDITORONLY_DATA
		SenseThread->bSenseThreadPauseLog = bSenseThreadPauseLog;
		SenseThread->bSenseThreadStateLog = bSenseThreadStateLog;
#endif
		//SenseThread->PauseThreadDelegate.BindUObject(this, &USenseManager::Close_SenseThread);
	}
}

void USenseManager::Close_SenseThread()
{
	if (SenseThread.IsValid())
	{
		SenseThread->EnsureCompletion();
		SenseThread = nullptr;
	}
}


bool USenseManager::RequestAsyncSenseUpdate(USensorBase* InSensor, const bool bHighPriority) const
{
	if (SenseThread.IsValid() && ContainsThreadCount)
	{
		return SenseThread->AddQueueSensors(InSensor, bHighPriority);
	}

	UE_LOG(
		LogSenseSys,
		Error,
		TEXT("RequestAsyncSenseUpdate Sensor: %s, SensorOwner: %s, SenseThread IsInValid "),
		*GetNameSafe(InSensor),
		*GetNameSafe(InSensor->GetSensorOwner()));

	return false;
}

///////////////

void USenseManager::ChangeSensorThreadType(
	const USenseReceiverComponent* Receiver,
	const ESensorThreadType NewSensorThreadType,
	const ESensorThreadType OldSensorThreadType)
{
	check(NewSensorThreadType != OldSensorThreadType);

	if ((NewSensorThreadType == ESensorThreadType::Sense_Thread || NewSensorThreadType == ESensorThreadType::Sense_Thread_HighPriority) &&
		Receiver->ContainsSenseThreadSensors())
	{
		ContainsThreadCount++;
		SenseThread_CreateIfNeed();
	}

	if ((OldSensorThreadType == ESensorThreadType::Sense_Thread || OldSensorThreadType == ESensorThreadType::Sense_Thread_HighPriority) &&
		!Receiver->ContainsSenseThreadSensors())
	{
		ContainsThreadCount--;
		SenseThread_DeleteIfNeed();
	}
}


void USenseManager::OnWorldCleanup(class UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (GetWorld() == World && World->IsGameWorld())
	{
		Cleanup();
		if (!World->bIsTearingDown)
		{
			UE_LOG(LogSenseSys, Warning, TEXT("SenseManager::OnWorldCleanup:World: %s, but world not bIsTearingDown"), *GetNameSafe(World));
		}
		else
		{
			UE_LOG(LogSenseSys, Log, TEXT("SenseManager::OnWorldCleanup:World: %s"), *GetNameSafe(World));
		}
	}
}


void USenseManager::DrawTree(
	const FName SensorTag,
	const FDrawElementSetup TreeNode,
	const FDrawElementSetup Link,
	const FDrawElementSetup ElemNode,
	const float LifeTime) const
{
#if ENABLE_DRAW_DEBUG
	if (const auto World = GetWorld())
	{
		if (const IContainerTree* Tree = GetNamedContainerTree(SensorTag))
		{
			const auto _TreeNode = FTreeDrawSetup(TreeNode.Color, TreeNode.Thickness, FDrawElementSetup::ToSceneDepthPriorityGroup(TreeNode.DrawDepth));
			const auto _Link = FTreeDrawSetup(Link.Color, Link.Thickness, FDrawElementSetup::ToSceneDepthPriorityGroup(Link.DrawDepth));
			const auto _ElemNode = FTreeDrawSetup(ElemNode.Color, ElemNode.Thickness, FDrawElementSetup::ToSceneDepthPriorityGroup(ElemNode.DrawDepth));
			Tree->DrawTree(World, _TreeNode, _Link, _ElemNode, LifeTime);
		}
	}
#endif //ENABLE_DRAW_DEBUG
}


#if WITH_EDITORONLY_DATA

FSenseSysDebugDraw USenseManager::GetSenseSysDebugDraw(const FName& SensorTag /*= NAME_None*/) const
{
	if (SensorTag != NAME_None)
	{
		const TMap<FName, FSensorTagSettings>& TagSettings = GetDefault<USenseSysSettings>()->SensorTagSettings;
		const FSensorTagSettings* Settings = TagSettings.Find(SensorTag);
		if (Settings)
		{
			return Settings->SenseSysDebugDraw;
		}
	}
	return SenseSysDebugDraw;
}

void USenseManager::SetSenseSysDebugDraw(const FSenseSysDebugDraw DebugDrawParam)
{
	SenseSysDebugDraw = DebugDrawParam;
}


//todo DebugDraw SenseManager SenseSys
void USenseManager::DebugDrawSenseSys(const class FSceneView* View, class FPrimitiveDrawInterface* PDI) const
{
	/*
	for (const USenseReceiverComponent* It : Receivers)
	{
		It->DrawComponent(View, PDI);
	}

	for (const USenseStimulusBase* It : Stimulus)
	{
		It->DrawComponent(View, PDI);
	}

	*/
}

void USenseManager::DebugDrawSenseSysHUD(const class FViewport* Viewport, const class FSceneView* View, class FCanvas* Canvas) const
{}

#endif


FBox USenseManager::GetIntersectTreeBox(const FName SensorTag, const FBox InBox)
{
	if (const auto Ptr = RegisteredSensorTags.GetContainerTree(SensorTag))
	{
		return Ptr->GetMaxIntersect(InBox);
	}
	return FBox(FVector::ZeroVector, FVector::ZeroVector);
}

void USenseManager::Tick(const float DeltaTime)
{
	if (TickingTimer.TickTimer(DeltaTime))
	{
		RegisteredSensorTags.CollapseAllTrees();
	}
}


void USenseManager::PreWorldOriginOffsetUpdt(UWorld* InWorld, FIntVector OriginLocation, FIntVector NewOriginLocation)
{
	Close_SenseThread();

	FScopeLock ScopeLock(&RegisteredSensorTags.CriticalSection);

	for (USenseReceiverComponent* Receiver : Receivers)
	{
		Receiver->SetComponentTickEnabled(false);
		for (uint8 i = 1; i < 4; i++)
		{
			const auto& SensorsArr = Receiver->GetSensorsByType(static_cast<ESensorType>(i));
			for (const auto It : SensorsArr)
			{
				if (It)
				{
					It->UpdateState = ESensorState::Uninitialized;
				}
			}
		}
	}

	{ //collect USenseStimulusBase
		const TMap<FName, TUniquePtr<IContainerTree>>& Map = RegisteredSensorTags.GetMap();
		for (const auto& Pair : Map)
		{
			Pair.Value.Get()->Lock();
			if (IContainerTree* const TRee = Pair.Value.Get())
			{
				for (const FSensedStimulus& Ss : TRee->GetCompDataPool())
				{
					StimulsWorldOrigin.Add(Ss.StimulusComponent.Get());
				}
			}
			Pair.Value.Get()->UnLock();
			Pair.Value.Get()->Clear();
		}
		RegisteredSensorTags.Empty();
	}

	for (USenseStimulusBase* It : StimulsWorldOrigin)
	{
		It->SetComponentTickEnabled(false);
		It->bRegisteredForSense = false;
		//UnRegisterSenseStimulus(It);
		if (const auto OwnerActor = It->GetOwner())
		{
			if (const auto Root = OwnerActor->GetRootComponent())
			{
				Root->TransformUpdated.RemoveAll(It);
			}
		}
	}
}

void USenseManager::PostWorldOriginOffsetUpdt(UWorld* InWorld, FIntVector OriginLocation, FIntVector NewOriginLocation)
{
	for (USenseReceiverComponent* Receiver : Receivers)
	{
		Receiver->SetComponentTickEnabled(true);
		for (uint8 i = 1; i < 4; i++)
		{
			const auto& SensorsArr = Receiver->GetSensorsByType(static_cast<ESensorType>(i));
			for (const auto It : SensorsArr)
			{
				if (It)
				{
					It->InitializeForSense(Receiver);
				}
			}
		}
	}

	for (USenseStimulusBase* It : StimulsWorldOrigin)
	{
		It->bRegisteredForSense = true;
		RegisterSenseStimulus(It);

		if (const auto OwnerActor = It->GetOwner())
		{
			if (const auto Root = OwnerActor->GetRootComponent())
			{
				Root->TransformUpdated.AddUObject(It, &USenseStimulusBase::RootComponentTransformUpdated);
			}
		}

		It->SetComponentTickEnabled(true);
	}

	StimulsWorldOrigin.Empty();

	SenseThread_CreateIfNeed();
}