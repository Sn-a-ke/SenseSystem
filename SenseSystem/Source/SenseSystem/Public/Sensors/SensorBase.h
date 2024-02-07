//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "Templates/Less.h"
#include "Math/NumericLimits.h"
#include "HAL/ThreadSafeCounter.h"
#include "HAL/ThreadSafeBool.h"
#include "Async/TaskGraphInterfaces.h"
#include "Async/AsyncWork.h"
#include "Delegates/DelegateSignatureImpl.inl"
#include "Containers/Map.h"
#include "Containers/Array.h"
#include "Algo/IsSorted.h"

#include "SenseSysHelpers.h"
#include "SensedStimulStruct.h"
#include "Sensors/Tests/SensorTestBase.h"
#include "SensedStimulStruct.h"

#if WITH_EDITORONLY_DATA
	#include "SceneManagement.h"
	#include "SceneView.h"
#endif

#include "SensorBase.generated.h"


class IContainerTree;

// clang-format off

/** CallStimulusFlag */
UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECallStimulusFlag : uint8
{
	CallOnCurrentSensed = 0x1,
	CallOnNewSensed     = 0x2,
	CallOnLost          = 0x4,
	CallOnForget        = 0x8	
};

// clang-format on


enum class ESensorState : uint8
{
	Uninitialized = 0,
	NotUpdate,
	ReadyToUpdate,
	Update,
	TestUpdated,
	AgeUpdate,
	PostUpdate
};

class FSensorState : private FThreadSafeCounter
{
public:
	FSensorState() {}
	explicit FSensorState(ESensorState InSensorState) : FThreadSafeCounter(static_cast<int32>(InSensorState)) {}

	FORCEINLINE ESensorState Set(ESensorState InSensorState) { return static_cast<ESensorState>(FThreadSafeCounter::Set(static_cast<int32>(InSensorState))); }
	FORCEINLINE ESensorState Get() const { return static_cast<ESensorState>(GetValue()); }

	FORCEINLINE void operator=(const ESensorState InSensorState) { Set(InSensorState); }
	FORCEINLINE operator ESensorState() const { return Get(); }
	FORCEINLINE bool operator==(const ESensorState InSensorState) const { return Get() == InSensorState; }
};


/** ChannelSetup */
USTRUCT(BlueprintType)
struct SENSESYSTEM_API FChannelSetup //128 byte
{
	GENERATED_USTRUCT_BODY()

	friend USensorBase;

	FChannelSetup(const uint8 InSenseChannel = 1);
	FChannelSetup(const FChannelSetup& In);
	~FChannelSetup();


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChannelSetup", meta = (ClampMin = "1", UIMin = "1", ClampMax = "64", UIMax = "64"))
	uint8 Channel = 1;

	/** if enable NewSense detecting from best score settings */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChannelSetup")
	bool bNewSenseForcedByBestScore = false;

	/** Count to detect best result */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChannelSetup", meta = (ClampMin = "0", UIMin = "0"))
	int32 TrackBestScoreCount = 1;

	/** Min Score for detect best Result */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChannelSetup")
	float MinBestScore = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "ChannelSetup")
	TArray<FSensedStimulus> NewSensed; // todo TArray<int32> indexes to CurrentSensed like a BestSensedID_ByScore

	UPROPERTY(BlueprintReadOnly, Category = "ChannelSetup")
	TArray<FSensedStimulus> CurrentSensed;

	UPROPERTY(BlueprintReadOnly, Category = "ChannelSetup")
	TArray<FSensedStimulus> LostAllSensed;


	UPROPERTY(BlueprintReadOnly, Category = "ChannelSetup")
	TArray<FSensedStimulus> LostCurrentSensed; //todo TArray<int32> indexes to LostAllSensed like a BestSensedID_ByScore

	UPROPERTY(BlueprintReadOnly, Category = "ChannelSetup")
	TArray<FSensedStimulus> ForgetSensed;

	UPROPERTY(BlueprintReadOnly, Category = "ChannelSetup")
	TArray<int32> BestSensedID_ByScore;


	FORCEINLINE uint64 GetSenseBitChannel() const { return Channel != 0 ? (1llu << (Channel - 1)) : 0; }

	const FSensedStimulus* GetBestSenseStimulus() const;

	FORCEINLINE operator uint8() const { return Channel; }

	FORCEINLINE friend uint32 GetTypeHash(const FChannelSetup& In) { return In.Channel; }

	FORCEINLINE bool operator==(const FChannelSetup& Other) const { return Channel == Other.Channel; }
	FORCEINLINE bool operator!=(const FChannelSetup& Other) const { return Channel != Other.Channel; }

	FORCEINLINE bool operator==(const uint8 OtherChannel) const { return Channel == OtherChannel; }
	FORCEINLINE bool operator!=(const uint8 OtherChannel) const { return Channel != OtherChannel; }

	FORCEINLINE bool operator<(const FChannelSetup& Other) const { return Channel < Other.Channel; }
	FORCEINLINE bool operator<(const uint8 OtherChannel) const { return Channel < OtherChannel; }

	FORCEINLINE bool operator>(const FChannelSetup& Other) const { return Channel > Other.Channel; }
	FORCEINLINE bool operator>(const uint8 OtherChannel) const { return Channel > OtherChannel; }

	void SetTrackBestScoreCount(int32 Count);
	void SetMinBestScore(float Score);

	const TArray<FSensedStimulus>& GetSensedStimulusBySenseEvent(ESensorArrayByType SenseEvent) const;
	TArray<FSensedStimulus>& GetSensedStimulusBySenseEvent(ESensorArrayByType SenseEvent);
	const TArray<FSensedStimulus>& GetSensedStimulusBySenseEvent(EOnSenseEvent SenseEvent) const;

	FChannelSetup& operator=(const FChannelSetup& Other);

	friend FArchive& operator<<(FArchive& Ar, FChannelSetup& In)
	{
		Ar << In.Channel;
		Ar << In.bNewSenseForcedByBestScore;
		Ar << In.TrackBestScoreCount;
		Ar << In.MinBestScore;
		return Ar;
	}

