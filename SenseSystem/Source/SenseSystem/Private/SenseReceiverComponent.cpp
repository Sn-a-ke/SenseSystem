//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "SenseReceiverComponent.h"
#include "HAL/Platform.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PawnMovementComponent.h"

#include "Sensors/SensorBase.h"
#include "SenseManager.h"
#include "SenseStimulusComponent.h"
#include "SensedStimulStruct.h"
#include "HashSorted.h"
#include "SenseSystemBPLibrary.h"


#if WITH_EDITORONLY_DATA
	#include "Engine/Engine.h"
	//#include "DrawDebugHelpers.h"
	#include "CanvasTypes.h"
	#include "SceneManagement.h"
	#include "SceneView.h"
	#include "UnrealClient.h"
#endif


USenseReceiverComponent::USenseReceiverComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bTickEvenWhenPaused = false;
	PrimaryComponentTick.bCanEverTick = 1;
	PrimaryComponentTick.bStartWithTickEnabled = 0;
	PrimaryComponentTick.TickInterval = EFPSTime_SenseSys::fps60;
	UActorComponent::SetComponentTickEnabled(false);

	{
		const auto Pawn = GetTypedOuter<class APawn>();
		if (Pawn)
		{
			const auto MovementComponent = Pawn->GetMovementComponent();
			if (MovementComponent)
			{
				UActorComponent::AddTickPrerequisiteComponent(MovementComponent);
			}
		}
	}
}

USenseReceiverComponent::~USenseReceiverComponent()
{}

//void USenseReceiverComponent::PostInitProperties()
//{
//	Super::PostInitProperties();
//#if WITH_EDITOR
//	if (GetOwner() && GetWorld() && !GetWorld()->IsGameWorld())
//	{
//		CheckAndRestoreSensorTestDefaults(ESensorType::Active);
//		CheckAndRestoreSensorTestDefaults(ESensorType::Passive);
//		CheckAndRestoreSensorTestDefaults(ESensorType::Manual);
//	}
//#endif
//}


bool USenseReceiverComponent::RegisterSelfSense()
{
	if (GetOwner() && GetWorld() && GetWorld()->IsGameWorld())
	{
		SetComponentTickEnabled(false);
		PrivateSenseManager = USenseManager::GetSenseManager(this);
		if (GetSenseManager() != nullptr)
		{
			AddToRoot();
			for (uint8 i = 1; i < 4; i++)
			{
				const auto& SensorsArr = GetSensorsByType(static_cast<ESensorType>(i));
				//ArrayHelpers::Filter_Sorted_V2(SensorsArr, [](const USensorBase* In) { return IsValid(In); }); //todo:USenseReceiverComponent::RegisterSelfSense : remove invalid sensors?

				for (USensorBase* const It : SensorsArr)
				{
					if (It)
					{
						It->SensorType = static_cast<ESensorType>(i);
						It->InitializeForSense(this);
						if (It->SensorThreadType == ESensorThreadType::Sense_Thread || It->SensorThreadType == ESensorThreadType::Sense_Thread_HighPriority)
						{
							bContainsSenseThreadSensors = true;
						}
					}
				}
			}
			const bool bSuccess = GetSenseManager()->RegisterSenseReceiver(this);
			SetEnableSenseReceiver(bEnableSenseReceiver && bSuccess);

			return bSuccess;
		}
		else
		{
			UE_LOG(LogSenseSys, Error, TEXT("Register Self For Sense: %s , SenseManager - NOTVALID!!! "), *GetNameSafe(this));
		}
	}
	else
	{
		UE_LOG(LogSenseSys, Error, TEXT("Register Self For Sense: %s , FAIL!!! "), *GetNameSafe(this));
	}
	return false;
}

bool USenseReceiverComponent::UnRegisterSelfSense()
{
	bool bSuccessUnReg = false;
	const UWorld* World = GetWorld();
	const bool bPlayWorld = World && World->IsGameWorld() && World->HasBegunPlay() && !World->bIsTearingDown;
	SetComponentTickEnabled(false);
	if (bPlayWorld && GetSenseManager())
	{
		bSuccessUnReg = GetSenseManager()->UnRegisterSenseReceiver(this);
	}

	for (uint8 i = 1; i < 4; i++)
	{
		const TArray<USensorBase*>& SensorsArr = GetSensorsByType(static_cast<ESensorType>(i));
		for (USensorBase* const It : SensorsArr)
		{
			if (It)
			{
				It->ResetInitialization();
				if (bPlayWorld)
				{
					It->ForgetAllSensed();
				}
			}
		}
	}
	PrivateSenseManager = nullptr;
	RemoveFromRoot();
	return bSuccessUnReg;
}

void USenseReceiverComponent::OnRegister()
{
	Super::OnRegister();

	if (IsValid(this))
	{
		for (uint8 i = 1; i < 4; i++)
		{
			const TArray<USensorBase*>& SensorsArr = GetSensorsByType(static_cast<ESensorType>(i));
			for (USensorBase* const It : SensorsArr)
			{
				if (IsValid(It))
				{
					//need sensor transform for drawing
					//initialize all, except SenseManager
					It->SensorType = static_cast<ESensorType>(i);
					It->InitializeFromReceiver(this);
				}
			}
		}

		if (GetOwner() && GetWorld() && GetWorld()->IsGameWorld() && HasBegunPlay()) //for in editor(F8) change actor variable in runtime
		{
			RegisterSelfSense();
		}
	}
}

