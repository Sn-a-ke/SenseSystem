//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/ObjectMacros.h"
#include "GameFramework/Actor.h"
#include "HAL/ThreadSafeBool.h"

#include "SenseSysHelpers.h"

#include "SenseStimulusBase.generated.h"


class ISensedStimulusLocation
{
public:
	virtual ~ISensedStimulusLocation() {}

	/**Get Single Point for Sensing, Actor Location As Example*/
	virtual FVector GetSingleSensePoint(FName SensorTag) const = 0;
	/**Get Points for Sensing*/
	virtual TArray<FVector> GetSensePoints(FName SensorTag) const = 0;
};


/** OnSenseEvent SenseSys */
UENUM(BlueprintType)
enum class EStimulusMobility : uint8
{
	Static = 0	 UMETA(DisplayName = "Static"),
	MovableOwner UMETA(DisplayName = "MovableOwner"),
	MovableTick  UMETA(DisplayName = "MovableTick")

	//todo animated sense points from animinstance
};


/** CallStimulusFlag */
UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EReceiveStimulusFlag : uint8
{
	DetectGlobalSenseState = 0x1,
	ReceiveOnCurrentSensed = 0x2,
	ReceiveOnNewSensed = 0x4,
	ReceiveOnLost = 0x8,
	ReceiveOnForget = 0x10
};


/**
 * StimulusTagResponse struct
 */
USTRUCT(BlueprintType)
struct SENSESYSTEM_API FStimulusTagResponse
{
	GENERATED_USTRUCT_BODY()


private:
	/** SensedStimulus ID */
	uint16 ObjID = MAX_uint16;

public:
	FStimulusTagResponse() {}

	/** Container Tree Ptr */
	class IContainerTree* ContainerTree = nullptr;

	/** BitChannels*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StimulusTagResponse")
	FBitFlag64_SenseSys BitChannels = FBitFlag64_SenseSys(1);

	/** Age*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StimulusTagResponse")
	float Age = 0.f;

	/** Score*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StimulusTagResponse")
	float Score = 1.f;

	/** ReceiveStimulusFlag */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SenseStimulus", meta = (Bitmask, BitmaskEnum = "/Script/SenseSystem.EReceiveStimulusFlag"))
	uint8 ReceiveStimulusFlag =											   //
		static_cast<uint8>(EReceiveStimulusFlag::DetectGlobalSenseState) | //
		static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnNewSensed) |	   //
		static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnLost) |		   //
		static_cast<uint8>(EReceiveStimulusFlag::ReceiveOnForget);		   //


	UPROPERTY(Transient)
	TMap<class AActor*, uint64> TmpSensed;
	UPROPERTY(Transient)
	TMap<class AActor*, uint64> TmpLost;


	FORCEINLINE uint16 GetObjID() const { return ObjID; }
	FORCEINLINE void SetObjID(const uint16 Val) { ObjID = Val; }

	void SetAge(float AgeValue);
	void SetScore(float ScoreValue);
	void SetBitChannels(uint64 NewBit);
	void UpdatePosition(const FVector& NewLocation, const TArray<FVector>& Points, float CurrentTime, bool bAllPoints = true) const;

	float GetAge() const;
	float GetScore() const;

	bool IsReceiveOnSense(EOnSenseEvent SenseEvent) const;
	bool IsOnSenseStateChanged(AActor* Actor, EOnSenseEvent OnSenseEvent, uint8 InChannel);

	FORCEINLINE bool IsResponseChannel(const uint8 InChannel) const
	{
		return BitChannels.Value & (1llu << (InChannel - 1));
	}
	FORCEINLINE bool IsResponseChannel(const FBitFlag64_SenseSys InChannels) const
	{
		return BitChannels.Value & InChannels;
	}
	
	//FORCEINLINE friend uint32 GetTypeHash(const FStimulusTagResponse& In) { return GetTypeHash(In.GetObjID()); }
	//FORCEINLINE bool operator==(const FStimulusTagResponse& Other) const { return SensorTag == Other.SensorTag; }
	//FORCEINLINE bool operator==(const FName& Other) const { return SensorTag == Other; }

	FStimulusTagResponse& operator=(const FStimulusTagResponse& Str);
	friend FArchive& operator<<(FArchive& Ar, FStimulusTagResponse& Str)
	{
		//Ar << Str.SensorTag;
		Ar << Str.BitChannels;
		Ar << Str.Age;
		Ar << Str.Score;
		Ar << Str.ReceiveStimulusFlag;
		return Ar;
	}



