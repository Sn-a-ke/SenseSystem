//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/SensorBase.h"
#include "SenseSystem.h"
#include "SensedStimulStruct.h"
#include "HashSorted.h"
#include "QtOtContainer.h"
#include "BaseSensorTask.h"
#include "Sensors/Tests/SensorTestBase.h"
#include "Sensors/Tests/SensorLocationTestBase.h"
#include "SenseStimulusBase.h"
#include "SenseReceiverComponent.h"
#include "SenseManager.h"
#include "SenseSystemBPLibrary.h"
#include "SenseDetectPool.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Async/ParallelFor.h"
#include "Stats/Stats2.h"
#include "Async/TaskGraphInterfaces.h"


#if WITH_EDITORONLY_DATA
	#include "Engine/Engine.h"
	#include "Engine/Font.h"
	#include "CanvasTypes.h"
	#include "SceneManagement.h"
	#include "SceneView.h"
	#include "UnrealClient.h"
	#include "UObject/Package.h"

	#include "DrawDebugHelpers.h"
#endif


struct FNotValidObjPredicate
{
	FORCEINLINE bool operator()(const UObject* Obj) const { return !IsValid(Obj); }
};
struct FValidObjPredicate
{
	FORCEINLINE bool operator()(const UObject* Obj) const { return IsValid(Obj); }
};


//struct FFindScoreIDByScorePredicate
//{
//	explicit FFindScoreIDByScorePredicate(const TArray<FSensedStimulus>& InPoolRef) : PoolRef(InPoolRef) {}
//	FORCEINLINE bool operator()(const int32 A, const float B) const { return PoolRef[A].Score > B; };
//	FORCEINLINE bool operator()(const float A, const int32 B) const { return A > PoolRef[B].Score; };
//
//private:
//	const TArray<FSensedStimulus>& PoolRef;
//};

struct FSortScorePredicate2
{
	explicit FSortScorePredicate2(const TArray<FSensedStimulus>& InPoolRef) : PoolRef(InPoolRef) {}
	FORCEINLINE bool operator()(const int32 A, const int32 B) const { return PoolRef[A].Score > PoolRef[B].Score; };

private:
	const TArray<FSensedStimulus>& PoolRef;
};


void FUpdateSensorTask::DoWork() const
{
	if (Sensor && Sensor->IsValidForTest())
	{
		if (Sensor->UpdateState == ESensorState::ReadyToUpdate)
		{
			Sensor->UpdateSensor();
		}
	}
}


FChannelSetup::FChannelSetup(const uint8 InSenseChannel) : Channel(InSenseChannel)
{}

FChannelSetup::FChannelSetup(const FChannelSetup& In)
{
	*this = In;
}
void FChannelSetup::FDeleterSdp::operator()(FSenseDetectPool* Ptr) const
{
	delete Ptr;
}
FChannelSetup::~FChannelSetup()
{}
void FChannelSetup::Init()
{
	_SenseDetect = MakeUnique<FSenseDetectPool>();
	_SenseDetect->Best_Sense.TrackBestScoreCount = FMath::Max(0, TrackBestScoreCount);
	_SenseDetect->Best_Sense.MinBestScore = MinBestScore;
}


void FChannelSetup::SetTrackBestScoreCount(const int32 Count)
{
	TrackBestScoreCount = FMath::Max(0, Count);
	_SenseDetect->Best_Sense.TrackBestScoreCount = TrackBestScoreCount;
}

void FChannelSetup::SetMinBestScore(const float Score)
{
	MinBestScore = Score;
	_SenseDetect->Best_Sense.MinBestScore = MinBestScore;
}

void FChannelSetup::OnSensorChannelUpdated(const ESensorType InSensorType)
{
	check(_SenseDetect);
	const auto& SenseDetectRef = *_SenseDetect;
	NewSensed = SenseDetectRef.GetArrayCopy_SenseEvent(ESensorArrayByType::SensedNew);
	CurrentSensed = SenseDetectRef.GetArrayCopy_SenseEvent(ESensorArrayByType::SenseCurrent);

	if (InSensorType != ESensorType::Passive)
	{
		LostCurrentSensed = SenseDetectRef.GetArrayCopy_SenseEvent(ESensorArrayByType::SenseCurrentLost);
	}
	BestSensedID_ByScore = SenseDetectRef.Best_Sense.ScoreIDs;

	checkSlow(Algo::IsSorted(BestSensedID_ByScore, FSortScorePredicate2(CurrentSensed)));
}

void FChannelSetup::OnSensorAgeUpdated(const ESensorType InSensorType, const EUpdateReady UpdateReady)
{
	check(_SenseDetect);
	const auto& SenseDetectRef = *_SenseDetect;
	LostAllSensed = SenseDetectRef.GetArrayCopy_SenseEvent(ESensorArrayByType::SenseLost);
	ForgetSensed = SenseDetectRef.GetArrayCopy_SenseEvent(ESensorArrayByType::SenseForget);

	if (InSensorType != ESensorType::Active || UpdateReady == EUpdateReady::Skip)
	{
		LostCurrentSensed = SenseDetectRef.GetArrayCopy_SenseEvent(ESensorArrayByType::SenseCurrentLost);
	}
}


USensorBase::USensorBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	if (SensorTag == NAME_None) SensorTag = *(this->GetClass()->GetName());
	SensorThreadType = ESensorThreadType::Sense_Thread;
	PrivateSenseReceiver = GetTypedOuter<USenseReceiverComponent>();

#if WITH_EDITORONLY_DATA
	if (PrivateSenseReceiver)
	{
		const UClass* ObjClass = this->GetClass();
		if (ObjClass == UActiveSensor::StaticClass() || ObjClass == UPassiveSensor::StaticClass())
		{
			bOuter = true;
		}
		else
		{
			bOuter = false;
		}
	}
#endif

	SensorTimer.UpdateTimeRate = UpdateTimeRate;

	UpdateState = ESensorState::Uninitialized;

	bClearCurrentSense = 0;
	bClearCurrentMemory = 0;

	//ChannelSetup
	//SensorTests

	//#if WITH_EDITOR
	//	if(GEditor) GEditor->OnBlueprintPreCompile().AddUObject(this, &USensorBase::OnPreCompile);
	//#endif
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS;
USensorBase::~USensorBase()
{}
PRAGMA_ENABLE_DEPRECATION_WARNINGS;

void USensorBase::Serialize(FArchive& Ar)
{
#if WITH_EDITORONLY_DATA
	if (Ar.IsSaving())
	{
		{
			Algo::Sort(ChannelSetup, TLess<uint8>());
			const auto Predicate = [](const TArray<FChannelSetup>& A, const int32 ID)
			{ return (ID > 0 && A[ID] == A[ID - 1]) || A[ID].Channel > 64 || A[ID].Channel <= 0; };
			ArrayHelpers::Filter_Sorted(ChannelSetup, Predicate);
		}
		BitChannels.Value = GetBitChannelChannelSetup();
	}
#endif

	Super::Serialize(Ar);

#if WITH_EDITORONLY_DATA
	if (Ar.IsLoading())
	{
		bool Mod = false;

		BitChannels.Value = GetBitChannelChannelSetup();

		if (PrivateSenseReceiver)
		{
			const UClass* ObjClass = this->GetClass();
			if (ObjClass == UActiveSensor::StaticClass() || ObjClass == UPassiveSensor::StaticClass())
			{
				bOuter = true;
			}
			else
			{
				bOuter = false;
			}
		}

		if (Mod)
		{
			MarkPackageDirty_Internal();
		}
	}
#endif
}

void USensorBase::BeginDestroy()
{
	ResetInitialization();
	DestroyUpdateSensorTask();
	Super::BeginDestroy();
}

bool USensorBase::PreUpdateSensor()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_SensorPreUpdate);
	if (IsValidForTest() /* && IsValid(GetSenseReceiverComponent())*/)
	{
		for (const auto It : SensorTests)
		{
			if (It && It->NeedTest())
			{
				It->PreTest();
			}
		}
		return true;
	}
	return false;
}

bool USensorBase::UpdateSensor()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_FullUpdateSensor);

	{
		bool bPreValidation = false;
		if (IsValidForTest_Short())
		{
			if (const UWorld* World = GetWorld())
			{
				bPreValidation = World->IsGameWorld() && World->HasBegunPlay() && !World->bIsTearingDown && !World->IsPaused();
			}
		}
		if (UNLIKELY(!bPreValidation)) return false;
	}

	{
		//FScopeLock Lock_CriticalSection(&SensorCriticalSection);
		this->UpdateState = ESensorState::Update;
	}

	if (GetSensorUpdateReady() == EUpdateReady::Ready)
	{
		if (SensorType == ESensorType::Passive)
		{
			if (bIsHavePendingUpdate && GetSenseManager() && PreUpdateSensor())
			{
				if (const auto ContainerTree = GetSenseManager()->GetNamedContainerTree(SensorTag))
				{
					if (bIsHavePendingUpdate && SensorTests.Num() != 0)
					{
						TArray<uint16> OutIDs;
						{
							FScopeLock Lock_CriticalSection(&SensorCriticalSection);
							OutIDs = ContainerTree->CheckHash_TS(this->PendingUpdate);
							this->PendingUpdate.Empty();
						}
						bIsHavePendingUpdate = false;


						ContainerTree->MarkRemoveControl();

						const bool bRes = SensorsTestForSpecifyComponents_V3(ContainerTree, MoveTemp(OutIDs));

						if (ContainerTree) ContainerTree->ResetRemoveControl();


						//UpdateState = ESensorState::TestUpdated;
						if (bRes)
						{
							for (const FChannelSetup& Ch : ChannelSetup)
							{
								Ch._SenseDetect->NewSensedUpdate(DetectDepth, true, Ch.bNewSenseForcedByBestScore);
							}
						}
					}
					else
					{
						for (const FChannelSetup& Ch : ChannelSetup)
						{
							Ch._SenseDetect->EmptyUpdate(DetectDepth, true);
						}
					}
				}
			}
			else
			{
				//UpdateState = ESensorState::TestUpdated;
				for (const FChannelSetup& Ch : ChannelSetup)
				{
					Ch._SenseDetect->EmptyUpdate(DetectDepth, true);
				}
			}
		}
		else
		{
			const bool bDone = PreUpdateSensor() && RunSensorTest();
			if (!bDone)
			{
				return false;
			}
		}
	}
	//UpdateState = ESensorState::AgeUpdate;
	DetectionLostAndForgetUpdate();

	//UpdateState = ESensorState::PostUpdate;
	TreadSafePostUpdate();

	return true;
}

void USensorBase::TreadSafePostUpdate()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_SensorPostUpdate);
	if (IsValidForTest())
	{
		if (!IsInGameThread())
		{
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(PostUpdateDelegate, TStatId(), nullptr, ENamedThreads::GameThread);
		}
		else
		{
			PostUpdateSensor();
		}
	}
}

void USensorBase::PostUpdateSensor()
{
	check(IsInGameThread());
	if (IsValidForTest_Short())
	{
		UpdateState = ESensorState::NotUpdate;
		if (PendingNewChannels)
		{
			Add_SenseChannels(PendingNewChannels);
			check(PendingNewChannels == BitChannels.Value);
			PendingNewChannels = 0;
		}
		if (PendingRemoveChannels)
		{
			Remove_SenseChannels(PendingRemoveChannels);
			check(!(PendingRemoveChannels & BitChannels.Value));
			PendingRemoveChannels = 0;
		}

		if (bClearCurrentSense || bClearCurrentMemory)
		{
			if (bClearCurrentSense)
			{
				ClearCurrentSense(true);
			}
			else if (bClearCurrentMemory) //!bClearCurrentSense &&
			{
				if (GetSensorUpdateReady() == EUpdateReady::Ready)
				{
					OnSensorUpdated();
				}
				ClearCurrentMemorySense(true);
			}
		}
		else
		{
			if (GetSensorUpdateReady() == EUpdateReady::Ready)
			{
				OnSensorUpdated();
			}
			OnAgeUpdated();
		}
	}
}