void USenseReceiverComponent::OnUnregister()
{
	Super::OnUnregister();
	UnRegisterSelfSense();
}

void USenseReceiverComponent::Cleanup()
{
	UnRegisterSelfSense();

	ClearTrackTargets();
	OnMainTargetStatusChanged.Clear();

	for (uint8 i = 1; i < 4; i++)
	{
		for (USensorBase* It : GetSensorsByType(static_cast<ESensorType>(i)))
		{
			if (It)
			{
				It->Cleanup();
			}
		}
	}

	PrivateSenseManager = nullptr;

	//! not Cleanup with registration events, failed serialization
	ActiveSensors.Empty();
	PassiveSensors.Empty();
	ManualSensors.Empty();

	OnForgetSense.Clear();

	OnCurrentSense.Clear();
	OnNewSense.Clear();
	OnLostSense.Clear();
	OnForgetSense.Clear();
	RemoveFromRoot();
}

void USenseReceiverComponent::TickComponent(const float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	checkSlow(bEnableSenseReceiver);
	if (const auto SM = GetSenseManager())
	{
		for (uint8 i = 1; i < 4; i++) //iterate all
		{
			const TArray<USensorBase*>& SensorsArr = GetSensorsByType(static_cast<ESensorType>(i));
			for (USensorBase* const It : SensorsArr)
			{
				if (LIKELY(It))
				{
					if (SM->IsHaveStimulusTag(It->SensorTag))
					{
						It->TickSensor(DeltaTime);
					}
				}
			}
		}
	}
}

void USenseReceiverComponent::BeginPlay()
{
	Super::BeginPlay();
	RegisterSelfSense();
}

void USenseReceiverComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Cleanup();
	Super::EndPlay(EndPlayReason);
}

void USenseReceiverComponent::BeginDestroy()
{
	//Cleanup();
	Super::BeginDestroy();
}

void USenseReceiverComponent::DestroyComponent(const bool bPromoteChildren)
{
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		Cleanup();
	}
	Super::DestroyComponent(bPromoteChildren);
}

/************************************/

void USenseReceiverComponent::OnUnregisterStimulus(UObject* Obj)
{
	USenseStimulusBase* Comp = Cast<USenseStimulusBase>(Obj);
	RemoveTrackTargetComponent(Comp);
	if (IsValid(Comp))
	{
		for (uint8 i = 1; i < 4; i++)
		{
			for (USensorBase* const It : GetSensorsByType(static_cast<ESensorType>(i)))
			{
				if (It)
				{
					TArray<FStimulusFindResult> FindResult = It->UnRegisterSenseStimulus(Comp);
					for (const auto& ResIt : FindResult)
					{
						if (OnUnregisterSense.IsBound())
						{
							const TArray<FSensedStimulus> Arr = {ResIt.SensedData};
							OnUnregisterSense.Broadcast(It, ResIt.Channel, Arr);
						}
					}
				}
			}
		}
	}
}


FOnSensorUpdateDelegate& USenseReceiverComponent::GetDelegateBySenseEvent(const EOnSenseEvent SenseEvent)
{
	switch (SenseEvent)
	{
		case EOnSenseEvent::SenseNew: return OnNewSense;
		case EOnSenseEvent::SenseCurrent: return OnCurrentSense;
		case EOnSenseEvent::SenseLost: return OnLostSense;
		case EOnSenseEvent::SenseForget: return OnForgetSense;
		default:
		{
			checkNoEntry();
			return OnNewSense;
		}
	}
}

const FOnSensorUpdateDelegate& USenseReceiverComponent::GetDelegateBySenseEvent(EOnSenseEvent SenseEvent) const
{
	switch (SenseEvent)
	{
		case EOnSenseEvent::SenseNew: return OnNewSense;
		case EOnSenseEvent::SenseCurrent: return OnCurrentSense;
		case EOnSenseEvent::SenseLost: return OnLostSense;
		case EOnSenseEvent::SenseForget: return OnForgetSense;
		default:
		{
			checkNoEntry();
			return OnNewSense;
		}
	}
}

/************************************/

const TArray<USenseStimulusBase*>& USenseReceiverComponent::GetTrackTargetComponents() const
{
	return TrackTargetComponents;
}

TArray<AActor*> USenseReceiverComponent::GetTrackTargetActors() const
{
	TArray<AActor*> Out;
	Out.Reserve(TrackTargetComponents.Num());
	for (const USenseStimulusBase* It : TrackTargetComponents)
	{
		if (LIKELY(IsValid(It) && IsValid(It->GetOwner())))
		{
			Out.Add(It->GetOwner());
		}
	}
	return Out;
}

void USenseReceiverComponent::AddTrackTargetActor(AActor* Actor)
{
	if (IsValid(Actor))
	{
		USenseStimulusBase* Comp = USenseSystemBPLibrary::GetStimulusFromActor(Actor);
		AddTrackTargetComponent(Comp);
	}
}

void USenseReceiverComponent::RemoveTrackTargetActor(AActor* Actor)
{
	if (Actor)
	{
		USenseStimulusBase* Comp = USenseSystemBPLibrary::GetStimulusFromActor(Actor);
		RemoveTrackTargetComponent(Comp);
	}
}