private:
	void AddSensed(AActor* Actor, uint8 InChannel);
	bool RemoveSensed(AActor* Actor, uint8 InChannel);
	void AddLost(AActor* Actor, uint8 InChannel);
	bool RemoveLost(AActor* Actor, uint8 InChannel);
};


class USensorBase;
class USenseManager;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSenseStateChanged, FName, SensorTag, EOnSenseEvent, SenseEvent); //todo OnSenseStateChanged  Channel?
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSensedFromSensor, const USensorBase*, Sensor, int32, Channel, EOnSenseEvent, SenseEvent);

DECLARE_DYNAMIC_DELEGATE_FourParams(FOnSensedFrom, AActor*, SensedActor, const USensorBase*, Sensor, int32, Channel, EOnSenseEvent, SenseEvent);

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnregisterSensorDelegate, AActor*, Actor, const USensorBase*, Sensor); //todo FOnUnregisterReceiverDelegate


/**
 * SenseStimulusBase
 */
UCLASS(BlueprintType, Blueprintable, Abstract, ClassGroup = (SenseSystem)) //, meta = (BlueprintSpawnableComponent)
class SENSESYSTEM_API USenseStimulusBase
	: public UActorComponent
	, public ISensedStimulusLocation
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USenseStimulusBase(const FObjectInitializer& ObjectInitializer);
	virtual ~USenseStimulusBase() override;

private:
	
	USenseManager* SenseManager = nullptr;

protected:
	FThreadSafeBool bRegisteredForSense = false;

	virtual bool RegisterSelfSense();
	virtual bool UnRegisterSelfSense();

	virtual void OnUnregister() override;
	virtual void OnRegister() override;

	/**Get Sense Manager*/
	USenseManager* GetSenseManager() const;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool IsSelfValid() const;

	virtual void Serialize(FArchive& Ar) override;
	//virtual void OnComponentCreated() override
	//{
	//	Super::OnComponentCreated();
	//	for (FStimulusTagResponse& Str : TagResponse)
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("OnComponentCreated -- SensorTag: %s, BitChannels: %llu"), *Str.SensorTag.ToString(), Str.BitChannels.Value );
	//	}
	//}

#if WITH_EDITOR

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;

#endif

#if WITH_EDITORONLY_DATA

	//FComponentVisualizer in editor module
	virtual void DrawComponent(const class FSceneView* View, class FPrimitiveDrawInterface* PDI) const;
	virtual void DrawComponentHUD(const class FViewport* Viewport, const class FSceneView* View, class FCanvas* Canvas) const;

	void MarkPackageDirty_Internal() const;

