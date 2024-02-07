//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "SenseStimulusBase.h"

#include "Templates/UnrealTemplate.h"
#include "Algo/MinElement.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"

#include "SensedStimulStruct.h"
#include "SenseManager.h"
#include "Sensors/SensorBase.h"
#include "SenseReceiverComponent.h"
#include "SenseSystemBPLibrary.h"
#include "QtOtContainer.h"
#include "HashSorted.h"

#if WITH_EDITORONLY_DATA
	#include "Engine/Engine.h"
	//#include "DrawDebugHelpers.h"
	#include "CanvasTypes.h"
	#include "SceneManagement.h"
	#include "SceneView.h"
	#include "UnrealClient.h"
	#include "UObject/Package.h"
#endif


void FStimulusTagResponse::SetAge(const float AgeValue)
{
	Age = AgeValue;
	if (ContainerTree)
	{
		check(GetObjID() != TNumericLimits<ElementIndexType>::Max());
		ContainerTree->SetAge_TS(GetObjID(), AgeValue);
	}
}

void FStimulusTagResponse::SetScore(const float ScoreValue)
{
	Score = ScoreValue;
	if (ContainerTree)
	{
		check(GetObjID() != TNumericLimits<ElementIndexType>::Max());
		ContainerTree->SetScore_TS(GetObjID(), ScoreValue);
	}
}

void FStimulusTagResponse::SetBitChannels(const uint64 NewBit)
{
	BitChannels.Value = NewBit;
	if (ContainerTree)
	{
		check(GetObjID() != TNumericLimits<ElementIndexType>::Max());
		ContainerTree->SetChannels_TS(GetObjID(), NewBit);
	}
}


void FStimulusTagResponse::UpdatePosition(const FVector& NewLocation, const TArray<FVector>& Points, const float CurrentTime, const bool bAllPoints) const
{
	if (ContainerTree)
	{
		check(GetObjID() != TNumericLimits<ElementIndexType>::Max());

		IContainerTree& CTree = *ContainerTree;
		FSensedStimulus SS = CTree.GetSensedStimulusCopy_Simple_TS(GetObjID());
		checkSlow(SS.StimulusComponent.IsValid());

		FBox NewBox(NewLocation, NewLocation);
		auto SP0 = FSensedPoint(NewLocation, SS.Score);
		bool bNeedUpdt = false;
		if (bAllPoints)
		{
			const int32 PNum = Points.Num();

			if (PNum + 1 != SS.SensedPoints.Num())
			{
				SS.SensedPoints.SetNum(PNum + 1);
				bNeedUpdt |= true;
			}

			if (bNeedUpdt || SS.SensedPoints[0] != SP0)
			{
				SS.SensedPoints[0] = MoveTemp(SP0);
				bNeedUpdt |= true;
			}

			for (int32 i = 0; i < PNum; i++)
			{
				auto SP = FSensedPoint(Points[i], SS.Score);
				if (bNeedUpdt || SS.SensedPoints[i + 1] != SP)
				{
					SS.SensedPoints[i + 1] = MoveTemp(SP);
					NewBox += Points[i];
					bNeedUpdt |= true;
				}
			}
			if (bNeedUpdt)
			{
				CTree.SetSensedPoints_TS(GetObjID(), MoveTemp(SS.SensedPoints), CurrentTime);
			}
		}
		else if (SS.SensedPoints[0] != SP0)
		{
			CTree.SetSensedPoints_TS(GetObjID(), FSensedPoint(NewLocation, SS.Score), CurrentTime);
			bNeedUpdt |= true;
		}
		if (bNeedUpdt)
		{
			CTree.Update(GetObjID(), NewBox);
		}
	}
}

bool FStimulusTagResponse::IsReceiveOnSense(const EOnSenseEvent SenseEvent) const
{
	// clang-format off
	switch (SenseEvent)
	{
		case EOnSenseEvent::SenseNew: return (ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnNewSensed));
		case EOnSenseEvent::SenseCurrent: return (ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnCurrentSensed));
		case EOnSenseEvent::SenseLost: return (ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnLost));
		case EOnSenseEvent::SenseForget: return (ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnForget));
	}
	// clang-format on
	return false;
}

bool FStimulusTagResponse::IsOnSenseStateChanged(AActor* Actor, const EOnSenseEvent OnSenseEvent, const uint8 InChannel)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_SenseStateChanged);
	switch (OnSenseEvent)
	{
		case EOnSenseEvent::SenseNew:
		{
			if (IsReceiveOnSense(EOnSenseEvent::SenseNew))
			{
				const bool bCall = TmpSensed.Num() == 0;
				AddSensed(Actor, InChannel);
				if (IsReceiveOnSense(EOnSenseEvent::SenseLost) || IsReceiveOnSense(EOnSenseEvent::SenseForget))
				{
					RemoveLost(Actor, InChannel);
				}
				return bCall;
			}
			break;
		}
		case EOnSenseEvent::SenseCurrent:
		{
			if (IsReceiveOnSense(EOnSenseEvent::SenseCurrent))
			{
				if (!IsReceiveOnSense(EOnSenseEvent::SenseNew))
				{
					AddSensed(Actor, InChannel);
					if (IsReceiveOnSense(EOnSenseEvent::SenseLost) || IsReceiveOnSense(EOnSenseEvent::SenseForget))
					{
						RemoveLost(Actor, InChannel);
					}
				}
				const bool bCall = TmpSensed.Num() != 0;
				return bCall;
			}
			break;
		}
		case EOnSenseEvent::SenseLost:
		{
			if (IsReceiveOnSense(EOnSenseEvent::SenseLost) && (IsReceiveOnSense(EOnSenseEvent::SenseNew) || IsReceiveOnSense(EOnSenseEvent::SenseCurrent)))
			{
				RemoveSensed(Actor, InChannel);
				if (IsReceiveOnSense(EOnSenseEvent::SenseForget))
				{
					AddLost(Actor, InChannel);
				}
				const bool bCall = TmpSensed.Num() == 0;
				return bCall;
			}
			break;
		}
		case EOnSenseEvent::SenseForget:
		{
			if (IsReceiveOnSense(EOnSenseEvent::SenseForget) && (IsReceiveOnSense(EOnSenseEvent::SenseNew) || IsReceiveOnSense(EOnSenseEvent::SenseCurrent)))
			{
				bool bNotRemSensed = true;
				if (!IsReceiveOnSense(EOnSenseEvent::SenseLost))
				{
					bNotRemSensed = !RemoveSensed(Actor, InChannel); //todo call forget without need SenseLost
				}
				if (bNotRemSensed)
				{
					RemoveLost(Actor, InChannel);
				}
				const bool bCall = TmpLost.Num() == 0 && TmpSensed.Num() == 0;
				return bCall;
			}
			break;
		}
		default: return false;
	}
	return false;
}