void USenseReceiverComponent::AddTrackTargetComponent(USenseStimulusBase* Comp)
{
	if (IsValid(Comp))
	{
		HashSorted::InsertUnique(TrackTargetComponents, Comp);
	}
}

void USenseReceiverComponent::RemoveTrackTargetComponent(USenseStimulusBase* Comp)
{
	if (IsValid(Comp) && IsValid(Comp->GetOwner()))
	{
		const int32 MainTargetID = HashSorted::BinarySearch_HashType(TrackTargetComponents, GetTypeHash(Comp));
		if (MainTargetID != INDEX_NONE)
		{
			const AActor* SelfOwner = GetOwner();
			if (Comp->IsSensedFrom(SelfOwner, NAME_None))
			{
				OnMainTargetStatusChanged.Broadcast(Comp->GetOwner(), FSensedStimulus(), EOnSenseEvent::SenseLost);
				OnMainTargetStatusChanged.Broadcast(Comp->GetOwner(), FSensedStimulus(), EOnSenseEvent::SenseForget);
			}
			if (Comp->IsRememberFrom(SelfOwner, NAME_None))
			{
				OnMainTargetStatusChanged.Broadcast(Comp->GetOwner(), FSensedStimulus(), EOnSenseEvent::SenseForget);
			}
			TrackTargetComponents.RemoveAt(MainTargetID);
		}
	}
}

/************************************/

void USenseReceiverComponent::TrackTargets(const TArray<FSensedStimulus>& InSensedStimulus, const EOnSenseEvent SenseEvent) const
{
	for (const USenseStimulusBase* It : TrackTargetComponents)
	{
		if (IsValid(It))
		{
			const int32 MainTargetID = HashSorted::BinarySearch_HashType(InSensedStimulus, GetTypeHash(It));
			if (MainTargetID != INDEX_NONE)
			{
				check(It->GetOwner());
				OnMainTargetStatusChanged.Broadcast(It->GetOwner(), InSensedStimulus[MainTargetID], SenseEvent);
			}
		}
	}
}

/************************************/

const TArray<TObjectPtr<USensorBase>>& USenseReceiverComponent::GetSensorsByType(const ESensorType Sensor_Type) const
{
	check(Sensor_Type != ESensorType::None);
	switch (Sensor_Type)
	{
		case ESensorType::Active: return TypeArr(ActiveSensors);
		case ESensorType::Passive: return TypeArr(PassiveSensors);
		case ESensorType::Manual: return TypeArr(ManualSensors);
		default: break;
	}
	checkNoEntry();
	UE_ASSUME(0);
	return TypeArr(ActiveSensors);
}

TArray<TObjectPtr<USensorBase>>& USenseReceiverComponent::GetSensorsByType(ESensorType Sensor_Type)
{
	check(Sensor_Type != ESensorType::None);
	switch (Sensor_Type)
	{
		case ESensorType::Active: return TypeArr(ActiveSensors);
		case ESensorType::Passive: return TypeArr(PassiveSensors);
		case ESensorType::Manual: return TypeArr(ManualSensors);
		default: break;
	}
	checkNoEntry();
	UE_ASSUME(0);
	return TypeArr(ActiveSensors);
}

TArray<USensorBase*> USenseReceiverComponent::GetSensorsByType_BP(const ESensorType Sensor_Type) const
{
	TArray<USensorBase*> Out;
	if (Sensor_Type != ESensorType::None)
	{
		const auto& Arr = GetSensorsByType(Sensor_Type);
		Out.Reserve(Arr.Num());
		for (auto It : Arr)
		{
			Out.Add(It);
		}
	}
	return Out;
}

USensorBase* USenseReceiverComponent::GetSensor_ByClass(TSubclassOf<USensorBase> SensorClass, ESuccessState& SuccessState) const
{
	USensorBase* Ptr = nullptr;
	for (uint8 i = 1; i < 4; i++)
	{
		const auto& SensorsArr = GetSensorsByType(static_cast<ESensorType>(i));
		const int32 ID = SensorsArr.IndexOfByPredicate(FObjectByClassPredicate(SensorClass));
		if (ID != INDEX_NONE)
		{
			Ptr = SensorsArr[ID];
			break;
		}
	}
	if (IsValid(Ptr))
	{
		SuccessState = ESuccessState::Success;
	}
	else
	{
		SuccessState = ESuccessState::Failed;
	}
	return Ptr;
}

USensorBase* USenseReceiverComponent::GetActiveSensor_ByClass(const TSubclassOf<UActiveSensor> SensorClass, ESuccessState& SuccessState) const
{
	USensorBase* Ptr = FindInArray_ByClass(ActiveSensors, SensorClass);
	if (IsValid(Ptr))
	{
		SuccessState = ESuccessState::Success;
	}
	else
	{
		SuccessState = ESuccessState::Failed;
	}
	return Ptr;
}

USensorBase* USenseReceiverComponent::GetPassiveSensor_ByClass(const TSubclassOf<UPassiveSensor> SensorClass, ESuccessState& SuccessState) const
{
	USensorBase* Ptr = FindInArray_ByClass(PassiveSensors, SensorClass);
	if (IsValid(Ptr))
	{
		SuccessState = ESuccessState::Success;
	}
	else
	{
		SuccessState = ESuccessState::Failed;
	}
	return Ptr;
}