//final callback target if sensor work finish
void USensorBase::OnSensorUpdated()
{
	check(IsInGameThread());
	if (IsValidForTest_Short())
	{
		for (FChannelSetup& Ch : ChannelSetup)
		{
			Ch.OnSensorChannelUpdated(SensorType);

			if (IsDetect(EOnSenseEvent::SenseNew))
			{
				if (Ch.NewSensed.Num() > 0)
				{
					OnSensorUpdateReceiver(EOnSenseEvent::SenseNew, Ch);
				}

				if (Ch.CurrentSensed.Num() > 0) //call order: new-> current-> lost-> forget
				{
					OnSensorUpdateReceiver(EOnSenseEvent::SenseCurrent, Ch);
				}

				if (IsDetect(EOnSenseEvent::SenseLost))
				{
					if (SensorType != ESensorType::Passive && Ch.LostCurrentSensed.Num() > 0)
					{
						OnSensorUpdateReceiver(EOnSenseEvent::SenseLost, Ch);
					}
				}
			}
			else
			{
				if (Ch.CurrentSensed.Num() > 0)
				{
					OnSensorUpdateReceiver(EOnSenseEvent::SenseCurrent, Ch);
				}
			}
		}

		if (NeedContinueTimer())
		{
			SensorTimer.ContinueTimer();
			//UE_LOG(LogSenseSys, Warning, TEXT(" %s ContinueTimer"), *GetNameSafe(this));
		}
	}
}

void USensorBase::OnAgeUpdated()
{
	check(IsInGameThread());
	check(GetSenseReceiverComponent());

	DestroyUpdateSensorTask();

	for (FChannelSetup& Ch : ChannelSetup)
	{
		Ch.OnSensorAgeUpdated(SensorType, GetSensorUpdateReady());

		if (IsDetect(EOnSenseEvent::SenseLost))
		{
			if ((SensorType != ESensorType::Active || GetSensorUpdateReady() == EUpdateReady::Skip) && Ch.LostAllSensed.Num() > 0)
			{
				OnSensorUpdateReceiver(EOnSenseEvent::SenseLost, Ch);
			}

			if (IsDetect(EOnSenseEvent::SenseForget))
			{
				if (Ch.ForgetSensed.Num() > 0)
				{
					OnSensorUpdateReceiver(EOnSenseEvent::SenseForget, Ch);
				}
			}
		}
	}
	if (!SensorTimer.IsStoppedTimer() && NeedStopTimer())
	{
		SensorTimer.StopTimer();
		//UE_LOG(LogSenseSys, Warning, TEXT("nothing to detect - stop tick timer : %s"), *GetNameSafe(this));
	}
}

//initialization
void USensorBase::InitializeFromReceiver(USenseReceiverComponent* FromReceiver)
{
	if (IsValid(FromReceiver))
	{
		PrivateSenseReceiver = FromReceiver;
		checkSlow(GetOuter() == GetSenseReceiverComponent());
		SensorTransform = FromReceiver->GetSensorTransform(SensorTag);

		//reset Ignored Arrays on Initialization
		Ignored_Actors.Empty();
		Ignored_Components.Empty();

		switch (SensorType)
		{
			case ESensorType::Active:
			{
				SensorTimer.ContinueTimer();
				break;
			}
			case ESensorType::Passive:
			{

				break;
			}
			case ESensorType::Manual:
			{
				SensorTimer.StopTimer();
				break;
			}
			case ESensorType::None:
			{
				//log error
				UE_LOG(LogSenseSys, Error, TEXT("sensor: %s InitFromReceiver %s, "), *GetNameSafe(this), *GetNameSafe(FromReceiver));
			}
		}

		{
			SensorTest_WithAllSensePoint = nullptr;
			for (USensorTestBase* const It : SensorTests)
			{
				if (It && It->GetOuter() == this)
				{
					It->InitializeFromSensor();

					const auto LocationTest = Cast<USensorLocationTestBase>(It);
					if (SensorTest_WithAllSensePoint == nullptr)
					{
						if (LocationTest && LocationTest->bTestBySingleLocation == false)
						{
							SensorTest_WithAllSensePoint = It;
						}
						else if (LocationTest == nullptr)
						{
							SensorTest_WithAllSensePoint = It;
						}
					}
				}
			}
		}
	}
}

void USensorBase::InitializeForSense(USenseReceiverComponent* FromReceiver)
{
	if (IsValid(FromReceiver) && FromReceiver == GetSenseReceiverComponent())
	{
		SetUpdateTimeRate(UpdateTimeRate);
		PrivateSenseManager = GetSenseReceiverComponent()->GetSenseManager();
		AddIgnoreActor(GetSensorOwner());

		if (GetSenseManager() != nullptr)
		{
			for (FChannelSetup& It : ChannelSetup)
			{
				It.Init();
			}
			UpdateState = ESensorState::NotUpdate;
		}

#if WITH_EDITOR
		check(Algo::IsSorted(ChannelSetup, TLess<uint8>()));
		check(!ArraySorted::ContainsDuplicates_Sorted(ChannelSetup));
#endif
	}
}

void USensorBase::Cleanup()
{
	UpdateState = ESensorState::Uninitialized;
	SensorCriticalSection.Lock();
	this->PostUpdateDelegate.Unbind();
	this->bEnable = false;
	SensorCriticalSection.Unlock();

	DestroyUpdateSensorTask();

	for (USensorTestBase* It : SensorTests)
	{
		if (It)
		{
			It->bEnableTest = false;
			It->MarkPendingKill();
		}
	}

	MarkPendingKill();

	if (UpdateState.Get() < ESensorState::Update) //force clean
	{
		SensorTests.Empty();
		//ChannelSetup.Empty(); //todo ChannelSetup.Empty() On USensorBase::Cleanup(

		Ignored_Components.Empty();
		Ignored_Actors.Empty();

		PrivateSenseManager = nullptr;
		PrivateSenseReceiver = nullptr;
	}
	else
	{
		//UE_LOG(LogSenseSys, Warning, TEXT("---Call Cleanup but sensor in update: %s, Component: %s, Owner: %s---"), *GetNameSafe(this), *GetNameSafe(PrivateSenseReceiver), *GetNameSafe(PrivateSenseReceiver->GetOwner()));
	}
}

void USensorBase::GetSensorTest_BoxAndRadius(FBox& OutBox, float& OutRadius) const
{
	OutRadius = 0.f;
	OutBox = FBox(FVector::ZeroVector, FVector::ZeroVector);

	for (const USensorTestBase* St : SensorTests)
	{
		const float Rad = St->GetSensorTestRadius();
		if (!FMath::IsNearlyZero(Rad) && (FMath::IsNearlyZero(OutRadius) || OutRadius > Rad))
		{
			OutRadius = Rad;
		}

		const FBox LocBox = St->GetSensorTestBoundBox();

		const float Current = OutBox.GetExtent().SizeSquared();
		const bool bNotZero = !FMath::IsNearlyZero(LocBox.GetExtent().SizeSquared());
		if (FMath::IsNearlyZero(Current) && bNotZero)
		{
			OutBox = LocBox;
		}
		else if (bNotZero)
		{
			OutBox = OutBox.Overlap(LocBox);
		}
	}

	if (OutRadius > KINDA_SMALL_NUMBER && IsZeroBox(OutBox))
	{
		const FVector Ext = FVector(OutRadius * 0.5f);
		OutBox = FBox::BuildAABB(GetSensorTransform().GetLocation(), Ext);
	}
}

void USensorBase::SetSensorThreadType(const ESensorThreadType NewSensorThreadType)
{
	if (SensorThreadType != NewSensorThreadType)
	{
		const ESensorThreadType OldSensorThreadType = SensorThreadType;
		SensorThreadType = NewSensorThreadType;
		const auto OwnerReceiver = GetSenseReceiverComponent();
		check(OwnerReceiver);
		const bool bOldRec = OwnerReceiver->ContainsSenseThreadSensors();

		const bool bNeedCallSM =
			(!bOldRec && (NewSensorThreadType == ESensorThreadType::Sense_Thread || NewSensorThreadType == ESensorThreadType::Sense_Thread_HighPriority)) ||
			(bOldRec && (OldSensorThreadType == ESensorThreadType::Sense_Thread || OldSensorThreadType == ESensorThreadType::Sense_Thread_HighPriority));

		if (bNeedCallSM)
		{
			GetSenseReceiverComponent()->UpdateContainsSenseThreadSensors();
			GetSenseManager()->ChangeSensorThreadType(OwnerReceiver, NewSensorThreadType, OldSensorThreadType);
		}
	}
}

void USensorBase::SetEnableSensor(const bool bInEnable)
{
	if (UpdateState > ESensorState::ReadyToUpdate)
	{
		FScopeLock Lock_CriticalSection(&SensorCriticalSection);
		this->bEnable = bInEnable;
	}
	else
	{
		bEnable = bInEnable;
	}
}

void USensorBase::SetSenseChannelParam(const uint8 InChannel, const bool NewSenseForcedByBestScore, const int32 TrackBestScoreCount, const float MinBestScore)
{
	const int32 ID = GetChannelID(InChannel);
	if (ID != INDEX_NONE)
	{
		const bool bLocInUpdate = UpdateState > ESensorState::ReadyToUpdate;
		if (bLocInUpdate)
		{
			SensorCriticalSection.Lock();
		}

		this->ChannelSetup[ID].bNewSenseForcedByBestScore = NewSenseForcedByBestScore;
		this->ChannelSetup[ID].TrackBestScoreCount = TrackBestScoreCount;
		this->ChannelSetup[ID].MinBestScore = MinBestScore;

		if (bLocInUpdate)
		{
			SensorCriticalSection.Unlock();
		}
	}
}

void USensorBase::SetNewSenseForcedByBestScore(const uint8 InChannel, const bool bValue)
{
	const int32 ID = GetChannelID(InChannel);
	if (ID != INDEX_NONE)
	{
		const bool bLocInUpdate = UpdateState > ESensorState::ReadyToUpdate;
		if (bLocInUpdate)
		{
			SensorCriticalSection.Lock();
		}

		this->ChannelSetup[ID].bNewSenseForcedByBestScore = bValue;

		if (bLocInUpdate)
		{
			SensorCriticalSection.Unlock();
		}
	}
}

void USensorBase::SetBestScore_Count(const uint8 InChannel, const int32 Count)
{
	const int32 ID = GetChannelID(InChannel);
	if (ID != INDEX_NONE)
	{
		const bool bLocInUpdate = UpdateState > ESensorState::ReadyToUpdate;
		if (bLocInUpdate)
		{
			SensorCriticalSection.Lock();
		}

		this->ChannelSetup[ID].SetTrackBestScoreCount(Count);

		if (bLocInUpdate)
		{
			SensorCriticalSection.Unlock();
		};
	}
}

void USensorBase::SetBestScore_MinScore(const uint8 InChannel, const float MinScore)
{
	const int32 ID = GetChannelID(InChannel);
	if (ID != INDEX_NONE)
	{
		const bool bLocInUpdate = UpdateState > ESensorState::ReadyToUpdate;
		if (bLocInUpdate)
		{
			SensorCriticalSection.Lock();
		}

		this->ChannelSetup[ID].SetMinBestScore(MinScore);

		if (bLocInUpdate)
		{
			SensorCriticalSection.Unlock();
		};
	}
}

void USensorBase::SetUpdateTimeRate(const float NewUpdateTimeRate)
{
	const bool bLocInUpdate = UpdateState > ESensorState::ReadyToUpdate;
	if (bLocInUpdate)
	{
		SensorCriticalSection.Lock();
	}

	this->SensorTimer.SetUpdateTimeRate(UpdateTimeRate);

	if (bLocInUpdate)
	{
		SensorCriticalSection.Unlock();
	}
}

void USensorBase::AddIgnoreActor(AActor* IgnoreActor)
{
	if (IsValid(IgnoreActor))
	{
		Ignored_Actors.AddUnique(IgnoreActor);
		const auto Ssc = USenseSystemBPLibrary::GetStimulusFromActor(IgnoreActor);
		if (IsValid(Ssc))
		{
			FScopeLock Lock_CriticalSection(&SensorCriticalSection);
			HashSorted::InsertUnique(this->Ignored_Components, Ssc);
		}
	}
	RemoveNullsIgnoreActorsAndComponents();
	//OnChangeIgnoredActors.Broadcast();
}

void USensorBase::AddIgnoreActors(TArray<AActor*> IgnoredActors)
{
	for (auto IgnoreActor : IgnoredActors)
	{
		if (IsValid(IgnoreActor))
		{
			Ignored_Actors.AddUnique(IgnoreActor);
			const auto Ssc = USenseSystemBPLibrary::GetStimulusFromActor(IgnoreActor);
			if (IsValid(Ssc))
			{
				FScopeLock Lock_CriticalSection(&SensorCriticalSection);
				HashSorted::InsertUnique(this->Ignored_Components, Ssc);
			}
		}
	}
	RemoveNullsIgnoreActorsAndComponents();
	//OnChangeIgnoredActors.Broadcast();
}