/************************************/


TArray<FVector> USenseStimulusBase::GetSensePoints(FName SensorTag) const
{
	return TArray<FVector>{};
}

FVector USenseStimulusBase::GetSingleSensePoint(FName SensorTag) const
{
	check(GetOwner());
	return GetOwner()->GetActorLocation();
}


USenseStimulusBase::USenseStimulusBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = EFPSTime_SenseSys::fps45;

	if (const auto Pawn = GetTypedOuter<class APawn>())
	{
		if (UActorComponent* MovementComponent = Pawn->GetMovementComponent())
		{
			UActorComponent::AddTickPrerequisiteComponent(MovementComponent);
		}
	}

	AddIgnoreTraceActor(GetOwner());
}

USenseStimulusBase::~USenseStimulusBase()
{}


void USenseStimulusBase::BeginDestroy()
{
	if (IsRegisteredForSense())
	{
		const UWorld* World = GetWorld();
		if (IsValid(World) && World->IsGameWorld() && World->HasBegunPlay() && !World->bIsTearingDown)
		{
			UnRegisterSelfSense();
		}
	}
	Super::BeginDestroy();
}

void USenseStimulusBase::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	const UWorld* World = GetWorld();
	if (IsValid(World) && World->IsGameWorld() && World->HasBegunPlay() && !World->bIsTearingDown)
	{
		SetEnableSenseStimulus(false);
	}
	Cleanup();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void USenseStimulusBase::SetComponentTickEnabled(const bool bTickEnabled)
{
	ResetDelayTickStart();
	if (bEnable || !bTickEnabled && !bEnable)
	{
		Super::SetComponentTickEnabled(bTickEnabled);
	}
}

void USenseStimulusBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	const UWorld* World = GetWorld();
	if (IsValid(World) && World->IsGameWorld() && World->HasBegunPlay() && !World->bIsTearingDown)
	{
		SetEnableSenseStimulus(false);
	}

	Cleanup();
	Super::EndPlay(EndPlayReason);
}

void USenseStimulusBase::Cleanup()
{
	RemoveFromRoot();
	bRegisteredForSense = false;
	OnSenseStateChanged.Clear();
	OnSensedFromSensor.Clear();
	TagResponse.Empty();
	SpecifyActorDelegateMap.Empty();
	//IgnoreTraceActors.Empty();
	SenseManager = nullptr;
}


TArray<AActor*> USenseStimulusBase::GetCurrentSensedFromActors(const FName SensorTag) const
{
	TArray<AActor*> Out;
	if (const auto Str = GetStimulusTagResponse(SensorTag))
	{
		Str->TmpSensed.GenerateKeyArray(Out);
	}
	return Out;
}

TArray<AActor*> USenseStimulusBase::GetLostSensedFromActors(const FName SensorTag) const
{
	TArray<AActor*> Out;
	if (const auto Str = GetStimulusTagResponse(SensorTag))
	{
		Str->TmpLost.GenerateKeyArray(Out);
	}
	return Out;
}

void USenseStimulusBase::ClearResponseChannels(const FName Tag)
{
	if (FStimulusTagResponse* StrPtr = GetStimulusTagResponse(Tag))
	{
		SetResponseChannels(Tag, *StrPtr, 0);
	}
}

void USenseStimulusBase::ClearAllResponseChannels()
{
	UnRegisterSelfSense();
	for (auto& Str : TagResponse)
	{
		Str.Value.BitChannels.Value = 0;
	}
}

void USenseStimulusBase::SetResponseChannel(const FName Tag, const uint8 Channel, const bool bEnableChannel)
{
	if (Tag != NAME_None && Channel > 0 && Channel <= 64)
	{
		if (FStimulusTagResponse* StrPtr = GetStimulusTagResponse(Tag))
		{
			auto& Str = *StrPtr;
			uint64 CurrentBit = Str.BitChannels.Value;

			const uint64 InBit = 1llu << (Channel - 1);
			const bool bContainsChannel = CurrentBit & InBit;

			if (bEnableChannel && !bContainsChannel)
			{
				CurrentBit |= InBit;
				SetResponseChannels(Tag, Str, CurrentBit);
			}
			else if (!bEnableChannel && bContainsChannel)
			{
				CurrentBit &= ~InBit;
				SetResponseChannels(Tag, Str, CurrentBit);
			}
		}
		else if (bEnableChannel)
		{
			FStimulusTagResponse NewStr = FStimulusTagResponse();
			NewStr.BitChannels.Value = 0;
			FStimulusTagResponse& Ref = TagResponse.Add(Tag, NewStr);
			SetResponseChannels(Tag, Ref, 1llu << (Channel - 1));
		}
	}
}
void USenseStimulusBase::SetResponseChannelsBit(const FName Tag, const FBitFlag64_SenseSys NewChannels)
{
	if (Tag != NAME_None && GetSenseManager() && NewChannels.Value != 0)
	{
		FStimulusTagResponse* StrPtr = GetStimulusTagResponse(Tag);
		if (StrPtr == nullptr)
		{
			FStimulusTagResponse NewStr = FStimulusTagResponse();
			NewStr.BitChannels.Value = 0;
			StrPtr = &TagResponse.Add(Tag, NewStr);
		}
		SetResponseChannels(Tag, *StrPtr, NewChannels.Value);
	}
}