USensorBase* USenseReceiverComponent::GetManualSensor_ByClass(const TSubclassOf<UActiveSensor> SensorClass, ESuccessState& SuccessState) const
{
	USensorBase* Ptr = FindInArray_ByClass(ManualSensors, SensorClass);
	if (IsValid(Ptr))
	{
		SuccessState = ESuccessState::Success;
	}
	else
	{
		SuccessState = ESuccessState::Failed;
	}
	return Ptr;
}

UActiveSensor* USenseReceiverComponent::GetActiveSensor_ByTag(const FName Tag, ESuccessState& SuccessState) const
{
	const auto FindElem = FindInArray_ByTag(ActiveSensors, Tag);
	UActiveSensor* Ptr = Cast<UActiveSensor>(FindElem);
	if (IsValid(Ptr))
	{
		SuccessState = ESuccessState::Success;
	}
	else
	{
		SuccessState = ESuccessState::Failed;
	}
	return Ptr;
}

USensorBase* USenseReceiverComponent::GetPassiveSensor_ByTag(const FName Tag, ESuccessState& SuccessState) const
{
	USensorBase* Ptr = FindInArray_ByTag(PassiveSensors, Tag);
	if (IsValid(Ptr))
	{
		SuccessState = ESuccessState::Success;
	}
	else
	{
		SuccessState = ESuccessState::Failed;
	}
	return Ptr;
}

UActiveSensor* USenseReceiverComponent::GetManualSensor_ByTag(const FName Tag, ESuccessState& SuccessState) const
{
	const auto FindElem = FindInArray_ByTag(ManualSensors, Tag);
	UActiveSensor* Ptr = Cast<UActiveSensor>(FindElem);
	if (IsValid(Ptr))
	{
		SuccessState = ESuccessState::Success;
	}
	else
	{
		SuccessState = ESuccessState::Failed;
	}
	return Ptr;
}

USensorBase* USenseReceiverComponent::GetSensor_ByTag(const ESensorType Sensor_Type, const FName Tag, ESuccessState& SuccessState) const
{
	USensorBase* Out = nullptr;
	switch (Sensor_Type)
	{
		case ESensorType::None:
		{
			SuccessState = ESuccessState::Failed;
			break;
		}
		case ESensorType::Active:
		{
			Out = GetActiveSensor_ByTag(Tag, SuccessState);
			break;
		}
		case ESensorType::Passive:
		{
			Out = GetPassiveSensor_ByTag(Tag, SuccessState);
			break;
		}
		case ESensorType::Manual:
		{
			Out = GetManualSensor_ByTag(Tag, SuccessState);
			break;
		}
		default:
		{
			SuccessState = ESuccessState::Failed;
			break;
		}
	}
	return Out;
}

USensorBase* USenseReceiverComponent::GetSensor_ByTagAndClass(
	const ESensorType Sensor_Type,
	const FName Tag,
	const TSubclassOf<USensorBase> SensorClass,
	ESuccessState& SuccessState) const
{
	const auto FindElem = GetSensor_ByTag(Sensor_Type, Tag, SuccessState);
	if (FindElem && FindElem->IsA(SensorClass))
	{
		SuccessState = ESuccessState::Success;
		return FindElem;
	}
	SuccessState = ESuccessState::Failed;
	return nullptr;
}

void USenseReceiverComponent::FindSensedActor(
	const AActor* Actor,
	const ESensorType Sensor_Type,
	const FName SensorTag,
	const ESensorArrayByType SenseEventType,
	FSensedStimulus& Out,
	ESuccessState& SuccessState) const
{
	if (IsValid(Actor))
	{
		const auto SensorPtr = GetSensor_ByTag(Sensor_Type, SensorTag, SuccessState);
		if (SensorPtr && SuccessState == ESuccessState::Success)
		{
			for (const auto& Ch : SensorPtr->ChannelSetup)
			{
				const auto& SSArray = Ch.GetSensedStimulusBySenseEvent(SenseEventType);
				const int32 ID = SensorPtr->FindInSortedArray(SSArray, Actor);
				if (ID != INDEX_NONE)
				{
					Out = SSArray[ID];
					SuccessState = ESuccessState::Success;
					return;
				}
			}
		}
	}
	SuccessState = ESuccessState::Failed;
}

void USenseReceiverComponent::FindActorInAllSensors(
	const AActor* Actor,
	ESensorType& Sensor_Type,
	FName& SensorTag,
	ESensorArrayByType& SenseEventType,
	FSensedStimulus& Out,
	ESuccessState& SuccessState) const
{
	if (IsValid(Actor))
	{
		const USenseStimulusBase* SensedStimulus = USenseSystemBPLibrary::GetStimulusFromActor(Actor);
		if (IsValid(SensedStimulus))
		{
			for (uint8 i = 1; i < 4; i++)
			{
				const auto& SensorsArr = GetSensorsByType(static_cast<ESensorType>(i));
				for (const USensorBase* It : SensorsArr)
				{
					if (It)
					{
						for (uint8 j = 0; j < 5; j++)
						{
							const auto SenseEvent = static_cast<ESensorArrayByType>(j);
							for (const auto& Ch : It->ChannelSetup)
							{
								const auto& SSArray = Ch.GetSensedStimulusBySenseEvent(SenseEventType);
								const int32 ID = It->FindInSortedArray(SSArray, SensedStimulus);
								if (ID != INDEX_NONE)
								{
									Out = SSArray[ID];
									Sensor_Type = static_cast<ESensorType>(i);
									SensorTag = It->SensorTag;
									SenseEventType = SenseEvent;
									SuccessState = ESuccessState::Success;
									return;
								}
							}
						}
					}
				}
			}
		}
	}
	SuccessState = ESuccessState::Failed;
}