void USensorBase::RemoveIgnoredActor(AActor* Actor)
{
	Ignored_Actors.Remove(Actor);
	const auto Ssc = USenseSystemBPLibrary::GetStimulusFromActor(Actor);
	if (Ssc)
	{
		FScopeLock Lock_CriticalSection(&SensorCriticalSection);
		HashSorted::Remove_HashType(this->Ignored_Components, GetTypeHash(Ssc));
	}
	RemoveNullsIgnoreActorsAndComponents();
	//OnChangeIgnoredActors.Broadcast();
}

void USensorBase::RemoveIgnoredActors(TArray<AActor*> Actors)
{
	for (auto It : Actors)
	{
		Ignored_Actors.Remove(It);
		const auto Ssc = USenseSystemBPLibrary::GetStimulusFromActor(It);
		if (Ssc)
		{
			FScopeLock Lock_CriticalSection(&SensorCriticalSection);
			HashSorted::Remove_HashType(this->Ignored_Components, GetTypeHash(Ssc));
		}
	}
	RemoveNullsIgnoreActorsAndComponents();
	//OnChangeIgnoredActors.Broadcast();
}

void USensorBase::ResetIgnoredActors()
{
	FScopeLock Lock_CriticalSection(&SensorCriticalSection);
	this->Ignored_Actors.Empty();
	this->Ignored_Components.Empty();
	this->AddIgnoreActor(this->GetSensorOwner());

	//OnChangeIgnoredActors.Broadcast();
}

void USensorBase::RemoveNullsIgnoreActorsAndComponents()
{
	ArrayHelpers::Filter_Sorted_V2(Ignored_Actors, FNotValidObjPredicate());
#if WITH_EDITOR
	checkf(
		!ArrayHelpers::Filter_Sorted_V2(Ignored_Components, FNotValidObjPredicate()),
		TEXT("dont need refresh Ignored_Components with Unregister Stimulus function"));
#endif
}


TArray<FStimulusFindResult> USensorBase::UnRegisterSenseStimulus(USenseStimulusBase* Ssc)
{
	if (!IsPendingKill() && IsInitialized() && Ssc)
	{
		checkSlow(IsValid(Ssc));
		Ignored_Actors.Remove(Ssc->GetOwner());

		if (const FStimulusTagResponse* StrPtr = Ssc->GetStimulusTagResponse(SensorTag))
		{
			if ((BitChannels.Value & (*StrPtr).BitChannels.Value & ~IgnoreBitChannels.Value))
			{
				const uint16 InStimulusID = (*StrPtr).GetObjID();
				if (InStimulusID != MAX_uint16)
				{
					FScopeLock Lock_CriticalSection(&SensorCriticalSection);
					this->PendingUpdate.Remove(InStimulusID);
				}

				TArray<FStimulusFindResult> FindResult = FindStimulusInAllState(Ssc, *StrPtr, BitChannels);
				for (int32 i = 0; i < FindResult.Num(); ++i)
				{
					FStimulusFindResult& It = FindResult[i];
					const int32 ChanIdx = It.Sensor->GetChannelID(It.Channel);
					FChannelSetup& Ch = It.Sensor->ChannelSetup[ChanIdx];

					ESensorArrayByType NewType = It.SensedType;
					if (It.SensedType == ESensorArrayByType::SenseCurrent)
					{
						if (Ch.BestSensedID_ByScore.Num() > 0)
						{
							checkSlow(Algo::IsSorted(Ch.BestSensedID_ByScore, FSortScorePredicate2(Ch.CurrentSensed)));
							const int32 FindIndex = Algo::BinarySearch(Ch.BestSensedID_ByScore, It.SensedID, FSortScorePredicate2(Ch.CurrentSensed));
							checkSlow(FindIndex == Ch.BestSensedID_ByScore.IndexOfByKey(It.SensedID));
							if (FindIndex != INDEX_NONE)
							{
								Ch.BestSensedID_ByScore.RemoveAt(FindIndex, 1, false);
							}
							for (int32& BestIt : Ch.BestSensedID_ByScore)
							{
								if (BestIt > It.SensedID)
								{
									--BestIt;
								}
							}
						}

						if (HashSorted::Remove_HashType(Ch.NewSensed, GetTypeHash(Ssc), false) != INDEX_NONE)
						{
							NewType = ESensorArrayByType::SensedNew;
						}
					}
					if (It.SensedType == ESensorArrayByType::SenseLost)
					{
						if (HashSorted::Remove_HashType(Ch.LostCurrentSensed, GetTypeHash(Ssc), false) != INDEX_NONE)
						{
							NewType = ESensorArrayByType::SenseCurrentLost;
						}
					}

					TArray<FSensedStimulus>& Array = Ch.GetSensedStimulusBySenseEvent(It.SensedType);
					Array.RemoveAt(It.SensedID, 1, false);
#if WITH_EDITOR
					for (const int32 BestIt : Ch.BestSensedID_ByScore)
					{
						checkSlow(BestIt < Ch.CurrentSensed.Num());
					}
#endif

					It.SensedID = INDEX_NONE;
					It.SensedType = NewType;
				}

				return FindResult;
			}
		}
	}
	return TArray<FStimulusFindResult>{};
}


/********************************/

void USensorBase::ReportSenseStimulusEvent(USenseStimulusBase* SenseStimulus)
{
	if (IsValid(SenseStimulus))
	{
		if (const FStimulusTagResponse* StrPtr = SenseStimulus->GetStimulusTagResponse(SensorTag))
		{
			const uint16 InStimulusID = (*StrPtr).GetObjID();
			if (InStimulusID != MAX_uint16)
			{
				ReportSenseStimulusEvent(InStimulusID);
			}
		}
	}
}

void USensorBase::ReportSenseStimulusEvent(const uint16 InStimulusID)
{
	if (InStimulusID != MAX_uint16 && IsValidForTest_Short() && !IsPendingKill() && bEnable)
	{
		check(IsInGameThread());
		if (SensorThreadType == ESensorThreadType::Main_Thread) //ReportSenseStimulusEvent only for Main_Thread
		{
			if (PreUpdateSensor())
			{
				UpdateState = ESensorState::Update;

				const auto ContainerTree = GetSenseManager()->GetNamedContainerTree(SensorTag);
				if (ContainerTree && GetSenseManager())
				{
					TSet<uint16> IDs = {InStimulusID};
					if (bIsHavePendingUpdate && ContainerTree && IsValidForTest() && bIsHavePendingUpdate)
					{
						TArray<uint16, TMemStackAllocator<>> OutIDs;
						{
							FScopeLock Lock_CriticalSection(&SensorCriticalSection);
							OutIDs = ContainerTree->CheckHashStack_TS(this->PendingUpdate);
							this->PendingUpdate.Empty();
						}
						bIsHavePendingUpdate = false;
						IDs.Append(OutIDs);
					}

					const bool bDone = SensorsTestForSpecifyComponents_V3(ContainerTree, MoveTemp(IDs));
					if (bDone)
					{
						//UpdateState = ESensorState::TestUpdated;
						for (const FChannelSetup& Ch : ChannelSetup)
						{
							Ch._SenseDetect->NewSensedUpdate(DetectDepth, IsOverrideSenseState(), Ch.bNewSenseForcedByBestScore);
						}
					}
					else
					{
						//checkNoEntry();
						OnSensorReadyFail();
					}
				}

				OnSensorUpdated();
				SensorTimer.ContinueTimer();
				UpdateState = ESensorState::NotUpdate;
			}
		}
		else
		{
			const auto ContainerTree = GetSenseManager()->GetNamedContainerTree(SensorTag);
			if (ContainerTree)
			{
				const uint32 TmpHash = ContainerTree->GetSensedStimulusCopy_Simple_TS(InStimulusID).TmpHash;
				if (TmpHash != MAX_uint32)
				{
					{
						FScopeLock Lock_CriticalSection(&SensorCriticalSection);
						this->PendingUpdate.Add(InStimulusID, TmpHash);
					}
					bIsHavePendingUpdate = true;
					const uint8 UpS = static_cast<uint8>(UpdateState.Get());
					if (UpS < static_cast<uint8>(ESensorState::Update))
					{
						SensorTimer.ForceNextTickTimer();
					}
					SensorTimer.ContinueTimer();
				}
			}
		}
	}
}


bool USensorBase::RunSensorTest()
{
	if (IsValidForTest_Short())
	{
		FBox Box;
		float Radius;
		GetSensorTest_BoxAndRadius(Box, Radius);

		if (BitChannels.Value & ~IgnoreBitChannels.Value && SensorTests.Num() != 0)
		{
			const auto SM = GetSenseManager();
			if (LIKELY(SM))
			{
				const auto ContainerTree = SM->GetNamedContainerTree(SensorTag);
				if (LIKELY(ContainerTree))
				{
					const IContainerTree& ContainerTreeRef = *ContainerTree;
					if (!IsZeroBox(Box))
					{
						TSet<uint16> IDs;
						ContainerTreeRef.MarkRemoveControl();
						if (Radius == 0.f)
						{
							ContainerTreeRef.GetInBoxIDs(Box, IDs, BitChannels.Value);
						}
						//else if (Box.GetExtent().SizeSquared() == 0.f)
						//{
						//	ContainerTreeRef.GetInRadiusIDs(Radius, GetSensorTransform().GetLocation(), IDs, BitChannels.Value);
						//}
						else
						{
							ContainerTreeRef.GetInBoxRadiusIDs(Box, GetSensorTransform().GetLocation(), Radius, IDs, BitChannels.Value);
						}

						if (bIsHavePendingUpdate && ContainerTree && IsValidForTest_Short())
						{
							TArray<uint16, TMemStackAllocator<>> OutIDs;
							{
								FScopeLock Lock_CriticalSection(&SensorCriticalSection);
								OutIDs = ContainerTree->CheckHashStack_TS(this->PendingUpdate);
								this->PendingUpdate.Empty();
							}
							bIsHavePendingUpdate = false;
							IDs.Append(MoveTemp(OutIDs));
						}

						if (ContainerTree && IsValidForTest_Short())
						{
							if (IDs.Num())
							{
								const bool bDoneSensorsTest = SensorsTestForSpecifyComponents_V3(ContainerTree, MoveTemp(IDs));
								if (ContainerTree)
								{
									ContainerTree->ResetRemoveControl();
								}
								if (bDoneSensorsTest)
								{
									UpdateState = ESensorState::TestUpdated;
									for (const FChannelSetup& Chan : ChannelSetup)
									{
										Chan._SenseDetect->NewSensedUpdate(DetectDepth, IsOverrideSenseState(), Chan.bNewSenseForcedByBestScore);
									}
									return true;
								}
							}
							else if (IsValidForTest_Short())
							{
								ContainerTreeRef.ResetRemoveControl();
								UpdateState = ESensorState::TestUpdated;

								for (const FChannelSetup& Chan : ChannelSetup)
								{
									Chan._SenseDetect->EmptyUpdate(DetectDepth, IsOverrideSenseState());
								}
								return true;
							}
#if WITH_EDITOR
							check(ContainerTree->IsRemoveControlClear());
#endif
						}
					}

					OnSensorReadyFail();
					return false;
				}
			}
		}
		else
		{
			for (const FChannelSetup& Chan : ChannelSetup)
			{
				Chan._SenseDetect->EmptyUpdate(DetectDepth, IsOverrideSenseState());
			}
			return true;
		}
	}
	else
	{
		OnSensorReadyFail();
	}
	return false;
}

/******************************/

float USensorBase::UpdtDetectPoolAndReturnMinScore() const
{
	float MinScore = MIN_flt;
	for (const FChannelSetup& Chan : ChannelSetup)
	{
		auto& Sd = *(Chan._SenseDetect);
		Sd.EmptyArr(ESensorArrayByType::SenseForget);
		Sd.DetectNew.Reset();
		Sd.DetectCurrent.Reset();
		MinScore = FMath::Max(MinScore, Chan.MinBestScore);
	}
	return MinScore;
}