void USenseStimulusBase::SetResponseChannels(const FName& Tag, FStimulusTagResponse& Str, const uint64 NewChannelsBit)
{
	if (Str.BitChannels.Value == 0 && NewChannelsBit != 0) //new
	{
		Str.SetBitChannels(NewChannelsBit);
		if (IsRegisteredForSense())
		{
			GetSenseManager()->Add_SenseStimulus(this, Tag, Str);
		}
		else
		{
			RegisterSelfSense();
		}
	}
	else if (Str.BitChannels.Value != 0 && NewChannelsBit == 0) // remove
	{
		if (IsRegisteredForSense())
		{
			if (TagResponse.Num() == 1)
			{
				UnRegisterSelfSense();
			}
			else
			{
				GetSenseManager()->Remove_SenseStimulus(this, Tag, Str);
			}
		}
		Str.SetBitChannels(NewChannelsBit);
	}
	else if (Str.BitChannels.Value != 0 && NewChannelsBit != 0) // update
	{
		Str.SetBitChannels(NewChannelsBit);
	}
}


bool USenseStimulusBase::IsResponseChannel(const FName& Tag, const uint8 Channel) const
{
	if (Tag != NAME_None && Channel > 0 && Channel <= 64)
	{
		if (const FStimulusTagResponse* StrPtr = GetStimulusTagResponse(Tag))
		{
			return StrPtr->IsResponseChannel(Channel);
		}
	}
	return false;
}

bool USenseStimulusBase::IsResponseTag(const FName& Tag) const
{
	if (Tag != NAME_None)
	{
		if (const FStimulusTagResponse* StrPtr = GetStimulusTagResponse(Tag))
		{
			return StrPtr->BitChannels.Value != 0;
		}
	}
	return false;
}


bool USenseStimulusBase::RegisterSelfSense()
{
	bRegisteredForSense = false;
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld()) // && World->HasBegunPlay()
	{
		SenseManager = USenseManager::GetSenseManager(this);
		if (GetSenseManager() != nullptr && bEnable)
		{
			AddToRoot();
			uint64 AllChan = 0;
			for (const auto& It : TagResponse)
			{
				AllChan |= It.Value.BitChannels.Value;
			}

			bRegisteredForSense = AllChan == 0 ? false : GetSenseManager()->RegisterSenseStimulus(this);

			if (IsRegisteredForSense())
			{
				if (const auto OwnerActor = GetOwner())
				{
					switch (Mobility)
					{
						case EStimulusMobility::Static: // nothing
						{
#if WITH_EDITOR
							if (OwnerActor->IsRootComponentMovable())
							{
								UE_LOG(
									LogSenseSys,
									Warning,
									TEXT("SenseStimulusComponent %s Mobility set to Static, but OwnerActor %s RootComponentMovable"),
									*GetNameSafe(this),
									*GetNameSafe(OwnerActor));
							}
#endif
							break;
						}
						case EStimulusMobility::MovableOwner: // no break, bind RootComponentTransformUpdated
						{
							BindTransformUpdated();
						}
						case EStimulusMobility::MovableTick: // enable tick
						{
							GetWorld()->GetTimerManager().SetTimer(
								TickStartHandle,
								FTimerDelegate::CreateUObject(this, &USenseStimulusBase::SetComponentTickEnabled, true),
								FMath::RandRange(0.01f, 0.2f),
								false);
							break;
						}
						default:
						{
							checkNoEntry();
							break;
						}
					}
				}
				else
				{
					UE_LOG(LogSenseSys, Error, TEXT("SenseStimulusComponent %s, dont have a valid OwnerActor"), *GetNameSafe(this));
				}
			}
			else
			{
				UE_LOG(LogSenseSys, Error, TEXT("SenseStimulusComponent %s, dont have a valid OwnerActor"), *GetNameSafe(this));
			}
		}
		else if (bEnable)
		{
			UE_LOG(LogSenseSys, Error, TEXT("%s, USenseStimulusBase::RegisterSelfSense: registration in SenseManager FAILED!"),*GetNameSafe(this));
		}
	}
	else
	{
		UE_LOG(LogSenseSys, Error, TEXT("SenseStimulusComponent: GetOuter %s, World->IsGameWorld() FAILED!"), *GetNameSafe(GetOuter()));
	}
	return IsRegisteredForSense();
}

void USenseStimulusBase::ResetDelayTickStart()
{
	if (TickStartHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickStartHandle);
		TickStartHandle.Invalidate();
	}
}