#endif

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;
	virtual void Cleanup();
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void SetComponentTickEnabled(const bool bTickEnabled) override;

	/************************************/


	/** Stimulus Mobility */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SenseStimulus")
	EStimulusMobility Mobility = EStimulusMobility::Static;


	/** Enable/Disable SenseStimulus */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SenseStimulus")
	bool bEnable = true;


	bool IsRegisteredForSense() const;

	/** Set Mobility */
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Set Stimulus Mobility"))
	void SetStimulusMobility(EStimulusMobility InMobility);

	/** Set Enable/Disable SenseStimulus */
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Set Enable Stimulus"))
	void SetEnableSenseStimulus(bool bNewEnable);

	/************************************/

	/** Sensed From Actor Delegate*/
	UPROPERTY(BlueprintAssignable, Category = "SenseStimulus")
	FOnSensedFromSensor OnSensedFromSensor;

	/**Global from all actors and all sensors, Sense State Changed Delegate*/
	UPROPERTY(BlueprintAssignable, Category = "SenseStimulus")
	FOnSenseStateChanged OnSenseStateChanged;

	/************************************/

	/** Response settings by Sensor Tag and channel */
	UPROPERTY(EditAnywhere, /*EditDefaultsOnly,*/ BlueprintReadOnly, Category = "SenseStimulus" /*, meta = (NoElementDuplicate)*/)
	TMap<FName, FStimulusTagResponse> TagResponse = {TPair<FName, FStimulusTagResponse>(NAME_None, FStimulusTagResponse())};


	/** Public StimulusSensorNameResponse Getter */
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Get Stimulus Sensor Tag Sensor Tag Name Response"))
	FStimulusTagResponse GetStimulusSensorTagResponse(FName SensorTag, bool& bSuccess) const;

	const TMap<FName, FStimulusTagResponse>& GetTagResponse() const;
	FStimulusTagResponse* GetStimulusTagResponse(const FName& SensorTag);
	const FStimulusTagResponse* GetStimulusTagResponse(const FName& SensorTag) const;

	void SetAge(FName SensorTag, float AgeValue);
	void SetScore(FName SensorTag, float ScoreValue);
	float GetAge(FName SensorTag) const;
	float GetScore(FName SensorTag) const;

	/** Set Age By Tag*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (DisplayName = "SetAge", Keywords = "Sense Set Age"))
	void SetAge_BP(FName SensorTag, float AgeValue);

	/** Set Score By Tag*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (DisplayName = "SetScore", Keywords = "Sense Set Score"))
	void SetScore_BP(FName SensorTag, float ScoreValue);

	/** Get Age By Tag*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (DisplayName = "GetAge", Keywords = "Sense Set Age"))
	float GetAge_BP(FName SensorTag) const;

	/** Get Score By Tag*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (DisplayName = "GetScore", Keywords = "Sense Set Score"))
	float GetScore_BP(FName SensorTag) const;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Set Receive Flag Stimulus"))
	void SetReceiveStimulusFlag(FName SensorTag, EReceiveStimulusFlag Flag, bool bValue);

	/************************************/

	/** Clear Response Channels */
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Sense Empty Clear Response Channel"))
	void ClearResponseChannels(FName Tag);
	/** Clear Response Channels */
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Sense Empty Clear Response Channel"))
	void ClearAllResponseChannels();

	/** Set Response Channel in Range 1 - 64 */
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Sense Set Response Channel"))
	void SetResponseChannel(FName Tag, uint8 Channel, bool bEnableChannel);

	/** Set Response Channels */
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (DisplayName = "SetResponseChannels", Keywords = "Sense Set Response Channel"))
	void SetResponseChannelsBit(FName Tag, FBitFlag64_SenseSys NewChannels);

	/************************************/

	bool IsResponseChannel(const FName& Tag, uint8 Channel) const;

	bool IsResponseTag(const FName& Tag) const;

protected:
	void SetResponseChannels(const FName& Tag, FStimulusTagResponse& Str, uint64 NewChannelsBit);