bool USensorBase::UpdtSensorTestForIDInternal(
	const uint16 Idx,
	const IContainerTree* ContainerTree,
	const float CurrentTime,
	const float MinScore,
	TArray<uint16>& ChannelContainsIDs) const
{
	if (LIKELY(IsValidForTest_Short() && ContainerTree))
	{
		FSensedStimulus It = ContainerTree->GetSensedStimulusCopy_TS(Idx);
		if (It.TmpHash != MAX_uint32)
		{
			const bool bNotIgnored = !HashSorted::Contains_HashType(Ignored_Components, It.TmpHash);
			if (bNotIgnored)
			{
				const ESenseTestResult TotalResult = Sensor_Run_Test(MinScore, CurrentTime, It, ChannelContainsIDs);
				if (TotalResult != ESenseTestResult::Lost && TotalResult != ESenseTestResult::None)
				{
					if (LIKELY(IsInitialized()))
					{
						It.SensedTime = CurrentTime;

						for (int32 i = 0; i < ChannelSetup.Num(); i++)
						{
							const FChannelSetup& ChanIt = ChannelSetup[i];

							if (TotalResult == ESenseTestResult::Sensed)
							{
								uint16& Outi = ChannelContainsIDs[i];
								Outi = ChanIt._SenseDetect->ContainsInCurrentSense(It);
								if (Outi == MAX_uint16)
								{
									It.FirstSensedTime = CurrentTime;
									Outi = ChannelSetup[i]._SenseDetect->ContainsInLostSense(It);
									if (Outi != MAX_uint16)
									{
										FSenseDetectPool& Pool = *ChanIt._SenseDetect;
										Pool.GetPool()[Outi].FirstSensedTime = CurrentTime;
									}
								}
							}

							if (It.BitChannels & ChanIt.GetSenseBitChannel() && ChanIt.MinBestScore <= It.Score)
							{
								ChanIt._SenseDetect->Add(CurrentTime, It, ChannelContainsIDs[i]);
							}
						}
					}
					else
					{
						return true;
					}
				}
			}
		}
	}
	else
	{
		return true;
	}
	return false;
}

ESenseTestResult USensorBase::Sensor_Run_Test(const float MinScore, const float CurrentTime, FSensedStimulus& Stimulus, TArray<uint16>& Out) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_SensorTests);

	Out.Init(MAX_uint16, ChannelSetup.Num());

	bool bOnceContainsGate = false;
	bool bOnceGate = false;

	ESenseTestResult TotalResult = ESenseTestResult::None;
	Stimulus.BitChannels &= (BitChannels.Value & ~IgnoreBitChannels.Value);

	for (int32 i = 0; i < SensorTests.Num(); ++i) //run sensors test for SensedStimulus struct
	{
		if (LIKELY(IsInitialized())) // && Stimulus.TmpHash != MAX_uint32
		{
			const USensorTestBase* STest = SensorTests[i];
			if (STest && STest->NeedTest())
			{
				if (!bOnceGate && STest == SensorTest_WithAllSensePoint) //update score for sense points
				{
					for (int32 j = 1; j < Stimulus.SensedPoints.Num(); j++)
					{
						Stimulus.SensedPoints[j].PointScore = Stimulus.Score;
					}
					bOnceGate = true;
				}

				const ESenseTestResult Result = STest->RunTest(Stimulus);
				TotalResult = static_cast<ESenseTestResult>(FMath::Max(static_cast<uint8>(TotalResult), static_cast<uint8>(Result)));

				if (TotalResult == ESenseTestResult::Lost)
				{
					return ESenseTestResult::Lost;
				}
				if (MinScore > Stimulus.Score)
				{
					TotalResult = ESenseTestResult::NotLost;
				}
				if (TotalResult == ESenseTestResult::NotLost && !bOnceContainsGate)
				{
					for (int32 j = 0; j < ChannelSetup.Num(); ++j)
					{
						const FChannelSetup& ChanIt = ChannelSetup[j];
						const uint64 Chan = ChanIt.GetSenseBitChannel();
						if (Stimulus.BitChannels & Chan)
						{
							Out[j] = ChanIt._SenseDetect->ContainsInCurrentSense(Stimulus);
							if (Out[j] == MAX_uint16)
							{
								Stimulus.BitChannels &= ~Chan;
							}
						}
					}
					if (Stimulus.BitChannels == 0)
					{
						return ESenseTestResult::Lost;
					}
					bOnceContainsGate = true;
				}
			}
		}
		else
		{
			return ESenseTestResult::Lost;
		}
	}

	return TotalResult;
}


void USensorBase::CheckWithCurrent(FSensedStimulus& SS, TArray<uint16>& Out) const
{
	for (int32 i = 0; i < ChannelSetup.Num(); i++)
	{
		const uint64 Chan = ChannelSetup[i].GetSenseBitChannel();
		if (SS.BitChannels & Chan)
		{
			Out[i] = ChannelSetup[i]._SenseDetect->ContainsInCurrentSense(SS);
			if (Out[i] == MAX_uint16)
			{
				SS.BitChannels &= ~Chan;
			}
		}
	}
}

float USensorBase::GetCurrentGameTimeInSeconds() const
{
	if (const UWorld* World = GetWorld())
	{
		return World->GetTimeSeconds();
	}
	return 0.f;
}


void USensorBase::DetectionLostAndForgetUpdate()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_SensorAge);
	const float CurrentTime = GetCurrentGameTimeInSeconds();
	if (IsValidForTest_Short() && CurrentTime > 0.f)
	{
		for (const FChannelSetup& Ch : ChannelSetup)
		{
			Ch._SenseDetect->NewAgeUpdate(CurrentTime, DetectDepth);
		}
	}
}

/******************************/

void USensorBase::Add_SenseChannels(uint64 NewChannels)
{
	if (NewChannels != 0)
	{
		check(UpdateState < ESensorState::Update);
		check(IsInGameThread());
		int32 i = 0;

		while (NewChannels)
		{
			if ((NewChannels & 1llu) && !(BitChannels.Value & (1llu << i)))
			{
				const int32 ID = ArraySorted::InsertUniqueSorted(ChannelSetup, FChannelSetup(i + 1), TLess<uint8>());
				ChannelSetup[ID].Init();
			}
			NewChannels = NewChannels >> 1;
			i++;
		}

		BitChannels.Value = GetBitChannelChannelSetup();
	}
}

void USensorBase::Remove_SenseChannels(uint64 NewChannels)
{
	if (NewChannels != 0)
	{
		check(UpdateState < ESensorState::Update);
		check(IsInGameThread());

		int32 i = 0;

		while (NewChannels)
		{
			if ((NewChannels & 1llu) && (BitChannels.Value & (1llu << i)))
			{
				const int32 ID = GetChannelID(i);
				if (ID != INDEX_NONE)
				{
					ChannelSetup.RemoveAt(ID);
				}
			}
			NewChannels = NewChannels >> 1;
			i++;
		}
		BitChannels.Value = GetBitChannelChannelSetup();
	}
}


void USensorBase::SetIgnoreSenseChannelsBit(const FBitFlag64_SenseSys InChannels)
{
	FScopeLock Lock_CriticalSection(&SensorCriticalSection);
	this->IgnoreBitChannels = InChannels;
}

void USensorBase::SetIgnoreSenseResponseChannelBit(const uint8 InChannel, const bool bEnableChannel)
{
	if (InChannel > 0 && InChannel <= 64)
	{
		const uint64 Bit = 1llu << (InChannel - 1);
		FScopeLock Lock_CriticalSection(&SensorCriticalSection);
		if (!bEnableChannel)
		{
			this->IgnoreBitChannels.Value &= ~Bit;
		}
		else if (bEnableChannel)
		{
			this->IgnoreBitChannels.Value |= Bit;
		}
	}
}

void USensorBase::SetSenseChannelsBit(const FBitFlag64_SenseSys InChannels)
{
	SensorCriticalSection.Lock();
	const uint64 CopyBitChannels = this->BitChannels.Value;
	SensorCriticalSection.Unlock();

	PendingNewChannels |= InChannels.Value;
	PendingRemoveChannels |= CopyBitChannels & ~PendingNewChannels;

	if (UpdateState < ESensorState::Update)
	{
		FScopeLock Lock_CriticalSection(&SensorCriticalSection);
		if (this->PendingNewChannels)
		{
			this->Add_SenseChannels(this->PendingNewChannels);
			this->PendingNewChannels = 0;
		}
		if (PendingRemoveChannels)
		{
			this->Remove_SenseChannels(this->PendingRemoveChannels);
			this->PendingRemoveChannels = 0;
		}
	}
}

void USensorBase::SetSenseResponseChannelBit(const uint8 InChannel, const bool bEnableChannel)
{
	if (InChannel > 0 && InChannel <= 64)
	{
		const uint64 BitChan = 1llu << (InChannel - 1);
		if (UpdateState > ESensorState::ReadyToUpdate)
		{
			if (bEnableChannel)
			{
				PendingNewChannels |= BitChan;
			}
			else
			{
				PendingRemoveChannels |= BitChan;
			}
		}
		else
		{
			FScopeLock Lock_CriticalSection(&SensorCriticalSection);
			if (bEnableChannel)
			{
				this->Add_SenseChannels(BitChan);
			}
			else
			{
				this->Remove_SenseChannels(BitChan);
			}
		}
	}
}


bool USensorBase::CheckResponseChannel(const USenseStimulusBase* StimulusComponent) const
{
	if (StimulusComponent && ChannelSetup.Num() > 0)
	{
		if (const FStimulusTagResponse* StrPtr = StimulusComponent->GetStimulusTagResponse(SensorTag))
		{
			return (BitChannels.Value & (*StrPtr).BitChannels.Value & ~IgnoreBitChannels.Value);
		}
	}
	return false;
}

/******************************/

AActor* USensorBase::GetSensorOwner() const
{
	if (GetSenseReceiverComponent())
	{
		return GetSenseReceiverComponent()->GetOwner();
	}
	return nullptr;
}

AActor* USensorBase::GetSensorOwner_BP() const
{
	return GetSensorOwner();
}

AActor* USensorBase::GetSensorOwnerAs(TSubclassOf<AActor> ActorClass, ESuccessState& SuccessState) const
{
	AActor* OwnerPtr = GetSensorOwner();
	if (IsValid(OwnerPtr) && OwnerPtr->IsA(ActorClass))
	{
		SuccessState = ESuccessState::Success;
	}
	else
	{
		SuccessState = ESuccessState::Failed;
	}
	return OwnerPtr;
}

USenseReceiverComponent* USensorBase::GetSenseReceiverComponent_BP() const
{
	return GetSenseReceiverComponent();
}

USenseManager* USensorBase::GetSenseManager_BP() const
{
	return GetSenseManager();
}

/******************************/

class UWorld* USensorBase::GetWorld() const
{
	if (IsValid(PrivateSenseReceiver))
	{
		return GetSenseReceiverComponent()->GetWorld();
	}
	return nullptr;
}

void USensorBase::TickSensor(const float DeltaTime)
{
	if (bEnable && IsInitialized() && BitChannels.Value != 0)
	{
		const ESensorState State = UpdateState.Get();
		switch (State)
		{
			case ESensorState::NotUpdate:
			{
				if (SensorTimer.TickTimer(DeltaTime))
				{
					TrySensorUpdate();
				}
				break;
			}
			case ESensorState::ReadyToUpdate:
			{
				SensorTransform = GetSenseReceiverComponent()->GetSensorTransform(SensorTag);
				break;
			}
			default: break;
		}
	}
}

void USensorBase::TrySensorUpdate()
{
	SensorUpdateReady = GetSensorReady();
	switch (GetSensorUpdateReady())
	{
		case EUpdateReady::None: break;
		case EUpdateReady::Ready:
		{
			OnSensorReady();
			break;
		}
		case EUpdateReady::Skip:
		{
			OnSensorReadySkip();
			break;
		}
		case EUpdateReady::Fail:
		{
			OnSensorReadyFail();
			break;
		}
		default: break;
	}
}

EUpdateReady USensorBase::GetSensorReady()
{
	auto Out = EUpdateReady::None;
	if (IsValidForTest())
	{
		SensorTransform = GetSenseReceiverComponent()->GetSensorTransform(SensorTag);
		for (USensorTestBase* const It : SensorTests)
		{
			if (It && It->bEnableTest)
			{
				const auto t = It->GetReadyToTest();
				It->bSkipTest = (t == EUpdateReady::Skip);
				Out = FMath::Max(Out, t); //skip all if skip one
			}
		}
	}
	return FMath::Max(GetSensorReadyBP(), Out);
}

void USensorBase::OnSensorReady()
{
	if (IsValidForTest())
	{
		if (GetSenseManager()->HaveSenseStimulus())
		{
			UpdateState = ESensorState::ReadyToUpdate;
			SensorTransform = GetSenseReceiverComponent()->GetSensorTransform(SensorTag);
			switch (SensorThreadType)
			{
				case ESensorThreadType::Main_Thread:
				{
					UpdateSensor();
					break;
				}
				case ESensorThreadType::Sense_Thread:
				{
					const bool bSuccess = GetSenseManager()->RequestAsyncSenseUpdate(this, false);
					if (!bSuccess) UpdateState = ESensorState::NotUpdate;
					break;
				}
				case ESensorThreadType::Sense_Thread_HighPriority:
				{
					const bool bSuccess = GetSenseManager()->RequestAsyncSenseUpdate(this, true);
					if (!bSuccess) UpdateState = ESensorState::NotUpdate;
					break;
				}
				case ESensorThreadType::Async_Task:
				{
					CreateUpdateSensorTask();
					break;
				}
				default:
				{
					checkNoEntry();
				}
			}
		}
	}
}