private:
	void Init();

	struct FDeleterSdp
	{
		FDeleterSdp() = default;
		FDeleterSdp(const FDeleterSdp&) = default;
		~FDeleterSdp() = default;
		FDeleterSdp& operator=(const FDeleterSdp&) = default;
		void operator()(class FSenseDetectPool* Ptr) const;

		template<typename U, typename = std::enable_if_t<TPointerIsConvertibleFromTo<U, FSenseDetectPool>::Value>>
		FDeleterSdp(const TDefaultDelete<U>&)
		{}
		template<typename U, typename = std::enable_if_t<TPointerIsConvertibleFromTo<U, FSenseDetectPool>::Value>>
		FDeleterSdp& operator=(const TDefaultDelete<U>&)
		{
			return *this;
		}
	};

	TUniquePtr<FSenseDetectPool, FDeleterSdp> _SenseDetect = nullptr;

	void OnSensorChannelUpdated(ESensorType InSensorType);
	void OnSensorAgeUpdated(ESensorType InSensorType, EUpdateReady UpdateReady);
};

USTRUCT(BlueprintType)
struct SENSESYSTEM_API FStimulusFindResult
{
	GENERATED_USTRUCT_BODY()

	FStimulusFindResult() {}
	~FStimulusFindResult() {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StimulusFIndResult")
	TWeakObjectPtr<class USensorBase> Sensor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StimulusFIndResult")
	ESensorType SensorType = ESensorType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StimulusFIndResult")
	FName SensorTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StimulusFIndResult")
	uint8 Channel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StimulusFIndResult")
	ESensorArrayByType SensedType = ESensorArrayByType::SenseForget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StimulusFIndResult")
	int32 SensedID = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StimulusFIndResult")
	FSensedStimulus SensedData = FSensedStimulus();
};


class USenseManager;
class USenseReceiverComponent;
class USenseStimulusBase;
class UObject;
class AActor;


class FUpdateSensorTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FUpdateSensorTask>;

public:
	FUpdateSensorTask(class USensorBase* InSensor) : Sensor(InSensor) {}
	~FUpdateSensorTask(){};

private:
	class USensorBase* Sensor;

public:
	void DoWork() const;

	static void OnDoWorkComplete() {}
	static FORCEINLINE TStatId GetStatId() { RETURN_QUICK_DECLARE_CYCLE_STAT(FUpdateSensorTask, STATGROUP_ThreadPoolAsyncTasks); }
};


/** SensorBase - The Base Class for all Sensors */
UCLASS(abstract, BlueprintType, EditInlineNew, HideDropdown, HideCategories = (SensorHide))
class SENSESYSTEM_API USensorBase : public UObject
{
	GENERATED_BODY()
public:
	using ElementIndexType = int32;

	USensorBase(const FObjectInitializer& ObjectInitializer);
	virtual ~USensorBase() override;

	virtual void Serialize(FArchive& Ar) override;

#if WITH_EDITOR

	TArray<uint8> GetUniqueChannelSetup() const;

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;

	virtual EDataValidationResult IsDataValid(FDataValidationContext& ValidationErrors) override;

	static uint8 GetUniqueChannel(const TArray<uint8>& Tmp, uint8 Start = 0);

	bool CheckSensorTestToDefaults(TArray<FSenseSysRestoreObject>& Rest);
	void RestoreSensorTestDefaults(TArray<FSenseSysRestoreObject>& Rest);
	void CheckAndRestoreSensorTestDefaults();
	void MarkPackageDirty_Internal() const;

#endif //WITH_EDITOR

#if WITH_EDITORONLY_DATA