public:
	/************************************/

	/**works only if sensor call "OnSensedFromActor"*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Is Sense From Actor"))
	bool IsSensedFrom(const AActor* Actor, FName SensorTag) const;

	/**works only if sensor call "OnSensedFromActor"*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Is Remember From Actor"))
	bool IsRememberFrom(const AActor* Actor, FName SensorTag) const;

	/************************************/


	/** Get Current Sensed From Actors */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Get Current Sense From Actors"))
	TArray<AActor*> GetCurrentSensedFromActors(FName SensorTag) const;

	/** Get Lost Sensed From Actors*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Get Lost Sense From Actors"))
	TArray<AActor*> GetLostSensedFromActors(FName SensorTag) const;


	UFUNCTION()
	void OnUnregisterReceiver(UObject* Obj);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Sensed From Specify Actor"))
	void OnSensedFromSpecifyActor(const AActor* Actor, UPARAM(DisplayName = "Event") FOnSensedFrom Delegate) { SpecifyActorDelegateMap.Add(Actor, Delegate); }


	/************************************/

	/**Get Points for Sensing*/
	virtual TArray<FVector> GetSensePoints(FName SensorTag) const override;
	/**Get Single Point for Sensing, Actor Location As Example*/
	virtual FVector GetSingleSensePoint(FName SensorTag) const override;


	/************************************/

public:
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Report Sense Event Stimulus"))
	void ReportSenseEvent(FName Tag);

	void SensedFromSensorUpdate(const class USensorBase* Sensor, int32 Channel, EOnSenseEvent OnSenseEvent);

	/************************************/

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Add Ignore Trace Actor"))
	void AddIgnoreTraceActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Add Ignore Trace Actors"))
	void AddIgnoreTraceActors(const TArray<AActor*>& Actors);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Remove Ignore Trace Actor"))
	void RemoveIgnoreTraceActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Remove Ignore Trace Actors"))
	void RemoveIgnoreTraceActors(const TArray<AActor*>& Actors);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Clear Ignore Trace Actor"))
	void ClearIgnoreTraceActors();

	const TArray<AActor*>& GetIgnoreTraceActors() const;

protected:
	// NotUproperty
	TArray<AActor*> IgnoreTraceActors;
	FTimerHandle TickStartHandle;
	void ResetDelayTickStart();
	bool bDirtyTransform = true;
	UPROPERTY()
	TMap<const AActor*, FOnSensedFrom> SpecifyActorDelegateMap;

	/************************************/

	static uint64 ChannelToBit(TArray<uint8>& InChannels);

	FCriticalSection CriticalSection;

	virtual void RootComponentTransformUpdated(class USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);
	void BindTransformUpdated();
	void UnBindTransformUpdated() const;
};

FORCEINLINE const TArray<AActor*>& USenseStimulusBase::GetIgnoreTraceActors() const
{
	return IgnoreTraceActors;
}

FORCEINLINE bool USenseStimulusBase::IsRegisteredForSense() const
{
	return bRegisteredForSense;
}

FORCEINLINE bool USenseStimulusBase::IsSelfValid() const
{
	return bRegisteredForSense && IsValid(this);
}
FORCEINLINE USenseManager* USenseStimulusBase::GetSenseManager() const
{
	return SenseManager;
}

FORCEINLINE const TMap<FName, FStimulusTagResponse>& USenseStimulusBase::GetTagResponse() const
{
	return TagResponse;
}
FORCEINLINE FStimulusTagResponse* USenseStimulusBase::GetStimulusTagResponse(const FName& SensorTag)
{
	return TagResponse.Find(SensorTag);
}
FORCEINLINE const FStimulusTagResponse* USenseStimulusBase::GetStimulusTagResponse(const FName& SensorTag) const
{
	return TagResponse.Find(SensorTag);
}
FORCEINLINE FStimulusTagResponse USenseStimulusBase::GetStimulusSensorTagResponse(FName SensorTag, bool& bSuccess) const
{
	if (const FStimulusTagResponse* StrPtr = GetStimulusTagResponse(SensorTag))
	{
		bSuccess = true;
		return *StrPtr;
	}
	bSuccess = false;
	return FStimulusTagResponse();
}