void USensorBase::OnSensorReadySkip()
{
	if (IsValidForTest_Short())
	{
		UpdateState = ESensorState::NotUpdate;

		for (FChannelSetup& It : ChannelSetup)
		{
			It.NewSensed.Empty();
			It.LostCurrentSensed.Empty();

			if (It._SenseDetect)
			{
				It._SenseDetect->NewCurrent.Empty();
				It._SenseDetect->LostCurrent.Empty();
			}
			OnSensorReady();
		}
	}
}

void USensorBase::OnSensorReadyFail()
{
	if (IsValidForTest_Short())
	{
		ClearCurrentSense(true);

		UpdateState = ESensorState::NotUpdate;
	}
	UE_LOG(LogSenseSys, Error, TEXT("OnSensorReadyFail: %s"), *GetNameSafe(this));
}


int32 USensorBase::FindInSortedArray(const TArray<FSensedStimulus>& Array, const FSensedStimulus& SensedStimulus) const
{
	if (SensedStimulus.StimulusComponent.IsValid())
	{
		return FindInSortedArray(Array, SensedStimulus.StimulusComponent.Get());
	}
	return INDEX_NONE;
}

int32 USensorBase::FindInSortedArray(const TArray<FSensedStimulus>& Array, const USenseStimulusBase* SensedStimulus) const
{
	if (SensedStimulus)
	{
		return HashSorted::BinarySearch_HashType(Array, GetTypeHash(SensedStimulus));
	}
	return INDEX_NONE;
}

int32 USensorBase::FindInSortedArray(const TArray<FSensedStimulus>& Array, const AActor* Actor) const
{
	const auto SensedStimulus = USenseSystemBPLibrary::GetStimulusFromActor(Actor);
	if (SensedStimulus)
	{
		return HashSorted::BinarySearch_HashType(Array, GetTypeHash(SensedStimulus));
	}
	return INDEX_NONE;
}


bool USensorBase::FindComponent(const USenseStimulusBase* Comp, const ESensorArrayByType SenseState, FSensedStimulus& SensedStimulus, const uint8 InChannel)
	const
{
	if (Comp && InChannel > 0 && InChannel <= 64)
	{
		const int32 ChannelID = GetChannelID(InChannel);
		if (ChannelID != INDEX_NONE)
		{
			const auto& Tmp = GetSensedStimulusBySenseEvent(SenseState, ChannelID);
			const int32 ID = FindInSortedArray(Tmp, Comp);
			if (ID != INDEX_NONE)
			{
				SensedStimulus = Tmp[ID];
				return true;
			}
		}
	}
	return false;
}

TArray<FStimulusFindResult> USensorBase::FindStimulusInAllState(
	const USenseStimulusBase* StimulusComponent,
	const FStimulusTagResponse& Str,
	FBitFlag64_SenseSys InChannels)
{
	TArray<FStimulusFindResult> Out;
	if (Str.IsResponseChannel(InChannels))
	{
		for (const auto& ChIt : ChannelSetup)
		{
			const uint64 ChanValue = ChIt.GetSenseBitChannel();
			if ((ChanValue & InChannels) & Str.BitChannels.Value)
			{
				constexpr ESensorArrayByType ByTypeArray[3] = //
					{
						ESensorArrayByType::SenseCurrent, //
						ESensorArrayByType::SenseForget,  //
						ESensorArrayByType::SenseLost	  //
					};

				for (int32 i = 0; i < 3; ++i)
				{
					const auto& SSArray = ChIt.GetSensedStimulusBySenseEvent(ByTypeArray[i]);
					const int32 ID = FindInSortedArray(SSArray, StimulusComponent);
					if (ID != INDEX_NONE)
					{
						FStimulusFindResult Res;
						Res.SensorType = SensorType;
						Res.Sensor = this;
						Res.SensorTag = SensorTag;
						Res.Channel = ChIt.Channel;
						Res.SensedType = ByTypeArray[i];
						Res.SensedData = SSArray[ID];
						Res.SensedID = ID;
						Out.Add(MoveTemp(Res));
						break;
					}
				}
			}
		}
	}
	return Out;
}
TArray<FStimulusFindResult> USensorBase::FindStimulusInAllState(const USenseStimulusBase* StimulusComponent, FBitFlag64_SenseSys InChannels)
{
	TArray<FStimulusFindResult> Out;
	if (IsValid(StimulusComponent))
	{
		InChannels &= BitChannels.Value & ~IgnoreBitChannels.Value;
		if (const FStimulusTagResponse* Response = StimulusComponent->GetStimulusTagResponse(SensorTag))
		{
			Out = FindStimulusInAllState(StimulusComponent, *Response, InChannels);
			for (FStimulusFindResult& It : Out)
			{
				const int32 ChanIdx = It.Sensor->GetChannelID(It.Channel);
				const FChannelSetup& Ch = It.Sensor->ChannelSetup[ChanIdx];

				if (It.SensedType == ESensorArrayByType::SenseCurrent)
				{
					const int32 Idx = HashSorted::BinarySearch_HashType(Ch.NewSensed, GetTypeHash(StimulusComponent));
					if (Idx != INDEX_NONE)
					{
						It.SensedID = Idx;
						It.SensedType = ESensorArrayByType::SensedNew;
					}
				}
				else if (It.SensedType == ESensorArrayByType::SenseLost)
				{
					const int32 Idx = HashSorted::BinarySearch_HashType(Ch.LostCurrentSensed, GetTypeHash(StimulusComponent));
					if (Idx != INDEX_NONE)
					{
						It.SensedID = Idx;
						It.SensedType = ESensorArrayByType::SenseCurrentLost;
					}
				}
			}
		}
	}
	return Out;
}


FStimulusFindResult USensorBase::FindStimulusInAllState_SingleChannel(const USenseStimulusBase* StimulusComponent, const uint8 InChannel)
{
	FStimulusFindResult Out;
	FBitFlag64_SenseSys LocChannel = //
		(InChannel != 0)			 //
		? (1llu << (InChannel - 1))
		: 0;

	if (IsValid(StimulusComponent) && (LocChannel & ~IgnoreBitChannels.Value))
	{
		if (StimulusComponent->IsResponseChannel(SensorTag, InChannel))
		{
			const int32 ChanIdx = GetChannelID(InChannel);
			if (ChanIdx != INDEX_NONE)
			{
				const auto& ChIt = ChannelSetup[ChanIdx];
				const uint32 Hash = GetTypeHash(StimulusComponent);

				constexpr ESensorArrayByType ByTypeArray[3] = //
					{
						ESensorArrayByType::SenseCurrent, //
						ESensorArrayByType::SenseForget,  //
						ESensorArrayByType::SenseLost	  //
					};

				for (int32 i = 0; i < 3; ++i)
				{
					const auto& SSArray = ChIt.GetSensedStimulusBySenseEvent(ByTypeArray[i]);
					const int32 ID = HashSorted::BinarySearch_HashType(SSArray, Hash);
					if (ID != INDEX_NONE)
					{
						Out.SensorType = SensorType;
						Out.Sensor = this;
						Out.Channel = ChIt.Channel;

						if (ByTypeArray[i] == ESensorArrayByType::SenseCurrent)
						{
							const int32 Idx = HashSorted::BinarySearch_HashType(ChIt.NewSensed, Hash);
							if (Idx != INDEX_NONE)
							{
								Out.SensedID = Idx;
								Out.SensedType = ESensorArrayByType::SensedNew;
							}
						}
						else if (ByTypeArray[i] == ESensorArrayByType::SenseLost)
						{
							const int32 Idx = HashSorted::BinarySearch_HashType(ChIt.LostCurrentSensed, Hash);
							if (Idx != INDEX_NONE)
							{
								Out.SensedID = Idx;
								Out.SensedType = ESensorArrayByType::SenseCurrentLost;
							}
						}
						else
						{
							Out.SensedType = ByTypeArray[i];
							Out.SensedID = ID;
						}

						Out.SensedData = ChIt.GetSensedStimulusBySenseEvent(Out.SensedType)[ID];
						return Out;
					}
				}
			}
		}
	}
	return Out;
}

TArray<FStimulusFindResult> USensorBase::FindActorInAllState(const AActor* Actor, FBitFlag64_SenseSys InChannel)
{
	return FindStimulusInAllState(USenseSystemBPLibrary::GetStimulusFromActor(Actor), InChannel);
}


bool USensorBase::FindActor(const AActor* Actor, const ESensorArrayByType SenseState, FSensedStimulus& SensedStimulus, const uint8 InChannel) const
{
	if (Actor && InChannel > 0 && InChannel <= 64)
	{
		const auto Comp = USenseSystemBPLibrary::GetStimulusFromActor(Actor);
		if (Comp)
		{
			return FindComponent(Comp, SenseState, SensedStimulus, InChannel);
		}
	}
	return false;
}

bool USensorBase::ContainsComponent(const USenseStimulusBase* Comp, const ESensorArrayByType SenseState, const uint8 InChannel) const
{
	if (Comp && InChannel > 0 && InChannel <= 64)
	{
		const int32 ChannelID = GetChannelID(InChannel);
		if (ChannelID != INDEX_NONE)
		{
			const auto& Tmp = GetSensedStimulusBySenseEvent(SenseState, ChannelID);
			checkSlow(HashSorted::IsSortedByHash(Tmp));
			const int32 ID = FindInSortedArray(Tmp, Comp);
			if (ID != INDEX_NONE)
			{
				return true;
			}
		}
	}
	return false;
}


bool USensorBase::ContainsActor(const AActor* Actor, const ESensorArrayByType SenseState, const uint8 InChannel) const
{
	const USenseStimulusBase* Comp = USenseSystemBPLibrary::GetStimulusFromActor(Actor);
	return ContainsComponent(Comp, SenseState, InChannel);
}


AActor* USensorBase::GetSensedActorByClass(const TSubclassOf<AActor> ActorClass, const ESensorArrayByType SenseState, const uint8 InChannel) const
{
	if (ActorClass != nullptr)
	{
		const int32 ChannelID = GetChannelID(InChannel);
		if (ChannelID != INDEX_NONE)
		{
			const auto& Tmp = GetSensedStimulusBySenseEvent(SenseState, ChannelID);
			for (const auto& It : Tmp)
			{
				if (It.StimulusComponent.IsValid())
				{
					AActor* Actor = It.StimulusComponent->GetOwner();
					if (Actor && Actor->IsA(ActorClass))
					{
						return Actor;
					}
				}
			}
		}
	}
	return nullptr;
}

TArray<AActor*> USensorBase::GetSensedActorsByClass(const TSubclassOf<AActor> ActorClass, const ESensorArrayByType SenseState, const uint8 InChannel) const
{
	TArray<AActor*> OutActors;
	if (ActorClass != nullptr && InChannel > 0 && InChannel <= 64)
	{
		const int32 ChannelID = GetChannelID(InChannel);
		if (ChannelID != INDEX_NONE)
		{
			const auto& Tmp = GetSensedStimulusBySenseEvent(SenseState, ChannelID);
			for (const auto& It : Tmp)
			{
				if (It.StimulusComponent.IsValid())
				{
					if (AActor* Actor = It.StimulusComponent.Get()->GetOwner())
					{
						if (Actor->IsA(ActorClass))
						{
							OutActors.Add(Actor);
						}
					}
				}
			}
		}
	}
	return OutActors;
}

bool USensorBase::GetActorLocationFromMemory_ByComponent(const USenseStimulusBase* Comp, FVector& Location, const uint8 InChannel) const
{
	FSensedStimulus FindElem;
	FindComponent(Comp, ESensorArrayByType::SenseLost, FindElem, InChannel);
	if (FindElem.StimulusComponent.IsValid())
	{
		if (const AActor* Actor = FindElem.StimulusComponent.Get()->GetOwner())
		{
			Location = Actor->GetActorLocation();
		}
		return true;
	}
	return false;
}

bool USensorBase::GetActorLocationFromMemory_ByActor(const AActor* Actor, FVector& Location, const uint8 InChannel) const
{
	Location = FVector::ZeroVector;
	const auto Comp = USenseSystemBPLibrary::GetStimulusFromActor(Actor);
	return GetActorLocationFromMemory_ByComponent(Comp, Location, InChannel);
}