	/** Draw Sensor on Edit or Selection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
	bool bDrawSensor = true;

	virtual void DrawSensor(const class FSceneView* View, class FPrimitiveDrawInterface* PDI) const;
	virtual void DrawSensorHUD(const class FViewport* Viewport, const class FSceneView* View, class FCanvas* Canvas) const;
	virtual void DrawDebug(bool bTest, bool bCurrentSensed, bool bLostSensed, bool bBestSensed, bool bAge, float Duration) const;

#endif

public:
	/** Draw Debug Sensor */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|Sensor", meta = (Keywords = "Draw Debug Sensor"))
	void DrawDebugSensor(bool bTest, bool bCurrentSensed, bool bLostSensed, bool bBestSensed, bool bAge, float Duration) const;


	//virtual void Serialize(FArchive& Ar) override;
	virtual void BeginDestroy() override;
	virtual class UWorld* GetWorld() const override;


	/** PreInitialization from Receiver */
	virtual void InitializeFromReceiver(USenseReceiverComponent* FromReceiver);

	/** Full Initialization*/
	virtual void InitializeForSense(USenseReceiverComponent* FromReceiver);

	virtual void Cleanup();

	virtual bool IsOverrideSenseState() const { return true; }


	EUpdateReady GetSensorUpdateReady() const;


	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Get Sensor Bound Box"))
	FBox GetSensorTestBoundBox() const;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Get Sensor Bound Box"))
	float GetSensorTestRadius() const;

	void GetSensorTest_BoxAndRadius(FBox& OutBox, float& OutRadius) const;

	/** Enable-Disable Sensor */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sensor")
	bool bEnable = true;

	/** Sensor Tag, Unique Sensor Identifier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sensor") //EditInstanceOnly
	FName SensorTag = NAME_None;

	/** Sensor Type */
	UPROPERTY(BlueprintReadOnly, Category = "Sensor")
	ESensorType SensorType = ESensorType::None;

	/** Sensor MultiThreading settings */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sensor")
	ESensorThreadType SensorThreadType = ESensorThreadType::Sense_Thread;

	/** SensorTransform update on GetSensorReadyBP */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Sensor")
	FTransform SensorTransform = FTransform::Identity;

	/** Zero value "0" - disable timer, try >=0.0167 (60 fps) or more */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", UIMin = "0.0"), Category = "Sensor")
	float UpdateTimeRate = 0.1f;

	/** Detect Depth */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sensor")
	EOnSenseEvent DetectDepth = EOnSenseEvent::SenseForget;

	/** CallStimulusFlag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor", meta = (Bitmask, BitmaskEnum = "/Script/SenseSystem.ECallStimulusFlag"))
	uint8 CallStimulusFlag =										 //
		static_cast<uint8>(ECallStimulusFlag::CallOnCurrentSensed) | //
		static_cast<uint8>(ECallStimulusFlag::CallOnNewSensed) |	 //
		static_cast<uint8>(ECallStimulusFlag::CallOnLost) |			 //
		static_cast<uint8>(ECallStimulusFlag::CallOnForget);		 //


	uint64 GetBitChannelChannelSetup() const;

	/** Response Channels */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sensor", meta = (DisplayName = "SenseChannels"))
	TArray<FChannelSetup> ChannelSetup = {FChannelSetup(1)};