TArray<FStimulusFindResult> USenseReceiverComponent::FindStimulusInSensor(
	const USenseStimulusBase* StimulusComponent,
	const ESensorType Sensor_Type,
	const FName SensorTag,
	ESuccessState& SuccessState) const
{
	TArray<FStimulusFindResult> StimulusFindResult;
	if (IsValid(StimulusComponent))
	{
		ESuccessState Success;
		if (USensorBase* const Sens = GetSensor_ByTag(Sensor_Type, SensorTag, Success))
		{
			StimulusFindResult.Append(Sens->FindStimulusInAllState(StimulusComponent, FBitFlag64_SenseSys(MAX_uint64)));
		}
	}
	SuccessState = StimulusFindResult.Num() > 0 ? ESuccessState::Success : ESuccessState::Failed;
	return StimulusFindResult;
}

TArray<FStimulusFindResult> USenseReceiverComponent::FindActorInSensor(
	const AActor* Actor,
	const ESensorType Sensor_Type,
	const FName SensorTag,
	ESuccessState& SuccessState) const
{
	return FindStimulusInSensor(USenseSystemBPLibrary::GetStimulusFromActor(Actor), Sensor_Type, SensorTag, SuccessState);
}


/************************************/

FQuat USenseReceiverComponent::GetOwnerControllerRotation() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (IsValid(Pawn))
	{
		const AController* Con = Pawn->GetController();
		if (IsValid(Con))
		{
			return FQuat(Con->GetControlRotation());
		}
	}
	return GetComponentTransform().GetRotation();
}

void USenseReceiverComponent::SetEnableSenseReceiver(const bool bEnable)
{
	bEnableSenseReceiver = bEnable;
	SetEnableAllSensors(bEnableSenseReceiver);
}

void USenseReceiverComponent::SetEnableAllSensors(const bool bEnable, const bool bForget)
{
	for (uint8 i = 1; i < 4; i++)
	{
		for (USensorBase* const It : GetSensorsByType(static_cast<ESensorType>(i)))
		{
			if (It && It->bEnable != bEnable)
			{
				It->SetEnableSensor(bEnable);
				if (!bEnable && bForget)
				{
					It->ForgetAllSensed();
				}
			}
		}
	}
	SetComponentTickEnabled(bEnable);
}

USenseManager* USenseReceiverComponent::GetSenseManager() const
{
	return PrivateSenseManager;
}

/************************************/

void USenseReceiverComponent::BindOnReportStimulusEvent(const uint16 StimulusID, const FName SensorTag)
{
	if (bEnableSenseReceiver && SensorTag != NAME_None && StimulusID != MAX_uint16 && IsValid(this))
	{
		for (UPassiveSensor* const It : PassiveSensors)
		{
			if (IsValid(It) && It->SensorTag == SensorTag)
			{
				It->ReportPassiveEvent(StimulusID);
			}
		}
	}
}

#if WITH_EDITOR
void USenseReceiverComponent::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
	for (uint8 i = 1; i < 4; i++)
	{
		const auto& SensorsArr = GetSensorsByType(static_cast<ESensorType>(i));
		for (USensorBase* const It : SensorsArr)
		{
			if (It)
			{
				if (It->SensorType != static_cast<ESensorType>(i))
				{
					It->SensorType = static_cast<ESensorType>(i);
				}
			}
		}
	}
	const FName PropertyName = (e.Property != nullptr) ? e.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(USenseReceiverComponent, ActiveSensors))
	{
		CheckAndRestoreSensorTestDefaults(ESensorType::Active);
	}
	if (PropertyName == GET_MEMBER_NAME_CHECKED(USenseReceiverComponent, PassiveSensors))
	{
		CheckAndRestoreSensorTestDefaults(ESensorType::Passive);
	}
	if (PropertyName == GET_MEMBER_NAME_CHECKED(USenseReceiverComponent, ManualSensors))
	{
		CheckAndRestoreSensorTestDefaults(ESensorType::Manual);
	}
}

EDataValidationResult USenseReceiverComponent::IsDataValid(TArray<FText>& ValidationErrors)
{
	EDataValidationResult IsValid = Super::IsDataValid(ValidationErrors);
	for (uint8 i = 1; i < 4; i++)
	{
		const auto& SensorsArr = GetSensorsByType(static_cast<ESensorType>(i));
		for (USensorBase* const It : SensorsArr)
		{
			if (It)
			{
				const EDataValidationResult IsSensorValid = It->IsDataValid(ValidationErrors);
				IsValid = CombineDataValidationResults(IsValid, IsSensorValid);
			}
		}
	}
	return IsValid;
}
#endif

#if WITH_EDITORONLY_DATA