bool USenseStimulusBase::UnRegisterSelfSense()
{
	if (IsRegisteredForSense())
	{
		bRegisteredForSense = false;
		SetComponentTickEnabled(false);

		if (const auto OwnerActor = GetOwner())
		{
			if (const auto Root = OwnerActor->GetRootComponent())
			{
				Root->TransformUpdated.RemoveAll(this);
			}
		}

		if (GetSenseManager() != nullptr && GetSenseManager()->HaveSenseStimulus())
		{
			if (const UWorld* World = GetWorld())
			{
				if (World->IsGameWorld() && World->HasBegunPlay() && !World->bIsTearingDown)
				{
					GetSenseManager()->UnRegisterSenseStimulus(this);
				}
			}
			else
			{
				GetSenseManager()->UnRegisterSenseStimulus(this);
			}
		}

		RemoveFromRoot();
	}

	return true;
}

void USenseStimulusBase::OnRegister()
{
	Super::OnRegister();
}

void USenseStimulusBase::BeginPlay()
{
	Super::BeginPlay();

	RegisterSelfSense();
}

void USenseStimulusBase::OnUnregister()
{
	Super::OnUnregister();
}


void USenseStimulusBase::ReportSenseEvent(const FName Tag)
{
	if (GetSenseManager())
	{
		if (const FStimulusTagResponse* StrPtr = GetStimulusTagResponse(Tag))
		{
			const ElementIndexType StimulusID = StrPtr->GetObjID();
			if (StimulusID != TNumericLimits<ElementIndexType>::Max())
			{
				const auto& Delegate = GetSenseManager()->ReportStimulus_Event;
				if (Delegate.IsBound())
				{
					Delegate.Broadcast(StimulusID, Tag);
				}
			}
		}
	}
}

void USenseStimulusBase::SetEnableSenseStimulus(const bool bNewEnable)
{
	if (bEnable && !bNewEnable)
	{
		CriticalSection.Lock();
		bEnable = bNewEnable;
		CriticalSection.Unlock();
		UnRegisterSelfSense();
	}
	else if (!bEnable && bNewEnable)
	{
		bEnable = bNewEnable;
		RegisterSelfSense();
	}
}

void USenseStimulusBase::SetStimulusMobility(const EStimulusMobility InMobility)
{
	if (bEnable)
	{
		switch (InMobility)
		{
			case EStimulusMobility::Static:
			{
				if (Mobility == EStimulusMobility::MovableOwner)
				{
					UnBindTransformUpdated();
				}
				SetComponentTickEnabled(false);
				break;
			}
			case EStimulusMobility::MovableOwner:
			{
				if (Mobility != EStimulusMobility::MovableOwner)
				{
					BindTransformUpdated();
				}
				SetComponentTickEnabled(true);
				break;
			}
			case EStimulusMobility::MovableTick:
			{
				if (Mobility == EStimulusMobility::MovableOwner)
				{
					UnBindTransformUpdated();
				}
				SetComponentTickEnabled(true);
				break;
			}
			default:
			{
				checkNoEntry();
				break;
			}
		}
	}
	Mobility = InMobility;
}

void USenseStimulusBase::RootComponentTransformUpdated(
	class USceneComponent* UpdatedComponent,
	EUpdateTransformFlags UpdateTransformFlags,
	ETeleportType Teleport)
{
#if WITH_EDITOR
	check(Mobility == EStimulusMobility::MovableOwner);
#endif
	bDirtyTransform = true;
}

void USenseStimulusBase::BindTransformUpdated()
{
	if (const auto OwnerActor = GetOwner())
	{
		if (const auto Root = OwnerActor->GetRootComponent())
		{
			Root->TransformUpdated.AddUObject(this, &USenseStimulusBase::RootComponentTransformUpdated);
		}
		else
		{
			UE_LOG(
				LogSenseSys,
				Error,
				TEXT(" %s, SenseStimulusBase::BindTransformUpdated: OwnerActor %s dont have a valid RootComponent"),
				*GetNameSafe(this),
				*GetNameSafe(OwnerActor))
		}
	}
	else
	{
		UE_LOG(LogSenseSys, Error, TEXT("%s SenseStimulusBase::BindTransformUpdated: ,  dont have a valid OwnerActor"), *GetNameSafe(this))
	}
}

void USenseStimulusBase::UnBindTransformUpdated() const
{
	if (const auto OwnerActor = GetOwner())
	{
		if (const auto Root = OwnerActor->GetRootComponent())
		{
			Root->TransformUpdated.RemoveAll(this);
		}
	}
}


void USenseStimulusBase::SetReceiveStimulusFlag(const FName SensorTag, EReceiveStimulusFlag Flag, const bool bValue)
{
	if (const auto Str = GetStimulusTagResponse(SensorTag))
	{
		CriticalSection.Lock();
		if (bValue)
		{
			Str->ReceiveStimulusFlag |= static_cast<uint8>(Flag);
		}
		else
		{
			Str->ReceiveStimulusFlag &= ~static_cast<uint8>(Flag);
		}
		CriticalSection.Unlock();
	}
}