	UPROPERTY()
	FBitFlag64_SenseSys BitChannels = FBitFlag64_SenseSys(1);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sensor", meta = (DisplayName = "IgnoreChannels"))
	FBitFlag64_SenseSys IgnoreBitChannels = FBitFlag64_SenseSys(0);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Get Response Channel ID"))
	int32 GetChannelID(uint8 InChannel = 1) const;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Set Sense Response Channel"))
	void SetSenseChannelsBit(FBitFlag64_SenseSys InChannels);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (DisplayName = "SetSenseResponseChannel", Keywords = "Set Sense Response Channel"))
	void SetSenseResponseChannelBit(uint8 InChannel, bool bEnableChannel);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (DisplayName = "SetIgnoreSenseChannels", Keywords = "Set Sense Response Channel"))
	void SetIgnoreSenseChannelsBit(FBitFlag64_SenseSys InChannels);

	/** Change Sensor Response Channel in range 1 - 64 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SenseSystem|Sensor",
		meta = (DisplayName = "SetIgnoreSenseResponseChannel", Keywords = "Set Sense Response Channel"))
	void SetIgnoreSenseResponseChannelBit(uint8 InChannel, bool bEnableChannel);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SenseSystem|Sensor", meta = (Keywords = "Check Sense Response Channel"))
	bool CheckResponseChannel(const USenseStimulusBase* StimulusComponent) const;

	// unsafe
	void Add_SenseChannels(uint64 NewChannels);
	// unsafe
	void Remove_SenseChannels(uint64 NewChannels);


	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Set Sensor Enable"))
	void SetEnableSensor(bool bInEnable);

	UFUNCTION(BlueprintCallable, Category = "Sensor", meta = (Keywords = "Set Sensor Thread Type"))
	void SetSensorThreadType(ESensorThreadType NewSensorThreadType);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Get Sensor Transform"))
	const FTransform& GetSensorTransform() const;


	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Set Sense Channel Param"))
	void SetSenseChannelParam(uint8 InChannel = 1, bool NewSenseForcedByBestScore = false, int32 TrackBestScoreCount = 1, float MinBestScore = 0.f);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Set New Sense Forced By Best Score"))
	void SetNewSenseForcedByBestScore(uint8 InChannel = 1, bool bValue = false);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Set Best Score Count"))
	void SetBestScore_Count(uint8 InChannel = 1, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Set Best Score Min MinScore"))
	void SetBestScore_MinScore(uint8 InChannel = 1, float MinScore = 0.f);


	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Set Timer Update Time Rate"))
	void SetUpdateTimeRate(float NewUpdateTimeRate);

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "SensorHide", Transient)
	bool bOuter = true;
#endif


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "SensorTests", meta = (EditCondition = bOuter))
	TArray<TObjectPtr<USensorTestBase>> SensorTests;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (DeterminesOutputType = "SensorTestClass", Keywords = "Create Add New Sensor Test"))
	USensorTestBase* CreateNewSensorTest(TSubclassOf<USensorTestBase> SensorTestClass, int32 SensorTestIndexPlace = 0);

	UFUNCTION(
		BlueprintCallable,
		Category = "SenseSystem|Sensor",
		meta = (DeterminesOutputType = "SensorTestClass", Keywords = "Destroy Remove Delete Sensor Test"))
	bool DestroySensorTest(TSubclassOf<USensorTestBase> SensorTestClass, int32 SensorTestIndexPlace = 0);


	UPROPERTY(BlueprintReadOnly, Category = "Sensor")
	TArray<AActor*> Ignored_Actors;

	UPROPERTY(BlueprintReadOnly, Category = "Sensor")
	TArray<USenseStimulusBase*> Ignored_Components;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (DisplayName = "GetIgnoredActors", Keywords = "Get Ignore Ignored Actor Actors"))
	TArray<AActor*> GetIgnoredActors_BP() const { return GetIgnoredActors(); }
	const TArray<AActor*>& GetIgnoredActors() const;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Add Ignore Ignored Actor Actors"))
	void AddIgnoreActor(AActor* IgnoreActor);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Add Ignore Ignored Actor Actors"))
	void AddIgnoreActors(TArray<AActor*> IgnoredActors);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Remove Ignore Ignored Actor Actors"))
	void RemoveIgnoredActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Remove Ignore Ignored Actor Actors"))
	void RemoveIgnoredActors(TArray<AActor*> Actors);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Reset Ignore Ignored Actor Actors"))
	void ResetIgnoredActors();

	void RemoveNullsIgnoreActorsAndComponents();


	USenseManager* GetSenseManager() const;

	USenseReceiverComponent* GetSenseReceiverComponent() const;

	AActor* GetSensorOwner() const;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (DisplayName = "GetSensorManager", Keywords = "Get Sense Manager"))
	USenseManager* GetSenseManager_BP() const;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (DisplayName = "GetSensorOwner", Keywords = "Get Sensor Owner Owner Actor"))
	AActor* GetSensorOwner_BP() const;

	UFUNCTION(
		BlueprintCallable,
		BlueprintPure = false,
		Category = "SenseSystem|Sensor",
		meta = (ExpandEnumAsExecs = SuccessState, Keywords = "Get Actor Get Owner As Class", DeterminesOutputType = "ActorClass"))
	AActor* GetSensorOwnerAs(TSubclassOf<AActor> ActorClass, ESuccessState& SuccessState) const;

	UFUNCTION(
		BlueprintCallable,
		Category = "SenseSystem|Sensor",
		meta = (DisplayName = "GetSenseReceiverComponent", Keywords = "Get SenseReceiver Receiver Component"))
	USenseReceiverComponent* GetSenseReceiverComponent_BP() const;

	UFUNCTION(BlueprintNativeEvent, Category = "SenseSystem|Sensor", meta = (DisplayName = "GetSensorReady", Keywords = "Get Sensor Ready"))
	EUpdateReady GetSensorReadyBP();
	virtual EUpdateReady GetSensorReadyBP_Implementation();

public:
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor")
	void ClearCurrentSense(bool bCallUpdate = true);
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor")
	void ClearCurrentMemorySense(bool bCallForget = true);
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor")
	void ForgetAllSensed();


	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|Sensor", meta = (Keywords = "Find Get Best Score Sensed Sense"))
	TArray<FSensedStimulus> FindBestScoreSensed(uint8 InChannel = 1, int32 Count = 1) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|Sensor", meta = (Keywords = "Find Get Best Age Score Sensed Sense"))
	TArray<FSensedStimulus> FindBestAgeSensed(uint8 InChannel = 1, int32 Count = 1) const;


	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Get Best SenseStimulus Sense Stimulus"))
	bool GetBestSenseStimulus(FSensedStimulus& SensedStimulus, uint8 InChannel = 1) const;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Get Best SenseStimulus Sense Stimulus Component"))
	USenseStimulusBase* GetBestSenseStimulusComponent(uint8 InChannel = 1) const;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Get Best Actor Component"))
	class AActor* GetBestSenseActor(uint8 Channel = 1) const;


	const TArray<FSensedStimulus>& GetSensedStimulusBySenseEvent(ESensorArrayByType SenseEvent, int32 ChannelID) const;
	const TArray<FSensedStimulus>& GetSensedStimulusBySenseEvent(EOnSenseEvent SenseEvent, int32 ChannelID) const;

	UFUNCTION(
		BlueprintCallable,
		BlueprintPure = false,
		Category = "SenseSystem|Sensor",
		meta = (DisplayName = "GetSensedStimulusBySenseEvent", Keywords = " Get Sensed Stimulus By Sense Event"))
	TArray<FSensedStimulus> GetSensedStimulusBySenseEvent_BP(ESensorArrayByType SenseEvent, uint8 Channel);


	static int32 FindInSortedArray(const TArray<FSensedStimulus>& Array, const FSensedStimulus& SensedStimulus);
	static int32 FindInSortedArray(const TArray<FSensedStimulus>& Array, const USenseStimulusBase* SensedStimulus);
	static int32 FindInSortedArray(const TArray<FSensedStimulus>& Array, const AActor* Actor);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|Sensor", meta = (Keywords = "Find Contains Actor"))
	bool FindActor(const AActor* Actor, ESensorArrayByType SenseState, FSensedStimulus& SensedStimulus, uint8 InChannel = 1) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|Sensor", meta = (Keywords = "Find Contains Component"))
	bool FindComponent(const USenseStimulusBase* Comp, ESensorArrayByType SenseState, FSensedStimulus& SensedStimulus, uint8 InChannel = 1) const;


	TArray<FStimulusFindResult> FindStimulusInAllState(
		const USenseStimulusBase* StimulusComponent,
		const struct FStimulusTagResponse& Str,
		FBitFlag64_SenseSys InChannels);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|Sensor", meta = (Keywords = "Find Stimulus Component In All State"))
	TArray<FStimulusFindResult> FindStimulusInAllState(const USenseStimulusBase* StimulusComponent, FBitFlag64_SenseSys InChannels);


	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|Sensor", meta = (Keywords = "Find Actor In All State"))
	TArray<FStimulusFindResult> FindActorInAllState(const AActor* Actor, FBitFlag64_SenseSys InChannel);

	UFUNCTION(
		BlueprintCallable,
		BlueprintPure = false,
		Category = "SenseSystem|Sensor",
		meta = (DisplayName = "FindStimulusInAllState_SingleChannel", Keywords = "Find Stimulus Component In All State"))
	FStimulusFindResult FindStimulusInAllState_SingleChannel(const USenseStimulusBase* StimulusComponent, uint8 InChannel = 1); //todo bug?


	/** Fast binary search actor in array_by_SenseState */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|Sensor", meta = (Keywords = "Find Contains Component"))
	bool ContainsActor(const AActor* Actor, ESensorArrayByType SenseState, uint8 InChannel = 1) const;

	/** fast binary search component in array_by_SenseState*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|Sensor", meta = (Keywords = "Find Contains Component"))
	bool ContainsComponent(const USenseStimulusBase* Comp, ESensorArrayByType SenseState, uint8 InChannel = 1) const;

	/** GetActorByClass in array_by_SenseState */
	UFUNCTION(
		BlueprintCallable,
		BlueprintPure = false,
		Category = "SenseSystem|Sensor",
		meta = (Keywords = "Get Actor By Class", DeterminesOutputType = "ActorClass"))
	AActor* GetSensedActorByClass(TSubclassOf<AActor> ActorClass, ESensorArrayByType SenseState, uint8 InChannel = 1) const;

	/** GetActorsByClass in array_by_SenseState */
	UFUNCTION(
		BlueprintCallable,
		BlueprintPure = false,
		Category = "SenseSystem|Sensor",
		meta = (Keywords = "Get Actor Actors By Class", DeterminesOutputType = "ActorClass"))
	TArray<AActor*> GetSensedActorsByClass(TSubclassOf<AActor> ActorClass, ESensorArrayByType SenseState, uint8 InChannel = 1) const;


	UFUNCTION(
		BlueprintCallable,
		BlueprintPure = false,
		Category = "SenseSystem|Sensor",
		meta = (Keywords = "Find Contains Memory Get Actor Component Location "))
	bool GetActorLocationFromMemory_ByComponent(const USenseStimulusBase* Comp, FVector& Location, uint8 InChannel = 1) const;

	UFUNCTION(
		BlueprintCallable,
		BlueprintPure = false,
		Category = "SenseSystem|Sensor",
		meta = (Keywords = "Find Contains Memory Get Actor Component Location "))
	bool GetActorLocationFromMemory_ByActor(const AActor* Actor, FVector& Location, uint8 InChannel = 1) const;


	bool IsDetect(EOnSenseEvent InDetectDepth) const;
	bool IsCallOnSense(EOnSenseEvent SenseEvent) const;


	/** For tick Age timer called from Receiver tick */
	virtual void TickSensor(float DeltaTime);