void USenseReceiverComponent::DrawComponent(const FSceneView* View, FPrimitiveDrawInterface* PDI) const
{
	const UWorld* World = GetWorld();
	if (!bEnableSenseReceiver || !IsRegistered() || !World) return;

	for (uint8 i = 1; i < 4; i++)
	{
		const auto& SensorsArr = GetSensorsByType(static_cast<ESensorType>(i));
		for (const USensorBase* It : SensorsArr)
		{
			if (It && It->bEnable)
			{
				FSenseSysDebugDraw _DebugDraw;
				if (GetSenseManager() && World->IsGameWorld())
				{
					_DebugDraw = GetSenseManager()->GetSenseSysDebugDraw(It->SensorTag);
				}
				else
				{
					_DebugDraw = FSenseSysDebugDraw(false);
					_DebugDraw.Sensor_DebugTest = true;
				}
				if (_DebugDraw.Sensor_DebugTest || _DebugDraw.Sensor_DebugCurrentSensed || _DebugDraw.Sensor_DebugLostSensed ||
					_DebugDraw.Sensor_DebugBestSensed)
				{
					It->DrawSensor(View, PDI);
				}
			}
		}
	}
}

void USenseReceiverComponent::DrawComponentHUD(const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) const
{
	const UWorld* World = GetWorld();
	if (!World || !bEnableSenseReceiver || !IsRegistered()) return;

	FString OutStr;
	for (uint8 i = 1; i < 4; i++)
	{
		const auto& SensorsArr = GetSensorsByType(static_cast<ESensorType>(i));
		for (const USensorBase* It : SensorsArr)
		{
			if (It && It->bEnable)
			{
				FSenseSysDebugDraw _DebugDraw;
				if (GetSenseManager() && World->IsGameWorld())
				{
					_DebugDraw = GetSenseManager()->GetSenseSysDebugDraw(It->SensorTag);
				}
				else
				{
					_DebugDraw = FSenseSysDebugDraw(false);
					_DebugDraw.Sensor_DebugTest = true;
				}

				if (_DebugDraw.Sensor_DebugTest || _DebugDraw.Sensor_DebugCurrentSensed || _DebugDraw.Sensor_DebugLostSensed ||
					_DebugDraw.Sensor_DebugBestSensed)
				{
					It->DrawSensorHUD(Viewport, View, Canvas);
					if (_DebugDraw.Sensor_DebugTest)
					{
						OutStr.Append(FString::Printf(TEXT("%s \n"), *It->SensorTag.ToString()));
					}
				}
			}
		}
	}

	if (OutStr.Len() > 0)
	{
		const FVector Loc = GetComponentTransform().GetLocation();
		const FText Text = FText::FromString(OutStr);
		FVector2D PixLoc;
		View->WorldToPixel(Loc, PixLoc);
		Canvas->DrawShadowedText(PixLoc.X, PixLoc.Y, Text, GEngine->GetLargeFont(), FLinearColor::Yellow, FLinearColor(0, 0, 0, 0));
	}
}

#endif //WITH_EDITORONLY_DATA

USensorBase* USenseReceiverComponent::CreateNewSensor(
	const TSubclassOf<USensorBase> SensorClass,
	const ESensorType Sensor_Type,
	const FName Tag,
	const ESensorThreadType ThreadType,
	const bool bEnableNewSensor,
	ESuccessState& SuccessState)
{
	const UWorld* World = GetWorld();
	const bool bPlayWorld = World && World->IsGameWorld() && World->HasBegunPlay() && !World->bIsTearingDown;
	if (IsValid(this) && bPlayWorld && SensorClass != nullptr && Sensor_Type != ESensorType::None && Tag != NAME_None)
	{
		TArray<TObjectPtr<USensorBase>>& TargetArr = GetSensorsByType(Sensor_Type);
		if (FindInArray_ByTag(TargetArr, Tag) == nullptr)
		{
			switch (Sensor_Type)
			{
				case ESensorType::None:
				{
#if WITH_EDITOR
					UE_LOG(
						LogSenseSys,
						Error,
						TEXT("USenseReceiverComponent::CreateNewSensor SensorType = None, Actor: %s, SenseReceiverComponent: %s, CreateNewSensorClass: %s"),
						*SensorClass->GetName(),
						*GetNameSafe(GetOwner()),
						*GetNameSafe(this))
#endif
					return nullptr;
				}
				case ESensorType::Active:
				{
					if (!SensorClass->IsChildOf<UActiveSensor>())
					{
#if WITH_EDITOR
						UE_LOG(
							LogSenseSys,
							Error,
							TEXT("USenseReceiverComponent::CreateNewSensor SensorType = Active - only for UActiveSensor subclass, Actor: %s, "
								 "SenseReceiverComponent: %s, CreateNewSensorClass: %s"),
							*SensorClass->GetName(),
							*GetNameSafe(GetOwner()),
							*GetNameSafe(this))

#endif
						return nullptr;
					}
					break;
				}
				case ESensorType::Passive:
				{
					if (!SensorClass->IsChildOf<UPassiveSensor>())
					{
#if WITH_EDITOR
						UE_LOG(
							LogSenseSys,
							Error,
							TEXT("USenseReceiverComponent::CreateNewSensor SensorType = Passive - only for UPassiveSensor subclass, Actor: %s, "
								 "SenseReceiverComponent: %s, CreateNewSensorClass: %s"),
							*SensorClass->GetName(),
							*GetNameSafe(GetOwner()),
							*GetNameSafe(this))

#endif
						return nullptr;
					}
					break;
				}
				case ESensorType::Manual:
				{
					if (!SensorClass->IsChildOf<UActiveSensor>())
					{
#if WITH_EDITOR
						UE_LOG(
							LogSenseSys,
							Error,
							TEXT("USenseReceiverComponent::CreateNewSensor SensorType = Manual - only for UActiveSensor subclass, Actor: %s, "
								 "SenseReceiverComponent: %s, CreateNewSensorClass: %s"),
							*SensorClass->GetName(),
							*GetNameSafe(GetOwner()),
							*GetNameSafe(this))

#endif
						return nullptr;
					}
					break;
				}
				default:
				{
					checkNoEntry();
					return nullptr;
				}
			}

			USensorBase* const NewSensor = NewObject<USensorBase>(this, SensorClass, Tag);
			NewSensor->bEnable = false;
			NewSensor->SensorTag = Tag;
			NewSensor->SensorType = Sensor_Type;
			NewSensor->SensorThreadType = ThreadType;
			TargetArr.Add(NewSensor);

			NewSensor->InitializeFromReceiver(this);
			NewSensor->InitializeForSense(this);

			const bool bOldContainsThreadCount = bContainsSenseThreadSensors;
			bContainsSenseThreadSensors |= ThreadType == ESensorThreadType::Sense_Thread || ThreadType == ESensorThreadType::Sense_Thread_HighPriority;
			checkSlow(GetSenseManager());
			GetSenseManager()->Add_ReceiverSensor(this, NewSensor, !bOldContainsThreadCount && bContainsSenseThreadSensors);

			if (bEnableNewSensor)
			{
				NewSensor->SetEnableSensor(true);
			}
			SuccessState = ESuccessState::Success;
			return TargetArr.Last();
		}
	}
	SuccessState = ESuccessState::Failed;
	return nullptr;
}