void USenseStimulusBase::OnUnregisterReceiver(UObject* Obj)
{
	if (!IsValid(this)) return;
	if (const auto Comp = Cast<USenseReceiverComponent>(Obj))
	{
		if (AActor* const CompOwner = Comp->GetOwner())
		{
			//const uint32 CompOwnerHash = GetTypeHash(CompOwner); //todo remove all actor current present by hash

			if (SpecifyActorDelegateMap.Num())
			{
				const auto DelegatePtr = SpecifyActorDelegateMap.Find(CompOwner);
				if (DelegatePtr)
				{
					DelegatePtr->ExecuteIfBound(CompOwner, nullptr, 0, EOnSenseEvent::SenseLost);
					DelegatePtr->ExecuteIfBound(CompOwner, nullptr, 0, EOnSenseEvent::SenseForget);
				}
				SpecifyActorDelegateMap.Remove(CompOwner);
			}

			for (auto& It : TagResponse)
			{
				auto& Str = It.Value;
				if (Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::DetectGlobalSenseState))
				{
					if (Str.TmpSensed.Remove(CompOwner) && Str.TmpSensed.Num() == 0 && OnSenseStateChanged.IsBound())
					{
						OnSenseStateChanged.Broadcast(It.Key, EOnSenseEvent::SenseLost);
					}
					if (Str.TmpLost.Remove(CompOwner) && Str.TmpLost.Num() == 0 && OnSenseStateChanged.IsBound())
					{
						OnSenseStateChanged.Broadcast(It.Key, EOnSenseEvent::SenseForget);
					}
				}
			}
			return; // return if success
		}
	}

	{
		while (SpecifyActorDelegateMap.Remove(nullptr))
		{}

		for (auto& It : TagResponse)
		{
			auto& Str = It.Value;
			if (Str.TmpSensed.Num())
			{
				bool bRemSensed = false;
				while (Str.TmpSensed.Remove(nullptr))
				{
					bRemSensed = true;
				}
				if (bRemSensed && Str.TmpSensed.Num() && Str.IsReceiveOnSense(EOnSenseEvent::SenseLost) && OnSenseStateChanged.IsBound())
				{
					OnSenseStateChanged.Broadcast(It.Key, EOnSenseEvent::SenseLost);
					if (Str.TmpLost.Num() && Str.IsReceiveOnSense(EOnSenseEvent::SenseForget) && OnSenseStateChanged.IsBound())
					{
						OnSenseStateChanged.Broadcast(It.Key, EOnSenseEvent::SenseForget);
					}
				}
			}
			if (Str.TmpLost.Num())
			{
				bool bRemLost = false;
				while (Str.TmpLost.Remove(nullptr))
				{
					bRemLost = true;
				}
				if (bRemLost && Str.TmpLost.Num() && Str.IsReceiveOnSense(EOnSenseEvent::SenseForget) && OnSenseStateChanged.IsBound())
				{
					OnSenseStateChanged.Broadcast(It.Key, EOnSenseEvent::SenseForget);
				}
			}
		}
		//UE_LOG(LogSenseSys, Error, TEXT("USenseStimulusBase::OnUnregisterReceiver Param : Invalid Object"));
	}
}

bool USenseStimulusBase::IsSensedFrom(const AActor* Actor, const FName SensorTag) const
{
	if (SensorTag == NAME_None) return true;
	if (const auto Str = GetStimulusTagResponse(SensorTag))
	{
		return Str->TmpSensed.Contains(Actor);
	}
	return false;
}

bool USenseStimulusBase::IsRememberFrom(const AActor* Actor, const FName SensorTag) const
{
	if (SensorTag == NAME_None) return true;
	if (const auto Str = GetStimulusTagResponse(SensorTag))
	{
		return Str->TmpLost.Contains(Actor);
	}
	return false;
}


void USenseStimulusBase::SensedFromSensorUpdate(const USensorBase* Sensor, const int32 Channel, const EOnSenseEvent OnSenseEvent)
{
	if (bEnable && IsRegisteredForSense() && Sensor && Sensor->SensorTag != NAME_None && IsValid(this))
	{
		const FName SensorTag = Sensor->SensorTag;
		if (const auto Actor = Sensor->GetSensorOwner())
		{
			if (const auto StrPtr = GetStimulusTagResponse(SensorTag))
			{
				auto& Str = *StrPtr;
				if (Str.IsReceiveOnSense(OnSenseEvent) && OnSensedFromSensor.IsBound())
				{
					OnSensedFromSensor.Broadcast(Sensor, Channel, OnSenseEvent);
				}

				if (SpecifyActorDelegateMap.Num())
				{
					const auto DelegatePtr = SpecifyActorDelegateMap.Find(Actor);
					if (DelegatePtr)
					{
						DelegatePtr->ExecuteIfBound(Actor, Sensor, Channel, OnSenseEvent);
					}
					else
					{
						SpecifyActorDelegateMap.Remove(Actor);
					}
				}

				if (Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::DetectGlobalSenseState))
				{
					const bool bCall = Str.IsOnSenseStateChanged(Actor, OnSenseEvent, Channel);
					if (bCall)
					{
						if (OnSenseStateChanged.IsBound())
						{
							OnSenseStateChanged.Broadcast(SensorTag, OnSenseEvent);
						}
					}
				}
			}
		}
	}
}


void USenseStimulusBase::Serialize(FArchive& Ar)
{

#if WITH_EDITORONLY_DATA

	if (Ar.IsSaving())
	{
		for (auto& It : TagResponse)
		{
			auto& Str = It.Value;
			if (Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::DetectGlobalSenseState))
			{
				if (!(Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnCurrentSensed) && //
					  Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnNewSensed)))		//
				{
					Str.ReceiveStimulusFlag |= static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnNewSensed);
				}

				if (!(Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnLost) &&	//
					  Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnForget))) //
				{
					Str.ReceiveStimulusFlag |= static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnLost);
				}
			}
			if (Str.IsReceiveOnSense(EOnSenseEvent::SenseForget))
			{
				Str.ReceiveStimulusFlag |= static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnLost);
			}
			if (Str.IsReceiveOnSense(EOnSenseEvent::SenseLost))
			{
				Str.ReceiveStimulusFlag |= static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnNewSensed);
			}
		}
	}
#endif //WITH_EDITORONLY_DATA

	Super::Serialize(Ar);