protected:
	virtual bool NeedStopTimer();
	virtual bool NeedContinueTimer();

public:
	/** UnRegister SenseStimulus called from sense manager*/
	virtual TArray<FStimulusFindResult> UnRegisterSenseStimulus(USenseStimulusBase* Ssc);

	/** Not Thread Safe Main Sensor work implementation */
	virtual bool UpdateSensor();

	/**  */
	virtual ElementIndexType ReportSenseStimulusEvent(USenseStimulusBase* SenseStimulus);
	virtual void ReportSenseStimulusEvent(ElementIndexType InStimulusID);

	/** check ready for sense tests */
	bool IsValidForTest() const;

	/** check ready for sense tests */
	bool IsValidForTest_Short() const;

	void TreadSafePostUpdate();

	void ForceLostCurrentSensed(const EOnSenseEvent Ost, const bool bOverrideSenseState = true) const;
	void ForceLostSensedStimulus(const USenseStimulusBase* SenseStimulus);

protected:
	void OnSensorUpdateReceiver(EOnSenseEvent SenseEvent, const FChannelSetup& InChannelSetup) const;

	virtual bool RunSensorTest();

	virtual void DetectionLostAndForgetUpdate();

	virtual bool PreUpdateSensor();
	virtual void PostUpdateSensor();


	virtual void OnSensorUpdated();
	virtual void OnAgeUpdated();

	virtual void TrySensorUpdate();

	virtual EUpdateReady GetSensorReady();

	virtual void OnSensorReady();
	virtual void OnSensorReadySkip();
	virtual void OnSensorReadyFail();