bool USenseReceiverComponent::DestroySensor(const ESensorType Sensor_Type, const FName Tag)
{
	const UWorld* World = GetWorld();
	const bool bPlayWorld = World && World->IsGameWorld() && World->HasBegunPlay() && !World->bIsTearingDown;
	if (IsValid(this) && bPlayWorld && Sensor_Type != ESensorType::None && Tag != NAME_None)
	{
		TArray<TObjectPtr<USensorBase>>& TargetArr = GetSensorsByType(Sensor_Type);
		if (USensorBase* Sen = FindInArray_ByTag(TargetArr, Tag))
		{
			Sen->SetEnableSensor(false);
			Sen->UpdateState = ESensorState::Uninitialized;
			Sen->ForgetAllSensed();
			Sen->SensorThreadType = ESensorThreadType::Main_Thread;
			if (const auto SM = GetSenseManager())
			{
				const bool bOldContainsThreadCount = bContainsSenseThreadSensors;
				UpdateContainsSenseThreadSensors();
				SM->Remove_ReceiverSensor(this, Sen, bOldContainsThreadCount && !bContainsSenseThreadSensors);
			}

			TargetArr.Remove(Sen);
			Sen->Cleanup();
			return true;
		}
	}
	//{ //todo
	//	bool ValidSensor = false;
	//	for (uint8 i = 1; i < 4 && !ValidSensor; i++)
	//	{
	//		const auto& SensorsArr = GetSensorsByType(static_cast<ESensorType>(i));
	//		for (const USensorBase* It : SensorsArr)
	//		{
	//			if (It)
	//			{
	//				ValidSensor = true;
	//				break;
	//			}
	//		}
	//	}
	//	if(!ValidSensor)
	//	{
	//		UnRegisterSelfSense();
	//	}
	//}

	return false;
}

#if WITH_EDITOR