#if WITH_EDITORONLY_DATA
	if (Ar.IsLoading())
	{
		//for (FStimulusTagResponse& Str : TagResponse)
		//{
		//	UE_LOG(LogTemp, Warning, TEXT("Ar.IsLoading -- SensorTag: %s, BitChannels: %llu"), *Str.SensorTag.ToString(), Str.BitChannels.Value);
		//}
		bool Mod = false;

		for (auto& It : TagResponse)
		{
			FStimulusTagResponse& Str = It.Value;

			const auto Tmp = Str.ReceiveStimulusFlag;
			if (Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::DetectGlobalSenseState))
			{
				if (!(Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnCurrentSensed) &&
					  Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnNewSensed)))
				{
					Str.ReceiveStimulusFlag |= static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnNewSensed);
				}

				if (!(Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnLost) &&
					  Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnForget)))
				{
					Str.ReceiveStimulusFlag |= static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnLost);
				}
			}
			if (Str.IsReceiveOnSense(EOnSenseEvent::SenseForget))
			{
				Str.ReceiveStimulusFlag |= static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnLost);
			}
			if (Str.IsReceiveOnSense(EOnSenseEvent::SenseLost))
			{
				Str.ReceiveStimulusFlag |= static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnNewSensed);
			}
			Mod |= Tmp != Str.ReceiveStimulusFlag;
		}
		if (Mod)
		{
			MarkPackageDirty_Internal();
		}
	}
#endif //WITH_EDITORONLY_DATA
}


#if WITH_EDITORONLY_DATA
void USenseStimulusBase::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	TagResponse.Compact();
	for (auto& It : TagResponse)
	{
		auto& Str = It.Value;
		if (Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::DetectGlobalSenseState))
		{
			if (!(Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnCurrentSensed) &&
				  Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnNewSensed)))
			{
				Str.ReceiveStimulusFlag |= static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnNewSensed);
			}

			if (!(Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnLost) &&
				  Str.ReceiveStimulusFlag & static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnForget)))
			{
				Str.ReceiveStimulusFlag |= static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnLost);
			}
		}
		if (Str.IsReceiveOnSense(EOnSenseEvent::SenseForget))
		{
			Str.ReceiveStimulusFlag |= static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnLost);
		}
		if (Str.IsReceiveOnSense(EOnSenseEvent::SenseLost))
		{
			Str.ReceiveStimulusFlag |= static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnNewSensed);
		}
	}

	Super::PostEditChangeProperty(e);
}

EDataValidationResult USenseStimulusBase::IsDataValid(FDataValidationContext& ValidationErrors) const
{
	const EDataValidationResult IsValid = Super::IsDataValid(ValidationErrors);
	return IsValid;
}

#endif //WITH_EDITORONLY_DATA

#if WITH_EDITORONLY_DATA