private:
	/** Collect specify tested SensedStimulus */
	template<typename ConType>
	bool SensorsTestForSpecifyComponents_V3(const IContainerTree* ContainerTree, ConType&& ObjIDs) const;
	float UpdtDetectPoolAndReturnMinScore() const;
	bool UpdtSensorTestForIDInternal(
		ElementIndexType Idx,
		const IContainerTree* ContainerTree,
		const float CurrentTime,
		const float MinScore,
		TArray<ElementIndexType>& ChannelContainsIDs) const;

public:
	/** Check Async Sensor Task IsWorkDone */
	bool IsSensorTaskWorkDone() const;

	FSensorState UpdateState = FSensorState(ESensorState::Uninitialized);

	bool IsInitialized() const;
	void ResetInitialization();

private:
	ESenseTestResult Sensor_Run_Test(float MinScore, const float CurrentTime, FSensedStimulus& Stimulus, TArray<ElementIndexType>& Out) const;
	void CheckWithCurrent(FSensedStimulus& SS, TArray<ElementIndexType>& Out) const;


	// NotUProperty
	USensorTestBase* SensorTest_WithAllSensePoint = nullptr;
	// NotUProperty
	USenseReceiverComponent* PrivateSenseReceiver = nullptr;
	// NotUProperty
	USenseManager* PrivateSenseManager = nullptr;

	/** Async Sensor Task Pointer */
	TUniquePtr<FAsyncTask<FUpdateSensorTask>> UpdateSensorTask = nullptr;

	uint8 bClearCurrentSense : 1;
	uint8 bClearCurrentMemory : 1;

	uint64 PendingNewChannels = 0;
	uint64 PendingRemoveChannels = 0;

	static bool IsZeroBox(const FBox& InBox);

protected:
	TMap<ElementIndexType, uint32> PendingUpdate;
	FThreadSafeBool bIsHavePendingUpdate;

	virtual float GetCurrentGameTimeInSeconds() const;

	/** Create Async Update  Sensor Task */
	void CreateUpdateSensorTask();

	/** Destroy Async Update Sensor Task */
	void DestroyUpdateSensorTask();

	/** Update Ready State*/
	EUpdateReady SensorUpdateReady = EUpdateReady::None;

	/** ticking timer for Age ,forget, lost  update */
	FTickingTimer SensorTimer;

	/** Sensor Critical Section*/
	mutable FCriticalSection SensorCriticalSection;

	FSimpleDelegateGraphTask::FDelegate PostUpdateDelegate = FSimpleDelegateGraphTask::FDelegate::CreateUObject(this, &USensorBase::PostUpdateSensor);
};


template<typename ConType>
bool USensorBase::SensorsTestForSpecifyComponents_V3(const IContainerTree* ContainerTree, ConType&& ObjIDs) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_NewSensed);

	const float CurrentTime = GetCurrentGameTimeInSeconds(); //once per update need CurrentGameTime
	if (CurrentTime == 0.f)
	{
		return false;
	}

	if (SensorTests.Num() != 0)
	{
		const float MinScore = UpdtDetectPoolAndReturnMinScore();
		TArray<ElementIndexType> ChannelContainsIDs;
		ChannelContainsIDs.Reserve(ChannelSetup.Num());

		for (const ElementIndexType ItID : ObjIDs)
		{
			if (UpdtSensorTestForIDInternal(ItID, ContainerTree, CurrentTime, MinScore, ChannelContainsIDs))
			{
				break;
			}
		}
	}
	return IsValidForTest_Short();
}