void USensorBase::ClearCurrentSense(const bool bCallUpdate)
{
	if (UpdateState == ESensorState::NotUpdate)
	{
		if (IsValidForTest_Short())
		{
			UpdateState = ESensorState::Update;

			for (const FChannelSetup& Ch : ChannelSetup)
			{
				Ch._SenseDetect->EmptyUpdate(DetectDepth, true);
			}

			if (bCallUpdate)
			{
				OnSensorUpdated();
				if (bClearCurrentMemory)
				{
					UpdateState = ESensorState::NotUpdate;
					ClearCurrentMemorySense(true);
				}
				else
				{
					OnAgeUpdated();
				}
			}
			UpdateState = ESensorState::NotUpdate;
			bClearCurrentSense = false;
		}
	}
	else
	{
		FScopeLock Lock_CriticalSection(&SensorCriticalSection);
		this->bClearCurrentSense = true;
	}
}

void USensorBase::ClearCurrentMemorySense(const bool bCallForget)
{
	if (UpdateState == ESensorState::NotUpdate)
	{
		if (IsValidForTest_Short())
		{
			UpdateState = ESensorState::AgeUpdate;

			for (FChannelSetup& CS : ChannelSetup)
			{
				FSenseDetectPool& Sd = *CS._SenseDetect;

				Sd.ResetArr(ESensorArrayByType::SenseForget);
				Sd.Forget = Sd.Lost;
				Sd.Lost.Empty();
				Sd.LostCurrent.Empty();

				if (bCallForget)
				{
					OnAgeUpdated(); //call lost and forget
				}

				Sd.EmptyArr(ESensorArrayByType::SenseForget);
			}

			UpdateState = ESensorState::NotUpdate;
			bClearCurrentMemory = false;
		}
	}
	else
	{
		FScopeLock Lock_CriticalSection(&SensorCriticalSection);
		this->bClearCurrentMemory = true;
	}
}

void USensorBase::ForgetAllSensed()
{
	ClearCurrentSense(true);
	ClearCurrentMemorySense(true);
}

/********************************/

TArray<FSensedStimulus> USensorBase::FindBestScoreSensed(uint8 InChannel, int32 Count) const
{
	struct FSortScorePredicate
	{
		explicit FSortScorePredicate(const TArray<FSensedStimulus>& InRef) : Ref(InRef) {}
		FORCEINLINE bool operator()(const int32 A, const int32 B) const { return Ref[A].Score > Ref[B].Score; };

	private:
		const TArray<FSensedStimulus>& Ref;
	};

	TArray<FSensedStimulus> Out;
	Out.Reserve(Count);

	if (InChannel > 0 && InChannel <= 64)
	{
		const int32 ChannelID = GetChannelID(InChannel);
		if (ChannelID != INDEX_NONE)
		{
			TArrayView<const int32> ArrayView;

			const auto& Ch = ChannelSetup[ChannelID];
			const auto& SensedArr = Ch.CurrentSensed;

			TArray<int32> OutIdx;
			if (Count > Ch.BestSensedID_ByScore.Num())
			{
				FSenseDetectPool::BestIdsByPredicate(Count, SensedArr, FSortScorePredicate(SensedArr), OutIdx);
				ArrayView = TArrayView<const int32>(OutIdx.GetData(), OutIdx.Num());
			}
			else
			{
				const auto& ScoreIdx = Ch.BestSensedID_ByScore;
				ArrayView = TArrayView<const int32>(ScoreIdx.GetData(), Count);
			}

			for (int32 i : ArrayView)
			{
				Out.Add(SensedArr[i]);
			}
		}
	}
	return Out;
}

TArray<FSensedStimulus> USensorBase::FindBestAgeSensed(uint8 InChannel, int32 Count) const
{
	struct FSortAgePredicate
	{
		explicit FSortAgePredicate(const TArray<FSensedStimulus>& InPoolRef, float InTime) : PoolRef(InPoolRef), Time(InTime) {}
		FORCEINLINE bool operator()(const int32 A, const int32 B) const { return AgeScore(PoolRef[A]) > AgeScore(PoolRef[B]); };
		FORCEINLINE float AgeScore(const FSensedStimulus& A) const { return A.Score * ((A.Age - (Time - A.SensedTime)) / A.Age); }

	private:
		const TArray<FSensedStimulus>& PoolRef;
		float Time = 0.f;
	};

	TArray<FSensedStimulus> Out;
	Out.Reserve(Count);

	if (InChannel > 0 && InChannel <= 64)
	{
		const int32 ChannelID = GetChannelID(InChannel);
		if (ChannelID != INDEX_NONE)
		{
			TArray<int32> OutIdx;
			const auto& Arr = ChannelSetup[ChannelID].LostAllSensed;
			FSenseDetectPool::BestIdsByPredicate(Count, Arr, FSortAgePredicate(Arr, GetCurrentGameTimeInSeconds()), OutIdx);

			check(OutIdx.Num() <= Count);
			for (int32 i : OutIdx)
			{
				Out.Add(Arr[i]);
			}
		}
	}
	return Out;
}


TArray<FSensedStimulus> USensorBase::GetBestSensedByScore(int32 Count, const uint8 InChannel) const
{
	return FindBestScoreSensed(InChannel, Count);
}

/********************************/

bool USensorBase::GetBestSenseStimulus(FSensedStimulus& SensedStimulus, const uint8 InChannel) const
{
	TArray<FSensedStimulus> Result = FindBestScoreSensed(InChannel, 1);
	if (Result.IsValidIndex(0))
	{
		SensedStimulus = Result[0];
		return true;
	}
	return false;
}

USenseStimulusBase* USensorBase::GetBestSenseStimulusComponent(const uint8 InChannel) const
{
	FSensedStimulus SensedStimulus;
	if (GetBestSenseStimulus(SensedStimulus, InChannel))
	{
		return SensedStimulus.StimulusComponent.Get();
	}
	return nullptr;
}

class AActor* USensorBase::GetBestSenseActor(const uint8 Channel) const
{
	FSensedStimulus SensedStimulus;
	if (GetBestSenseStimulus(SensedStimulus, Channel) && SensedStimulus.StimulusComponent.IsValid())
	{
		return SensedStimulus.StimulusComponent.Get()->GetOwner();
	}
	return nullptr;
}

/********************************/


void USensorBase::OnSensorUpdateReceiver(const EOnSenseEvent SenseEvent, const FChannelSetup& InChannelSetup) const
{
	if (IsValidForTest())
	{
		check(IsInGameThread());
		const TArray<FSensedStimulus>& InSensedStimulus = InChannelSetup.GetSensedStimulusBySenseEvent(SenseEvent);
		if (InSensedStimulus.Num() > 0)
		{
			if (const USenseReceiverComponent* Receiver = GetSenseReceiverComponent())
			{
				const auto& Delegate = Receiver->GetDelegateBySenseEvent(SenseEvent);
				if (Delegate.IsBound())
				{
					Delegate.Broadcast(this, static_cast<int32>(InChannelSetup.Channel), InSensedStimulus);
				}

				if (IsCallOnSense(SenseEvent))
				{
					for (int32 i = InSensedStimulus.Num() - 1; i >= 0; --i)
					{
						if (InSensedStimulus[i].StimulusComponent.IsValid())
						{
							if (const auto Comp = InSensedStimulus[i].StimulusComponent.Get())
							{
								if (Comp->IsRegisteredForSense() && Comp->bEnable)
								{
									Comp->SensedFromSensorUpdate(this, InChannelSetup.Channel, SenseEvent);
								}
							}
						}
					}
				}

				Receiver->TrackTargets(InSensedStimulus, SenseEvent);
			}
		}
	}
}


bool USensorBase::NeedContinueTimer()
{
	//if timer UpdateTimeRate have a valid value
	//Active Sensor always ticking
	//if need age Update
	if (bIsHavePendingUpdate)
	{
		return true;
	}

	if (SensorType == ESensorType::Active) return true;

	if (IsDetect(EOnSenseEvent::SenseForget))
	{
		for (const FChannelSetup& Chan : ChannelSetup)
		{
			if (Chan.LostAllSensed.Num() > 0) return true;
		}
	}
	return false;
	//return  SensorType == ESensorType::Active || (IsDetect(EOnSenseEvent::SenseForget) && SenseDetect.LostAll_Sensed.Num());
}


void USensorBase::DrawDebugSensor(bool bTest, bool bCurrentSensed, bool bLostSensed, bool bBestSensed, bool bAge, float Duration) const
{
#if WITH_EDITORONLY_DATA
	DrawDebug(bTest, bCurrentSensed, bLostSensed, bBestSensed, bAge, Duration);
#endif
}


TArray<FSensedStimulus> USensorBase::GetSensedStimulusBySenseEvent_BP(ESensorArrayByType SenseEvent, uint8 InChannel)
{
	TArray<FSensedStimulus> Out;
	const int32 ChannelID = GetChannelID(InChannel);
	if (ChannelSetup.IsValidIndex(ChannelID))
	{
		FScopeLock Lock_CriticalSection(&SensorCriticalSection);
		Out = GetSensedStimulusBySenseEvent(SenseEvent, ChannelID);
	}
	return MoveTemp(Out);
}


#if WITH_EDITOR

TArray<uint8> USensorBase::GetUniqueChannelSetup() const
{
	TArray<uint8> Tmp;
	Tmp.Reserve(ChannelSetup.Num());

	for (const FChannelSetup& Chan : ChannelSetup)
	{
		Tmp.AddUnique(Chan.Channel);
	}
	return Tmp;
}

void USensorBase::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	const FName PropertyName = (e.Property != nullptr) ? e.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(USensorBase, SensorTag))
	{
		if (SensorTag == NAME_None)
		{
			SensorTag = *(this->GetClass()->GetName());
		}
	}
	if (PropertyName == GET_MEMBER_NAME_CHECKED(USensorBase, SensorTests))
	{
		CheckAndRestoreSensorTestDefaults();
	}

	{
		TArray<uint8> Tmp = GetUniqueChannelSetup();
		TArray<uint8> Tmp2 = Tmp;
		for (FChannelSetup& It : ChannelSetup)
		{
			if (Tmp2.Contains(It.Channel))
			{
				Tmp2.Remove(It.Channel);
			}
			else
			{
				It.Channel = GetUniqueChannel(Tmp, It.Channel);
				Tmp.Add(It.Channel);
			}
		}
	}

	Algo::Sort(ChannelSetup, TLess<uint8>());
	check(!ArraySorted::ContainsDuplicates_Sorted(ChannelSetup));
	BitChannels.Value = GetBitChannelChannelSetup();

	Super::PostEditChangeProperty(e);
}

EDataValidationResult USensorBase::IsDataValid(TArray<FText>& ValidationErrors)
{
	EDataValidationResult eIsValid = Super::IsDataValid(ValidationErrors);

	for (const auto It : SensorTests)
	{
		if (!IsValid(It))
		{
			const FText ErrText = FText::FromString((TEXT("Sensor containes Invalid Sensor test: %s"), *GetNameSafe(It)));
			ValidationErrors.Add(ErrText);
			eIsValid = CombineDataValidationResults(eIsValid, EDataValidationResult::Invalid);
		}
	}

	if (!Algo::IsSorted(ChannelSetup, TLess<uint8>()))
	{
		Algo::Sort(ChannelSetup, TLess<uint8>());
		const FText ErrText = FText::FromString((TEXT("ChannelSetup is Unsorted in USensorBase: %s"), *GetNameSafe(this)));
		ValidationErrors.Add(ErrText);
		eIsValid = CombineDataValidationResults(eIsValid, EDataValidationResult::Invalid);
	}

	if (ArraySorted::ContainsDuplicates_Sorted(ChannelSetup))
	{
		const auto Predicate = [](const TArray<FChannelSetup>& A, const int32 ID)
		{ return (ID > 0 && A[ID] == A[ID - 1]) || A[ID].Channel > 64 || A[ID].Channel <= 0; };
		ArrayHelpers::Filter_Sorted(ChannelSetup, Predicate);
		const FText ErrText = FText::FromString((TEXT("Duplicates in ChannelSetup USensorBase: %s"), *GetNameSafe(this)));
		ValidationErrors.Add(ErrText);
		eIsValid = CombineDataValidationResults(eIsValid, EDataValidationResult::Invalid);
	}

	return eIsValid;
}

uint8 USensorBase::GetUniqueChannel(const TArray<uint8>& Tmp, const uint8 Start /*= 0*/)
{
	for (uint8 i = Start; i < 255; i++)
	{
		if (!Tmp.Contains(i))
		{
			return i;
		}
	}
	checkNoEntry();
	return 0;
}