void USenseStimulusBase::DrawComponent(const FSceneView* View, FPrimitiveDrawInterface* PDI) const
{
	const UWorld* World = GetWorld();
	if (!bEnable || !IsRegisteredForSense() || !IsRegistered() || !World) return;

	constexpr float Thickness = 0.75f;
	constexpr float DepthBias = 1.f;
	constexpr bool bScreenSpace = false;
	constexpr int32 CircleSide = 4;
	constexpr uint8 DepthPriority = SDPG_Foreground;

	for (const auto& It : GetTagResponse())
	{
		const FStimulusTagResponse& Str = It.Value;
		FSenseSysDebugDraw _DebugDraw;
		if (GetSenseManager() && World->IsGameWorld())
		{
			_DebugDraw = GetSenseManager()->GetSenseSysDebugDraw(It.Key);
		}
		else
		{
			_DebugDraw = FSenseSysDebugDraw(false);
			_DebugDraw.Stimulus_DebugSensedPoints = true;
		}

		if (_DebugDraw.Stimulus_DebugSensedPoints)
		{
			/*
			FSensedStimulus SS = TagResponse_It.ContainerTree->GetSensedStimulusCopy_Simple_TS(TagResponse_It.GetObjID()); //bug crash?
			if (SS.SensedPoints.Num())
			*/

			const FQuat ViewRot = FQuat(View->ViewRotation);
			const FVector Dir = ViewRot.GetAxisX();
			const FVector Yax = ViewRot.GetAxisY();
			const FVector Zax = ViewRot.GetAxisZ();

			const FVector Location = GetSingleSensePoint(It.Key);

			// clang-format off
			
			{
				const FVector YaxMain = Yax.RotateAngleAxis(45.f, Dir);
				const FVector ZaxMain = Zax.RotateAngleAxis(45.f, Dir);

				DrawCircle(PDI, Location, YaxMain, ZaxMain,
					FColor::Blue, EDebugSenseSysHelpers::DebugNormalSize, CircleSide,
					DepthPriority, Thickness, DepthBias, bScreenSpace);
			}
			const TArray<FVector> Points = GetSensePoints(It.Key);
			if (Points.Num())
			{
				FBox Box(Location, Location);
				for (int32 i = 1; i < Points.Num(); i++)
				{
					Box += Points[i];
					DrawCircle(PDI, Points[i], Yax, Zax,
						FColor::Blue, EDebugSenseSysHelpers::DebugSmallSize, CircleSide,
						DepthPriority, Thickness, DepthBias, bScreenSpace);
				}
				if (!Box.Max.Equals(Box.Min))
				{
					DrawWireBox(PDI, Box, FColor::Blue, DepthPriority, 0.5f);
				}
			}

			// clang-format on
		}
		if (_DebugDraw.Stimulus_DebugCurrentSensed || _DebugDraw.Stimulus_DebugLostSensed)
		{
			if (World->IsGameWorld() && World->HasBegunPlay() && GetOwner())
			{
				const auto Start = GetOwner()->GetActorLocation();
				if (_DebugDraw.Stimulus_DebugCurrentSensed)
				{
					TArray<AActor*> TmpSensed;
					Str.TmpSensed.GenerateKeyArray(TmpSensed);
					for (const auto& ItSensed : TmpSensed)
					{
						if (ItSensed)
						{
							const auto Color = FLinearColor::Red;
							auto End = ItSensed->GetActorLocation();
							PDI->DrawLine(Start, End, Color, DepthPriority);
						}
					}
				}

				if (_DebugDraw.Stimulus_DebugLostSensed)
				{
					constexpr auto Color = FLinearColor(1, 0, 1);

					const FQuat ViewRot = FQuat(View->ViewRotation);
					const FVector Dir = ViewRot.GetAxisX();
					const FVector Yax = ViewRot.GetAxisY();
					const FVector Zax = ViewRot.GetAxisZ();

					struct FDrawDT
					{
						FDrawDT(const float InAge, const FVector& InLoc, const FVector& InALoc) : Age(InAge), Loc(InLoc), ALoc(InALoc) {}
						float Age;
						FVector Loc;
						FVector ALoc;
					};

					{
						TArray<AActor*> TmpLost;
						Str.TmpLost.GenerateKeyArray(TmpLost);
						if (TmpLost.Num())
						{
							TArray<FDrawDT> DrawArr;
							DrawArr.Reserve(TmpLost.Num());

							for (const auto& ItLost : TmpLost)
							{
								if (ItLost)
								{
									if (const USenseReceiverComponent* Receiver = USenseSystemBPLibrary::GetReceiverFromActor(ItLost))
									{
										FSensedStimulus SS;
										ESuccessState Success;
										Receiver->FindSensedActor(GetOwner(), ESensorType::Active, It.Key, ESensorArrayByType::SenseLost, SS, Success);
										if (Success == ESuccessState::Success && ItLost)
										{
											DrawArr.Add(FDrawDT(SS.SensedTime, SS.SensedPoints[0].SensedPoint, ItLost->GetActorLocation()));
										}
									}
								}
							}

							if (DrawArr.Num())
							{
								// clang-format off
								
								DrawArr.Sort([](const FDrawDT& A, const FDrawDT& B) { return A.Age < B.Age; });

								const FVector LocYax = Yax.RotateAngleAxis(45.f, Dir);
								const FVector LocZax = Zax.RotateAngleAxis(45.f, Dir);

								FVector Previous = DrawArr[0].Loc;
								for (int32 i = 0; i < DrawArr.Num(); i++)
								{
									PDI->DrawLine(DrawArr[i].Loc, DrawArr[i].ALoc, FLinearColor(0.1f, 0.f, 0.1f), DepthPriority);									
									DrawCircle(PDI, DrawArr[i].Loc, Yax, Zax,
										Color, 1.f, 3, 
										DepthPriority, 0.0f, DepthBias, bScreenSpace);
																		
									if (i > 1)
									{
										PDI->DrawLine(Previous, DrawArr[i].Loc, Color, DepthPriority);

										if (!Previous.Equals(DrawArr[i].Loc, 15.f))
										{
											DrawCircle(PDI, Previous, LocYax, LocZax,
												Color, EDebugSenseSysHelpers::DebugLargeSize, CircleSide,
												DepthPriority, Thickness, DepthBias, bScreenSpace);
											
											Previous = DrawArr[i].Loc;
										}											
									}
								}

								const FVector V = GetOwner()->GetActorLocation();
								if (!DrawArr.Last().Loc.Equals(V))
								{
									DrawCircle(PDI, DrawArr.Last().Loc, LocYax, LocZax,
										Color, EDebugSenseSysHelpers::DebugLargeSize, CircleSide,
										DepthPriority, Thickness, DepthBias, bScreenSpace);
								}
								
								DrawCircle(PDI, V, LocYax, LocZax,
									Color, EDebugSenseSysHelpers::DebugLargeSize, CircleSide,
									DepthPriority, Thickness, DepthBias, bScreenSpace);
								PDI->DrawLine(DrawArr.Last().Loc, V, Color, DepthPriority);

								// clang-format on
							}
						}
					}
				}
			}
		}
	}
}