FORCEINLINE void USenseStimulusBase::SetAge(const FName SensorTag, const float AgeValue)
{
	if (FStimulusTagResponse* StrPtr = GetStimulusTagResponse(SensorTag))
	{
		if (IsRegisteredForSense())
		{
			(*StrPtr).SetAge(AgeValue);
		}
		else
		{
			(*StrPtr).Age = AgeValue;
		}
	}
}
FORCEINLINE void USenseStimulusBase::SetScore(const FName SensorTag, const float ScoreValue)
{
	if (FStimulusTagResponse* StrPtr = GetStimulusTagResponse(SensorTag))
	{
		if (IsRegisteredForSense())
		{
			(*StrPtr).SetScore(ScoreValue);
		}
		else
		{
			(*StrPtr).Score = ScoreValue;
		}
	}
}
FORCEINLINE float USenseStimulusBase::GetAge(const FName SensorTag) const
{
	if (const FStimulusTagResponse* StrPtr = GetStimulusTagResponse(SensorTag))
	{
		return (*StrPtr).GetAge();
	}
	return 0.f;
}
FORCEINLINE float USenseStimulusBase::GetScore(const FName SensorTag) const
{
	if (const FStimulusTagResponse* StrPtr = GetStimulusTagResponse(SensorTag))
	{
		return (*StrPtr).GetScore();
	}
	return 0.f;
}

FORCEINLINE void USenseStimulusBase::SetAge_BP(const FName SensorTag, const float AgeValue)
{
	SetAge(SensorTag, AgeValue);
}
FORCEINLINE void USenseStimulusBase::SetScore_BP(const FName SensorTag, const float ScoreValue)
{
	SetScore(SensorTag, ScoreValue);
}
FORCEINLINE float USenseStimulusBase::GetAge_BP(const FName SensorTag) const
{
	return GetAge(SensorTag);
}
FORCEINLINE float USenseStimulusBase::GetScore_BP(const FName SensorTag) const
{
	return GetScore(SensorTag);
}


FORCEINLINE float FStimulusTagResponse::GetAge() const
{
	return Age;
}
FORCEINLINE float FStimulusTagResponse::GetScore() const
{
	return Score;
}


FORCEINLINE void FStimulusTagResponse::AddSensed(AActor* Actor, const uint8 InChannel)
{
	if (IsValid(Actor) && InChannel < 64)
	{
		uint64& Val = TmpSensed.FindOrAdd(Actor);
		Val |= (1llu << InChannel);
	}
}
FORCEINLINE bool FStimulusTagResponse::RemoveSensed(AActor* Actor, const uint8 InChannel)
{
	if (Actor && InChannel < 64)
	{
		if (uint64* Val = TmpSensed.Find(Actor))
		{
			uint64& ValRef = *Val;
			ValRef &= ~(1llu << InChannel);
			if (ValRef == 0)
			{
				TmpSensed.Remove(Actor);
				return true;
			}
		}
	}
	return false;
}

FORCEINLINE void FStimulusTagResponse::AddLost(AActor* Actor, const uint8 InChannel)
{
	if (IsValid(Actor) && InChannel < 64)
	{
		uint64& Val = TmpLost.FindOrAdd(Actor);
		Val |= (1llu << InChannel);
	}
}
FORCEINLINE bool FStimulusTagResponse::RemoveLost(AActor* Actor, const uint8 InChannel)
{
	if (Actor && InChannel < 64)
	{
		if (uint64* Val = TmpLost.Find(Actor))
		{
			uint64& ValRef = *Val;
			ValRef &= ~(1llu << InChannel);
			if (ValRef == 0)
			{
				TmpLost.Remove(Actor);
				return true;
			}
		}
	}
	return false;
}


FORCEINLINE FStimulusTagResponse& FStimulusTagResponse::operator=(const FStimulusTagResponse& Str)
{
	ContainerTree = Str.ContainerTree;
	ObjID = Str.ObjID;

	//SensorTag = Str.SensorTag;
	BitChannels = Str.BitChannels;
	Age = Str.Age;
	Score = Str.Score;
	ReceiveStimulusFlag = Str.ReceiveStimulusFlag;

	return *this;
}