FORCEINLINE EUpdateReady USensorBase::GetSensorUpdateReady() const
{
	return SensorUpdateReady;
}
FORCEINLINE bool USensorBase::IsDetect(const EOnSenseEvent InDetectDepth) const
{
	return InDetectDepth <= DetectDepth;
}
FORCEINLINE bool USensorBase::IsInitialized() const
{
	return UpdateState != ESensorState::Uninitialized;
}

FORCEINLINE void USensorBase::ResetInitialization()
{
	UpdateState = ESensorState::Uninitialized;
}
FORCEINLINE bool USensorBase::IsZeroBox(const FBox& InBox)
{
	return InBox.Min == FVector::ZeroVector && InBox.Max == FVector::ZeroVector;
}

FORCEINLINE const TArray<AActor*>& USensorBase::GetIgnoredActors() const
{
	return Ignored_Actors;
}

FORCEINLINE bool USensorBase::IsValidForTest_Short() const
{
	return IsInitialized() && GetSenseManager() && GetSenseReceiverComponent();
}
FORCEINLINE bool USensorBase::IsValidForTest() const
{
	return IsValidForTest_Short() && IsValid(this) && ChannelSetup.Num() != 0; /*&& SensorTests.Num() != 0*/
}

FORCEINLINE USenseManager* USensorBase::GetSenseManager() const
{
	return PrivateSenseManager;
}
FORCEINLINE USenseReceiverComponent* USensorBase::GetSenseReceiverComponent() const
{
	return PrivateSenseReceiver;
}

FORCEINLINE EUpdateReady USensorBase::GetSensorReadyBP_Implementation()
{
	return EUpdateReady::Ready;
}

FORCEINLINE const FTransform& USensorBase::GetSensorTransform() const
{
	return SensorTransform;
}

FORCEINLINE bool USensorBase::NeedStopTimer()
{
	//return !(IsDetect(EOnSenseEvent::SenseForget) && SenseDetect.LostAll_Sensed.Num()) || SensorTimer.UpdateTimeRate == 0;
	//(SenseDetect.LostAll_Sensed.Num() == 0 && SenseDetect.Current_Sensed.Num() == 0) || SensorTimer.UpdateTimeRate == 0;
	return !NeedContinueTimer();
}


FORCEINLINE int32 USensorBase::GetChannelID(const uint8 InChannel) const
{
	checkSlow(Algo::IsSorted(ChannelSetup, TLess<uint8>()));
	if (InChannel > 0 && InChannel <= 64)
	{
		return Algo::BinarySearch(ChannelSetup, InChannel, TLess<uint8>());
	}
	return INDEX_NONE;
}

/** Create Async Sensor Task */
FORCEINLINE void USensorBase::CreateUpdateSensorTask()
{
	if (!UpdateSensorTask.IsValid())
	{
		UpdateSensorTask = MakeUnique<FAsyncTask<FUpdateSensorTask>>(this);
		UpdateSensorTask->StartBackgroundTask();
	}
}

/** Destroy Async Sensor Task */
FORCEINLINE void USensorBase::DestroyUpdateSensorTask()
{
	if (UpdateSensorTask)
	{
		UpdateSensorTask->EnsureCompletion();
		UpdateSensorTask = nullptr;
	}
}

/** Check Async Sensor Task IsWorkDone */
FORCEINLINE bool USensorBase::IsSensorTaskWorkDone() const
{
	if (UpdateSensorTask)
	{
		return UpdateSensorTask->IsWorkDone();
	}
	return true;
}


FORCEINLINE const TArray<FSensedStimulus>& USensorBase::GetSensedStimulusBySenseEvent(const ESensorArrayByType SenseEvent, const int32 ChannelID) const
{
	check(ChannelSetup.IsValidIndex(ChannelID));
	return ChannelSetup[ChannelID].GetSensedStimulusBySenseEvent(SenseEvent);
}

FORCEINLINE const TArray<FSensedStimulus>& USensorBase::GetSensedStimulusBySenseEvent(const EOnSenseEvent SenseEvent, const int32 ChannelID) const
{
	check(ChannelSetup.IsValidIndex(ChannelID));
	return ChannelSetup[ChannelID].GetSensedStimulusBySenseEvent(SenseEvent);
}


FORCEINLINE FBox USensorBase::GetSensorTestBoundBox() const
{
	if (SensorTests.Num())
	{
		if (const USensorTestBase* St = SensorTests[0])
		{
			const FBox LocBox = St->GetSensorTestBoundBox();
			if (LocBox.Min != FVector::ZeroVector || LocBox.Max != FVector::ZeroVector)
			{
				return LocBox;
			}
		}
	}
	return FBox(FVector::ZeroVector, FVector::ZeroVector);
}
FORCEINLINE float USensorBase::GetSensorTestRadius() const
{
	if (SensorTests.Num())
	{
		if (const USensorTestBase* St = SensorTests[0])
		{
			const float Rad = St->GetSensorTestRadius();
			if (Rad != 0.f) return Rad;
		}
	}
	return 0.f;
}