void USenseStimulusBase::DrawComponentHUD(const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) const
{
	const UWorld* World = GetWorld();
	if (!bEnable || !IsRegisteredForSense() || !IsRegistered() || !World) return;
	if (!(World->IsGameWorld() && World->HasBegunPlay() && GetOwner())) return;

	for (const auto& It : GetTagResponse())
	{
		const FStimulusTagResponse& Str = It.Value;
		const FSenseSysDebugDraw DebugDraw =
			(GetSenseManager() && World->IsGameWorld()) ? GetSenseManager()->GetSenseSysDebugDraw(It.Key) : FSenseSysDebugDraw(false);

		if (DebugDraw.SenseSys_DebugAge && DebugDraw.Stimulus_DebugLostSensed && Str.TmpLost.Num())
		{
			struct FDrawDT
			{
				FDrawDT(const float InAgeValue, const FVector& InLoc) : AgeValue(InAgeValue), Loc(InLoc) {}
				float AgeValue;
				FVector Loc;
			};

			TArray<FDrawDT> DrawArr;
			TArray<AActor*> TmpLost;
			Str.TmpLost.GenerateKeyArray(TmpLost);
			DrawArr.Reserve(TmpLost.Num());

			constexpr auto Color = FLinearColor(1, 0, 1);
			const float Sec = World->GetTimeSeconds();

			for (const auto& ItLost : TmpLost)
			{
				if (ItLost)
				{
					if (const USenseReceiverComponent* Receiver = USenseSystemBPLibrary::GetReceiverFromActor(ItLost))
					{
						FSensedStimulus SS;
						ESuccessState Success;
						Receiver->FindSensedActor(GetOwner(), ESensorType::Active, It.Key, ESensorArrayByType::SenseLost, SS, Success);
						if (Success == ESuccessState::Success)
						{
							DrawArr.Add(FDrawDT(Sec - SS.SensedTime, SS.SensedPoints[0].SensedPoint));
						}
					}
				}
			}

			if (DrawArr.Num())
			{
				FString OutStr;
				DrawArr.Sort([](const FDrawDT& A, const FDrawDT& B) { return A.AgeValue > B.AgeValue; });
				constexpr FLinearColor ZeroColor(0, 0, 0, 0);
				const FVector V = GetOwner()->GetActorLocation();
				FVector Previous = V;
				for (int32 i = 0; i < DrawArr.Num(); i++)
				{
					FString SText = FString::Printf(TEXT("Age: %3.3f \n"), DrawArr[i].AgeValue);
					if (Previous.Equals(DrawArr[i].Loc, 15.f))
					{
						OutStr.Append(MoveTemp(SText));
					}
					else
					{
						FVector2D PixLoc;
						{
							View->WorldToPixel(Previous, PixLoc);
						}
						Canvas->DrawShadowedString(PixLoc.X, PixLoc.Y, *OutStr, GEngine->GetLargeFont(), Color, ZeroColor);
						OutStr = MoveTemp(SText);
						Previous = DrawArr[i].Loc;
					}
				}

				FVector2D PixLoc;
				{
					const FVector DrawLast = DrawArr.Last().Loc.Equals(V, 15.f) ? V : DrawArr.Last().Loc;
					View->WorldToPixel(DrawLast, PixLoc);
				}
				Canvas->DrawShadowedString(PixLoc.X, PixLoc.Y, *OutStr, GEngine->GetLargeFont(), Color, ZeroColor);
			}
		}
	}
}

void USenseStimulusBase::MarkPackageDirty_Internal() const
{
	if (!this->HasAnyFlags(RF_Transient))
	{
		if (UPackage* Package = this->GetOutermost())
		{
			const bool bIsDirty = Package->IsDirty();
			if (!bIsDirty)
			{
				Package->SetDirtyFlag(true);
			}
			Package->PackageMarkedDirtyEvent.Broadcast(Package, bIsDirty);
		}
	}
}

#endif //WITH_EDITORONLY_DATA

void USenseStimulusBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	//tick wrap positions
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_SenseStimulusTick);

	if (bEnable && IsRegisteredForSense())
	{
		const bool bNeedTick = (Mobility == EStimulusMobility::MovableTick) || (bDirtyTransform && Mobility == EStimulusMobility::MovableOwner);
		if (bNeedTick)
		{
			if (const UWorld* World = GetWorld())
			{
				if (const auto SenseManagerPtr = GetSenseManager())
				{
					const float PositionUpdateTime = World->GetTimeSeconds();
					for (auto& It : TagResponse)
					{
						if (SenseManagerPtr->IsHaveReceiverTag(It.Key)) //todo Event driven bool
						{
							const FVector NewLocation = GetSingleSensePoint(It.Key);
							const TArray<FVector> Points = GetSensePoints(It.Key);
							It.Value.UpdatePosition(NewLocation, Points, PositionUpdateTime, true);
						}
					}
					if (Mobility == EStimulusMobility::MovableOwner)
					{
						bDirtyTransform = false;
					}
				}
			}
		}
	}
	//Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USenseStimulusBase::AddIgnoreTraceActor(AActor* Actor)
{
	CriticalSection.Lock();
	IgnoreTraceActors.AddUnique(Actor);
	CriticalSection.Unlock();
}

void USenseStimulusBase::AddIgnoreTraceActors(const TArray<AActor*>& Actors)
{
	CriticalSection.Lock();
	for (const auto It : Actors)
	{
		IgnoreTraceActors.AddUnique(It);
	}
	CriticalSection.Unlock();
}

void USenseStimulusBase::RemoveIgnoreTraceActor(AActor* Actor)
{
	CriticalSection.Lock();
	IgnoreTraceActors.Remove(Actor);
	CriticalSection.Unlock();
}

void USenseStimulusBase::RemoveIgnoreTraceActors(const TArray<AActor*>& Actors)
{
	CriticalSection.Lock();
	for (const auto It : Actors)
	{
		IgnoreTraceActors.Remove(It);
	}
	CriticalSection.Unlock();
}

void USenseStimulusBase::ClearIgnoreTraceActors()
{
	CriticalSection.Lock();
	IgnoreTraceActors.Empty();
	CriticalSection.Unlock();
}

uint64 USenseStimulusBase::ChannelToBit(TArray<uint8>& InChannels)
{
	if (InChannels.Num())
	{
		if (InChannels.Num() > 1)
		{
			// sort and remove duplicates
			Algo::Sort(InChannels, TLess<>());
			const auto Predicate = [](const TArray<uint8>& A, const int32 ID) { return (ID > 0 && A[ID] == A[ID - 1]) || A[ID] > 63; };
			ArrayHelpers::Filter_Sorted(InChannels, Predicate);
		}
		uint64 Out = 0;
		for (const uint8 Channel : InChannels)
		{
			if (Channel > 0 && Channel <= 64)
			{
				Out |= 1llu << (Channel - 1);
			}
		}
		return Out;
	}
	return 0;
}