#endif //WITH_EDITOR


#if WITH_EDITORONLY_DATA
// clang-format off

void USensorBase::DrawSensor(const FSceneView* View, FPrimitiveDrawInterface* PDI) const
{
	const UWorld* World = GetWorld();
	if (!bDrawSensor || !bEnable || !World) return;

	FSenseSysDebugDraw DebugDraw;
	if (GetSenseManager() && World->IsGameWorld())
	{
		DebugDraw = GetSenseManager()->GetSenseSysDebugDraw(SensorTag);
	}
	else
	{
		DebugDraw = FSenseSysDebugDraw(false);
		DebugDraw.Sensor_DebugTest = true;
	}

	if (DebugDraw.Sensor_DebugTest)
	{
		for (const USensorTestBase* It : SensorTests)
		{
			if (It && It->bEnableTest)
			{
				It->DrawTest(View, PDI);
			}
		}
	}

	if (DebugDraw.Sensor_DebugCurrentSensed || DebugDraw.Sensor_DebugLostSensed || DebugDraw.Sensor_DebugBestSensed)
	{
		constexpr float Thickness = 1.2f;
		constexpr float DepthBias = 1.f;
		constexpr bool bScreenSpace = false;
		constexpr int32 CircleSide = 4;
		constexpr uint8 DepthPriority = SDPG_Foreground;

		const FQuat ViewRot = FQuat(View->ViewRotation);
		const FVector Dir = ViewRot.GetAxisX();
		const FVector Yax = ViewRot.GetAxisY();
		const FVector Zax = ViewRot.GetAxisZ();

		if (DebugDraw.Sensor_DebugCurrentSensed)
		{
			for (int32 ChannelID = 0; ChannelID < ChannelSetup.Num(); ChannelID++)
			{
				for (const FSensedStimulus& SensedStimulus : ChannelSetup[ChannelID].CurrentSensed)
				{
					const FVector SenseLoc = SensedStimulus.SensedPoints[0].SensedPoint;
					DrawCircle(PDI, SenseLoc, Yax, Zax, FColor::Red, EDebugSenseSysHelpers::DebugLargeSize, CircleSide, DepthPriority, 1.f, DepthBias, bScreenSpace); // SDPG_World 

					for (int32 i = 1; i < SensedStimulus.SensedPoints.Num(); i++)
					{
						const auto& V = SensedStimulus.SensedPoints[i];
						FColor LocColor;
						switch (V.PointTestResult)
						{
							case ESenseTestResult::Sensed:
							{
								LocColor = FColor::Red;
								break;
							}
							case ESenseTestResult::NotLost:
							{
								LocColor = FColor::Orange;
								break;
							}
							default:
							{
								LocColor = FColor::Black;
								break;
							}
						}
						DrawCircle(PDI, V.SensedPoint, Yax, Zax,
							LocColor, EDebugSenseSysHelpers::DebugSmallSize, CircleSide, DepthPriority,
							Thickness + 0.5f, DepthBias, bScreenSpace);
					}
				}
			}
		}

		if (DebugDraw.Sensor_DebugBestSensed)
		{
			for (int32 ChannelID = 0; ChannelID < ChannelSetup.Num(); ChannelID++)
			{
				if (ChannelSetup[ChannelID].BestSensedID_ByScore.Num())
				{
					const FVector LocYax = Yax.RotateAngleAxis(45.f, Dir);
					const FVector LocZax = Zax.RotateAngleAxis(45.f, Dir);

					for (int32 ID : ChannelSetup[ChannelID].BestSensedID_ByScore)
					{
						DrawCircle(PDI, ChannelSetup[ChannelID].CurrentSensed[ID].SensedPoints[0].SensedPoint, LocYax, LocZax, FColor::Red,
							EDebugSenseSysHelpers::DebugLargeSize, CircleSide,
							DepthPriority, Thickness + 0.5f, DepthBias, bScreenSpace);
					}
				}
			}
		}

		if (DebugDraw.Sensor_DebugLostSensed)
		{
			for (int32 ChannelID = 0; ChannelID < ChannelSetup.Num(); ChannelID++)
			{
				for (const FSensedStimulus& SensedStimulus : ChannelSetup[ChannelID].LostAllSensed)
				{
					//if component unexpected destroyed? simple validation do not work
					const auto Comp = Cast<USenseStimulusBase>(SensedStimulus.StimulusComponent);
					if (Comp && Comp->bEnable && Comp->GetOwner())
					{
						const FVector ActorLoc = Comp->GetOwner()->GetActorLocation();
						const FVector& SenseLoc = SensedStimulus.SensedPoints[0].SensedPoint;

						DrawCircle(PDI, SenseLoc, Yax, Zax,
							FColor::Yellow, EDebugSenseSysHelpers::DebugLargeSize,
							CircleSide, DepthPriority, Thickness, DepthBias, bScreenSpace);

						if (!SenseLoc.Equals(ActorLoc, 25.f))
						{
							PDI->DrawLine(ActorLoc, SenseLoc, FColor::Yellow, DepthPriority, 2.f);

							DrawCircle(PDI, ActorLoc, Yax, Zax,
								FColor::Orange, EDebugSenseSysHelpers::DebugLargeSize,
								CircleSide, DepthPriority, Thickness, DepthBias, bScreenSpace);
						}
					}
				}
			}
		}
	}
}

void USensorBase::DrawSensorHUD(const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) const
{
	const UWorld* World = GetWorld();
	if (!bDrawSensor || !bEnable || !World) return;

	FSenseSysDebugDraw DebugDraw;
	if (GetSenseManager() && World->IsGameWorld())
	{
		DebugDraw = GetSenseManager()->GetSenseSysDebugDraw(SensorTag);
	}
	else
	{
		DebugDraw = FSenseSysDebugDraw(false);
		DebugDraw.Sensor_DebugTest = true;
	}

	if (DebugDraw.Sensor_DebugTest)
	{
		for (const USensorTestBase* It : SensorTests)
		{
			if (It && It->bEnableTest)
			{
				It->DrawTestHUD(Viewport, View, Canvas);
			}
		}
	}

	if (World->IsGameWorld() && World->HasBegunPlay())
	{
		const auto Font = GEngine->GetLargeFont();
		if (DebugDraw.Stimulus_DebugCurrentSensed && DebugDraw.SenseSys_DebugScore)
		{
			for (int32 ChannelID = 0; ChannelID < ChannelSetup.Num(); ChannelID++)
			{
				for (const auto& SensedStimulus : ChannelSetup[ChannelID].CurrentSensed)
				{
					FVector2D PixLoc;
					View->WorldToPixel(SensedStimulus.SensedPoints[0].SensedPoint, PixLoc);
					const FString S = FString::Printf(TEXT("Score: %f"), SensedStimulus.Score);
					Canvas->DrawShadowedString(PixLoc.X, PixLoc.Y, *S, Font, FLinearColor::White, FLinearColor(0, 0, 0, 0));
				}
			}
		}

		if (DebugDraw.Sensor_DebugBestSensed)
		{
			const FString HS = FString::Printf(TEXT("Score: %f"), 0.f);
			const float H = Font->GetStringHeightSize(*HS);

			for (int32 ChannelID = 0; ChannelID < ChannelSetup.Num(); ChannelID++)
			{
				if (ChannelSetup[ChannelID].BestSensedID_ByScore.Num())
				{
					for (const int32 ID : ChannelSetup[ChannelID].BestSensedID_ByScore)
					{
						FVector2D PixLoc;
						{
							View->WorldToPixel(ChannelSetup[ChannelID].CurrentSensed[ID].SensedPoints[0].SensedPoint, PixLoc);
						}
						const FString S = FString::Printf(TEXT("BestTarget: %s"), *SensorTag.ToString());
						Canvas->DrawShadowedString(PixLoc.X, PixLoc.Y + H, *S, Font, FLinearColor::Red, FLinearColor(0, 0, 0, 0));
					}
				}
			}
		}

		if (IsDetect(EOnSenseEvent::SenseForget) && DebugDraw.SenseSys_DebugAge && DebugDraw.Sensor_DebugLostSensed)
		{
			const float Sec = GetCurrentGameTimeInSeconds();
			for (int32 ChannelID = 0; ChannelID < ChannelSetup.Num(); ChannelID++)
			{
				for (const auto& SensedStimulus : ChannelSetup[ChannelID].LostAllSensed)
				{
					FVector2D PixLoc;
					{
						View->WorldToPixel(SensedStimulus.SensedPoints[0].SensedPoint, PixLoc);
					}
					const FString S = FString::Printf(TEXT("Age: %3.3f"), Sec - SensedStimulus.SensedTime);
					Canvas->DrawShadowedString(PixLoc.X, PixLoc.Y, *S, Font, FLinearColor::Yellow, FLinearColor(0, 0, 0, 0));
				}
			}
		}
	}
}

void USensorBase::DrawDebug(bool bTest, bool bCurrentSensed, bool bLostSensed, bool bBestSensed, bool bAge, float Duration) const
{
	#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	if (!bEnable || !World) return;

	APlayerController* PlayerController = nullptr;
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		PlayerController = Iterator->Get();
		if (PlayerController)
		{
			break;
		}
	}

	if (bTest)
	{
		for (const USensorTestBase* It : SensorTests)
		{
			if (It && It->bEnableTest)
			{
				It->DrawDebug(Duration);
			}
		}
	}

	if (bCurrentSensed || bLostSensed || bBestSensed)
	{
		constexpr float Thickness = 1.2f;
		constexpr int32 CircleSide = 4;
		constexpr uint8 DepthPriority = SDPG_Foreground;

		const FQuat ViewRot = FQuat(PlayerController->GetControlRotation());
		const FVector Dir = ViewRot.GetAxisX();
		const FVector Yax = ViewRot.GetAxisY();
		const FVector Zax = ViewRot.GetAxisZ();
		const FTransform T = GetSensorTransform();
		if (bCurrentSensed)
		{
			for (int32 ChannelID = 0; ChannelID < ChannelSetup.Num(); ChannelID++)
			{
				for (const auto& SensedStimulus : ChannelSetup[ChannelID].CurrentSensed)
				{
					const FVector SenseLoc = SensedStimulus.SensedPoints[0].SensedPoint;

					DrawDebugCircle(World, SenseLoc, EDebugSenseSysHelpers::DebugLargeSize, CircleSide,
						FColor::Red, false, Duration, DepthPriority, 1.f,
						Yax, Zax, false);

					DrawDebugDirectionalArrow(World, T.GetLocation(), SenseLoc, 5625.f,
						FColor::Red, false, Duration, DepthPriority, 2.f);

					for (int32 i = 1; i < SensedStimulus.SensedPoints.Num(); i++)
					{
						const auto& V = SensedStimulus.SensedPoints[i];
						FColor LocColor;
						switch (V.PointTestResult)
						{
							case ESenseTestResult::Sensed:
							{
								LocColor = FColor::Red;
								break;
							}
							case ESenseTestResult::NotLost:
							{
								LocColor = FColor::Orange;
								break;
							}
							default:
							{
								LocColor = FColor::Black;
								break;
							}
						}						
						DrawDebugCircle(World, V.SensedPoint, EDebugSenseSysHelpers::DebugSmallSize, CircleSide,
							LocColor, false, Duration, DepthPriority, Thickness + 0.5f,
							Yax, Zax, false);
					}
				}
			}
		}

		if (bBestSensed)
		{
			for (int32 ChannelID = 0; ChannelID < ChannelSetup.Num(); ChannelID++)
			{
				if (ChannelSetup[ChannelID].BestSensedID_ByScore.Num())
				{
					const FVector LocYax = Yax.RotateAngleAxis(45.f, Dir);
					const FVector LocZax = Zax.RotateAngleAxis(45.f, Dir);

					for (int32 ID : ChannelSetup[ChannelID].BestSensedID_ByScore)
					{
						const FVector _V = ChannelSetup[ChannelID].CurrentSensed[ID].SensedPoints[0].SensedPoint;
						DrawDebugCircle(World, _V, EDebugSenseSysHelpers::DebugLargeSize, CircleSide,
							FColor::Red, false, Duration, DepthPriority, Thickness + 0.5f,
							LocYax, LocZax, false);
						const FString S = FString::Printf(TEXT("BestTarget: %s"), *SensorTag.ToString());
						DrawDebugString(World, _V, S, nullptr, FColor::Red, Duration);
					}
				}
			}
		}

		if (bLostSensed)
		{
			const float Sec = bAge ? GetCurrentGameTimeInSeconds() : 0.f;
			for (int32 ChannelID = 0; ChannelID < ChannelSetup.Num(); ChannelID++)
			{
				for (const auto& SensedStimulus : ChannelSetup[ChannelID].LostAllSensed)
				{
					//if component unexpected destroyed? simple validation do not work
					const auto Comp = Cast<USenseStimulusBase>(SensedStimulus.StimulusComponent);
					if (Comp && Comp->bEnable && Comp->GetOwner())
					{
						const FVector ActorLoc = Comp->GetOwner()->GetActorLocation();
						const FVector& SenseLoc = SensedStimulus.SensedPoints[0].SensedPoint;

						DrawDebugCircle(World, SenseLoc, EDebugSenseSysHelpers::DebugLargeSize, CircleSide,
							FColor::Yellow, false, Duration, DepthPriority, Thickness,
							Yax, Zax, false);
						
						DrawDebugDirectionalArrow(World, T.GetLocation(), SenseLoc, 5625.f,
							FColor::Yellow, false, Duration, DepthPriority, 2.f);

						if (!SenseLoc.Equals(ActorLoc, 25.f))
						{
							DrawDebugLine(World, ActorLoc, SenseLoc, FColor::Yellow, false, Duration, DepthPriority, 2.f);

							DrawDebugCircle(World, ActorLoc, EDebugSenseSysHelpers::DebugLargeSize, CircleSide,
								FColor::Yellow, false, Duration, DepthPriority, Thickness,
								Yax, Zax, false);
						}
						if (bAge)
						{
							const FString S = FString::Printf(TEXT("Age: %3.3f"), Sec - SensedStimulus.SensedTime);
							DrawDebugString(World, SensedStimulus.SensedPoints[0].SensedPoint, S, nullptr, FColor::Yellow, Duration);
						}
					}
				}
			}
		}
	}
	#endif
}