FORCEINLINE uint64 USensorBase::GetBitChannelChannelSetup() const
{
	uint64 Out = 0;
	for (const auto& It : ChannelSetup)
	{
		Out |= It.GetSenseBitChannel();
	}
	return Out;
}

FORCEINLINE FChannelSetup& FChannelSetup::operator=(const FChannelSetup& Other)
{
	Channel = Other.Channel;
	bNewSenseForcedByBestScore = Other.bNewSenseForcedByBestScore;
	TrackBestScoreCount = Other.TrackBestScoreCount;
	MinBestScore = Other.MinBestScore;

	NewSensed = Other.NewSensed;
	CurrentSensed = Other.CurrentSensed;
	LostAllSensed = Other.LostAllSensed;
	LostCurrentSensed = Other.LostCurrentSensed;
	ForgetSensed = Other.ForgetSensed;
	BestSensedID_ByScore = Other.BestSensedID_ByScore;

	return *this;
}

FORCEINLINE const TArray<FSensedStimulus>& FChannelSetup::GetSensedStimulusBySenseEvent(const ESensorArrayByType SenseEvent) const
{
	switch (SenseEvent)
	{
		case ESensorArrayByType::SensedNew: return NewSensed;
		case ESensorArrayByType::SenseCurrent: return CurrentSensed;
		case ESensorArrayByType::SenseCurrentLost: return LostCurrentSensed;
		case ESensorArrayByType::SenseForget: return ForgetSensed;
		case ESensorArrayByType::SenseLost: return LostAllSensed;
	}
	checkNoEntry();
	UE_ASSUME(0);
	return CurrentSensed;
}
FORCEINLINE TArray<FSensedStimulus>& FChannelSetup::GetSensedStimulusBySenseEvent(const ESensorArrayByType SenseEvent)
{
	switch (SenseEvent)
	{
		case ESensorArrayByType::SensedNew: return NewSensed;
		case ESensorArrayByType::SenseCurrent: return CurrentSensed;
		case ESensorArrayByType::SenseCurrentLost: return LostCurrentSensed;
		case ESensorArrayByType::SenseForget: return ForgetSensed;
		case ESensorArrayByType::SenseLost: return LostAllSensed;
	}
	checkNoEntry();
	UE_ASSUME(0);
	return CurrentSensed;
}

FORCEINLINE const TArray<FSensedStimulus>& FChannelSetup::GetSensedStimulusBySenseEvent(const EOnSenseEvent SenseEvent) const
{
	switch (SenseEvent)
	{
		case EOnSenseEvent::SenseNew: return NewSensed;
		case EOnSenseEvent::SenseCurrent: return CurrentSensed;
		case EOnSenseEvent::SenseLost: return LostCurrentSensed;
		case EOnSenseEvent::SenseForget: return ForgetSensed;
	}
	checkNoEntry();
	UE_ASSUME(0);
	return CurrentSensed;
}

FORCEINLINE const FSensedStimulus* FChannelSetup::GetBestSenseStimulus() const
{
	if (BestSensedID_ByScore.IsValidIndex(0) && CurrentSensed.IsValidIndex(BestSensedID_ByScore[0]))
	{
		return &CurrentSensed[BestSensedID_ByScore[0]];
	}
	return nullptr;
}

FORCEINLINE bool USensorBase::IsCallOnSense(const EOnSenseEvent SenseEvent) const
{
	switch (SenseEvent)
	{
		case EOnSenseEvent::SenseNew: return CallStimulusFlag & static_cast<uint8>(ECallStimulusFlag::CallOnNewSensed);
		case EOnSenseEvent::SenseCurrent: return CallStimulusFlag & static_cast<uint8>(ECallStimulusFlag::CallOnCurrentSensed);
		case EOnSenseEvent::SenseLost: return CallStimulusFlag & static_cast<uint8>(ECallStimulusFlag::CallOnLost);
		case EOnSenseEvent::SenseForget: return CallStimulusFlag & static_cast<uint8>(ECallStimulusFlag::CallOnForget);
	}
	checkNoEntry();
	UE_ASSUME(0);
	return false;
}

//FORCEINLINE bool USensorBase::CheckResponseChannel(const FSensedStimulus& Stimulus) const
//{
//	return BitChannels & Stimulus.BitChannels & ~IgnoreBitChannels;
//}
//FORCEINLINE bool USensorBase::CheckResponseChannel(const FSensedStimulus* Stimulus) const
//{
//	if (Stimulus) return CheckResponseChannel(*Stimulus);
//	return false;
//}
//FORCEINLINE bool USensorBase::ContainsIgnoreChannel(const uint8 InChannel) const
//{
//	return IgnoreBitChannels & (1llu << (InChannel - 1));
//}
//FORCEINLINE bool USensorBase::ContainsSenseChannel(const uint8 InChannel) const
//{
//	return BitChannels & (1llu << (InChannel - 1));
//}
//FORCEINLINE bool USensorBase::CheckChannel(const uint8 InChannel)const
//{
//	return BitChannels & (1llu << (InChannel - 1)) & ~IgnoreBitChannels);
//}