bool USenseReceiverComponent::CheckSensorTestToDefaults(TArray<FSenseSysRestoreObject>& Rest, ESensorType InSensor_Type)
{
	if (InSensor_Type != ESensorType::None)
	{
		check(Rest.Num() == 0);

		UBlueprintGeneratedClass* BaseClass = USenseSystemBPLibrary::GetOwnerBlueprintClass(this);
		if (IsValid(BaseClass))
		{
			USenseReceiverComponent* DefaultComponent = nullptr;
			USensorBase* DefaultSensor = nullptr;

			const EOwnerBlueprintClassType BlueprintClassType = USenseSystemBPLibrary::GetOwnerBlueprintClassType(BaseClass);
			const FName RecName = GetFName();

			if (UBlueprint* Blueprint_Self = Cast<UBlueprint>(BaseClass->ClassGeneratedBy))
			{
				switch (BlueprintClassType)
				{
					case EOwnerBlueprintClassType::None: break;
					case EOwnerBlueprintClassType::ActorBP:
					{
						if (UBlueprintGeneratedClass* ParentClass_0 = Cast<UBlueprintGeneratedClass>(Blueprint_Self->ParentClass))
						{
							DefaultComponent = USenseSystemBPLibrary::GetSenseReceiverComponent_FromDefaultActorClass(ParentClass_0, RecName);
						}
						if (!DefaultComponent) break;
					}
					case EOwnerBlueprintClassType::SenseReceiverComponentBP:
					{
						if (!DefaultComponent && BlueprintClassType == EOwnerBlueprintClassType::SenseReceiverComponentBP)
						{
							if (UBlueprintGeneratedClass* ParentClass_0 = Cast<UBlueprintGeneratedClass>(Blueprint_Self->ParentClass))
							{
								DefaultComponent = Cast<USenseReceiverComponent>(ParentClass_0->ClassDefaultObject);
							}
							else if (UClass* ParentClass_1 = Cast<UClass>(Blueprint_Self->ParentClass))
							{
								DefaultComponent = Cast<USenseReceiverComponent>(ParentClass_1->GetDefaultObject(true));
							}
						}
						if (!DefaultComponent)
						{
							break;
						}

						//if (InSensor_Type != ESensorType::None)
						{
							TArray<USensorBase*>& SensorsArr = GetSensorsByType(InSensor_Type);
							TArray<USensorBase*>& SensorCDO_Arr = DefaultComponent->GetSensorsByType(InSensor_Type);
							for (int32 j = 0; j < SensorCDO_Arr.Num(); ++j)
							{
								if (IsValid(SensorCDO_Arr[j]))
								{
									UClass* ClassCDO = SensorCDO_Arr[j]->GetClass();
									if (SensorsArr.IsValidIndex(j) && IsValid(SensorsArr[j]))
									{
										const UClass* Class1 = SensorsArr[j]->GetClass();
										if (SensorCDO_Arr[j]->GetFName() != SensorsArr[j]->GetFName() || !Class1->IsChildOf(ClassCDO))
										{
											Rest.Add(FSenseSysRestoreObject(SensorCDO_Arr[j], ClassCDO, SensorCDO_Arr[j]->GetFName(), j, InSensor_Type));
										}
										//else if (SensorCDO_Arr[j]->GetFName() != SensorsArr[j]->GetFName()) //todo
										//{
										//	{
										//		Rest.Add(FSenseSysRestoreObject(nullptr, Class1, SensorCDO_Arr[j]->GetFName(), j, InSensor_Type));
										//		SensorsArr[j]->MarkPendingKill();
										//		SensorsArr[j] = nullptr;
										//	}
										//	{
										//		SensorsArr[j]->Rename(
										//			*SensorCDO_Arr[j]->GetFName().ToString(),
										//			this,
										//			REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
										//	}
										//}
									}
									else
									{
										if (!SensorsArr.IsValidIndex(j))
										{
											SensorsArr.Insert(nullptr, j);
										}
										Rest.Add(FSenseSysRestoreObject(SensorCDO_Arr[j], ClassCDO, SensorCDO_Arr[j]->GetFName(), j, InSensor_Type));
									}
								}
							}
						}
						break;
						/*
						else
						{
						for (uint8 i = 1; i < 4; i++)
						{
						const ESensorType Sensor_Type = static_cast<ESensorType>(i);
						TArray<USensorBase*>& SensorsArr = GetSensorsByType(Sensor_Type);
						TArray<USensorBase*>& DefSensorsArr = DefaultComponent->GetSensorsByType(Sensor_Type);
						for (int32 j = 0; j < DefSensorsArr.Num(); ++j)
						{
						if (IsValid(DefSensorsArr[j]))
						{
						UClass* Class2 = DefSensorsArr[j]->GetClass();
						if (SensorsArr.IsValidIndex(j))
						{
						UClass* Class1 = SensorsArr[j]->GetClass();
						if (DefSensorsArr[j]->GetFName() != SensorsArr[j]->GetFName() || !Class1->IsChildOf(Class1))
						{
						Rest.Add(FSenseSysRestoreObject(SensorsArr[j], Class2, SensorsArr[j]->GetFName(), j, Sensor_Type));
						}
						}
						else
						{
						Rest.Add(FSenseSysRestoreObject(SensorsArr[j], Class2, SensorsArr[j]->GetFName(), j, Sensor_Type));
						}
						}
						}
						}
						}
						*/
					}
					case EOwnerBlueprintClassType::SensorBaseBP:
					{
						checkNoEntry();
						break;
					}
				}
			}
		}
	}
	return Rest.Num() > 0;
}

void USenseReceiverComponent::RestoreSensorTestDefaults(TArray<FSenseSysRestoreObject>& Rest)
{
	const EObjectFlags Flags = GetMaskedFlags(RF_PropagateToSubObjects);
	for (auto& It : Rest)
	{
		UObject* Obj = NewObject<UObject>(this, It.Class, It.ObjectName, Flags, It.DefaultObject);
		TArray<USensorBase*>& SensorsArr = GetSensorsByType(It.SensorType);
		switch (It.SensorType)
		{
			case ESensorType::None:
			{
				checkNoEntry();
				break;
			}
			case ESensorType::Active:
			{
				SensorsArr[It.Idx] = Cast<UActiveSensor>(Obj);
				break;
			}
			case ESensorType::Passive:
			{
				SensorsArr[It.Idx] = Cast<UPassiveSensor>(Obj);
				break;
			}
			case ESensorType::Manual:
			{
				SensorsArr[It.Idx] = Cast<UActiveSensor>(Obj);
				break;
			}
			default:
			{
				checkNoEntry();
				break;
			}
		}
	}
}

void USenseReceiverComponent::CheckAndRestoreSensorTestDefaults(ESensorType InSensor_Type)
{
	TArray<FSenseSysRestoreObject> Rest;
	if (CheckSensorTestToDefaults(Rest, InSensor_Type))
	{
		RestoreSensorTestDefaults(Rest);
		MarkPackageDirty_Internal();
	}
}
void USenseReceiverComponent::MarkPackageDirty_Internal() const
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

#endif