// clang-format on


#endif //WITH_EDITORONLY_DATA


USensorTestBase* USensorBase::CreateNewSensorTest(const TSubclassOf<USensorTestBase> SensorTestClass, int32 SensorTestIndexPlace)
{

#if WITH_EDITOR
	if (bEnable)
	{
		if (const auto Receiver = GetSenseReceiverComponent())
		{
			UE_LOG(
				LogSenseSys,
				Error,
				TEXT("Call CreateNewSensorTest but Sensor is Enable=true, disable sensor before add new test, Sensor: %s, Receiver: %s, Actor: %s"),
				*GetNameSafe(this),
				*GetNameSafe(Receiver),
				*GetNameSafe(Receiver->GetOwner()));
		}
	}
#endif //WITH_EDITOR

	const UWorld* World = GetWorld();
	const bool bPlayWorld = World && World->IsGameWorld() && World->HasBegunPlay() && !World->bIsTearingDown;
	if (!IsPendingKill() && SensorTestClass != nullptr && bPlayWorld && !bEnable)
	{
		USensorTestBase* STest = NewObject<USensorTestBase>(this, SensorTestClass);

		SensorTestIndexPlace = FMath::Clamp<int32>(SensorTestIndexPlace, 0, SensorTests.Num());

		SensorTests.Insert(STest, SensorTestIndexPlace);

		SensorTest_WithAllSensePoint = nullptr;
		for (USensorTestBase* const It : SensorTests)
		{
			if (It && It->GetOuter() == this)
			{
				It->InitializeFromSensor();

				const auto LocationTest = Cast<USensorLocationTestBase>(It);
				if (SensorTest_WithAllSensePoint == nullptr)
				{
					if (LocationTest && LocationTest->bTestBySingleLocation == false)
					{
						SensorTest_WithAllSensePoint = It;
					}
					else if (LocationTest == nullptr)
					{
						SensorTest_WithAllSensePoint = It;
					}
				}
			}
		}
		return SensorTests[SensorTestIndexPlace];
	}
	return nullptr;
}

bool USensorBase::DestroySensorTest(const TSubclassOf<USensorTestBase> SensorTestClass, const int32 SensorTestIndexPlace)
{

#if WITH_EDITOR
	if (bEnable)
	{
		if (const auto Receiver = GetSenseReceiverComponent())
		{
			UE_LOG(
				LogSenseSys,
				Error,
				TEXT("Call USensorBase::DestroyNewSensorTest but Sensor is Enable=true, disable sensor before Destroy test, Sensor: %s, Receiver: %s, Actor: "
					 "%s"),
				*GetNameSafe(this),
				*GetNameSafe(Receiver),
				*GetNameSafe(Receiver->GetOwner()));
		}
	}
#endif //WITH_EDITOR

	const UWorld* World = GetWorld();
	const bool bPlayWorld = World && World->IsGameWorld() && World->HasBegunPlay() && !World->bIsTearingDown;
	if (!IsPendingKill() && SensorTestClass != nullptr && bPlayWorld && !bEnable && SensorTests.IsValidIndex(SensorTestIndexPlace))
	{
		USensorTestBase* STest = SensorTests[SensorTestIndexPlace];
		if (STest->IsA(SensorTestClass))
		{
			SensorTests.RemoveAt(SensorTestIndexPlace);
			STest->bEnableTest = false;
			STest->MarkPendingKill();
			if (SensorTests.Num() == 0)
			{}
		}
	}
	return false;
}

#if WITH_EDITOR

bool USensorBase::CheckSensorTestToDefaults(TArray<FSenseSysRestoreObject>& Rest)
{
	check(Rest.Num() == 0);

	UBlueprintGeneratedClass* BaseClass = USenseSystemBPLibrary::GetOwnerBlueprintClass(this);
	if (IsValid(BaseClass))
	{
		USenseReceiverComponent* DefaultComponent = nullptr;
		USensorBase* DefaultSensor = nullptr;

		const EOwnerBlueprintClassType BlueprintClassType = USenseSystemBPLibrary::GetOwnerBlueprintClassType(BaseClass);
		const FName RecName = GetOuter()->GetFName();

		if (const UBlueprint* Blueprint_Self = Cast<UBlueprint>(BaseClass->ClassGeneratedBy))
		{
			switch (BlueprintClassType)
			{
				case EOwnerBlueprintClassType::None: break;
				case EOwnerBlueprintClassType::ActorBP:
				{
					if (UClass* ParentClass = Cast<UClass>(Blueprint_Self->ParentClass))
					{
						DefaultComponent = USenseSystemBPLibrary::GetSenseReceiverComponent_FromDefaultActorClass(ParentClass, RecName);
					}
					if (!DefaultComponent) break;
				}
				case EOwnerBlueprintClassType::SenseReceiverComponentBP:
				{
					if (!DefaultComponent && BlueprintClassType == EOwnerBlueprintClassType::SenseReceiverComponentBP)
					{
						if (const UBlueprintGeneratedClass* ParentClass_0 = Cast<UBlueprintGeneratedClass>(Blueprint_Self->ParentClass))
						{
							DefaultComponent = Cast<USenseReceiverComponent>(ParentClass_0->ClassDefaultObject);
						}
						else if (const UClass* ParentClass_1 = Cast<UClass>(Blueprint_Self->ParentClass))
						{
							DefaultComponent = Cast<USenseReceiverComponent>(ParentClass_1->GetDefaultObject(true));
						}
					}
					if (!DefaultComponent) break;

					ESuccessState SuccessState;
					DefaultSensor = DefaultComponent->GetSensor_ByTag(SensorType, SensorTag, SuccessState);
					if (!DefaultSensor) break;
				}
				case EOwnerBlueprintClassType::SensorBaseBP:
				{
					if (!DefaultSensor && BlueprintClassType == EOwnerBlueprintClassType::SensorBaseBP)
					{
						if (const UBlueprintGeneratedClass* ParentClass_0 = Cast<UBlueprintGeneratedClass>(Blueprint_Self->ParentClass))
						{
							DefaultSensor = Cast<USensorBase>(ParentClass_0->ClassDefaultObject);
						}
						else if (const UClass* ParentClass_1 = Cast<UClass>(Blueprint_Self->ParentClass))
						{
							DefaultSensor = Cast<USensorBase>(ParentClass_1->GetDefaultObject(true));
						}
					}
					if (!DefaultSensor) break;

					TArray<USensorTestBase*>& TestCDO_Arr = DefaultSensor->SensorTests;
					for (int32 j = 0; j < TestCDO_Arr.Num(); ++j)
					{
						if (IsValid(TestCDO_Arr[j]))
						{
							UClass* ClassCDO = TestCDO_Arr[j]->GetClass();
							if (SensorTests.IsValidIndex(j) && IsValid(SensorTests[j]))
							{
								const UClass* Class1 = SensorTests[j]->GetClass();
								if (TestCDO_Arr[j]->GetFName() != SensorTests[j]->GetFName() || !Class1->IsChildOf(ClassCDO))
								{
									Rest.Add(FSenseSysRestoreObject(TestCDO_Arr[j], ClassCDO, TestCDO_Arr[j]->GetFName(), j, SensorType));
								}
								//else if (TestCDO_Arr[j]->GetFName() != SensorTests[j]->GetFName())//todo
								//{
								//	{
								//		Rest.Add(FSenseSysRestoreObject(nullptr, Class1, SensorCDO_Arr[j]->GetFName(), j, InSensor_Type));
								//		SensorTests[j]->MarkPendingKill();
								//		SensorTests[j] = nullptr;
								//	}
								//	{
								//		SensorTests[j]->Rename(
								//			*TestCDO_Arr[j]->GetFName().ToString(),
								//			this,
								//			REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
								//	}
								//}
							}
							else
							{
								if (!SensorTests.IsValidIndex(j))
								{
									SensorTests.Insert(nullptr, j);
								}
								Rest.Add(FSenseSysRestoreObject(TestCDO_Arr[j], ClassCDO, TestCDO_Arr[j]->GetFName(), j, SensorType));
							}
						}
					}
					break;
				}
			}
		}
	}
	return Rest.Num() > 0;
}


void USensorBase::RestoreSensorTestDefaults(TArray<FSenseSysRestoreObject>& Rest)
{
	const EObjectFlags Flags = GetMaskedFlags(RF_PropagateToSubObjects);
	for (const auto& It : Rest)
	{
		UObject* Obj = NewObject<UObject>(this, It.Class, It.ObjectName, Flags, It.DefaultObject);
		SensorTests[It.Idx] = Cast<USensorTestBase>(Obj);
	}
}

void USensorBase::CheckAndRestoreSensorTestDefaults()
{
	TArray<FSenseSysRestoreObject> Rest;
	if (CheckSensorTestToDefaults(Rest))
	{
		RestoreSensorTestDefaults(Rest);
		MarkPackageDirty_Internal();
	}
}

void USensorBase::MarkPackageDirty_Internal() const
{
	if (!HasAnyFlags(RF_Transient))
	{
		if (UPackage* Package = GetOutermost())
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

#endif //WITH_EDITOR


/*
bool USensorBase::RemoveFromSensingByHash(const uint32 Hash)
{
	const int32 CompIdx = HashSorted::Remove_HashType(Ignored_Components, Hash);
	if (CompIdx == INDEX_NONE)
	{
		bool bRemoved = false;
		for (auto& Ch : ChannelSetup)
		{
			bRemoved |= HashSorted::Remove_HashType(Ch.NewSensed, Hash, false) != INDEX_NONE;
			if (!bRemoved)
			{
				bRemoved |= HashSorted::Remove_HashType(Ch.LostAllSensed, Hash, false) != INDEX_NONE;
			}
			if (!bRemoved)
			{
				bRemoved |= HashSorted::Remove_HashType(Ch.LostCurrentSensed, Hash, false) != INDEX_NONE;
			}
			if (!bRemoved)
			{
				bRemoved |= HashSorted::Remove_HashType(Ch.ForgetSensed, Hash, false) != INDEX_NONE;
			}
			if (!bRemoved)
			{
				const int32 ID = HashSorted::Remove_HashType(Ch.CurrentSensed, Hash, false);
				bRemoved |= ID != INDEX_NONE;
				if (ID != INDEX_NONE && Ch.BestSensedID_ByScore.Num())
				{
					Ch.BestSensedID_ByScore.RemoveSingle(ID);

					const int32 RemIdx = Ch.BestSensedID_ByScore.IndexOfByKey(ID);
					if (RemIdx != INDEX_NONE)
					{
						Ch.BestSensedID_ByScore.RemoveAt(RemIdx);
						for (auto& It : Ch.BestSensedID_ByScore)
						{
							if (It > ID)
							{
								--It;
							}
						}
					}
				}
			}
		}
		return bRemoved;
	}
	return false;
}
*/