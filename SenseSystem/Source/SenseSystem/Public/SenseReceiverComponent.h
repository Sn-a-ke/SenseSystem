//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/NoExportTypes.h"
#include "UObject/UObjectBaseUtility.h"
#include "Components/SceneComponent.h"

#include "SenseSysHelpers.h"
#include "SensedStimulStruct.h"
#include "Sensors/SensorBase.h"
#include "Sensors/ActiveSensor.h"
#include "Sensors/PassiveSensor.h"

#include "SenseReceiverComponent.generated.h"


class USenseStimulusBase;
class USenseManager;
class UObject;

// clang-format off
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnSensorUpdateDelegate,
	const USensorBase*, SensorPtr,
	int32, Channel,
	const TArray<FSensedStimulus>&, inSensedStimulus);
// clang-format on

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMainTargetStatusChanged, AActor*, Actor, FSensedStimulus, SensedStimulus, const EOnSenseEvent, SenseEvent);


/**
* SenseReceiverComponent
*/
UCLASS(BlueprintType, Blueprintable, ClassGroup = (SenseSystem), meta = (BlueprintSpawnableComponent), hidecategories = ("Rendering"))
class SENSESYSTEM_API USenseReceiverComponent : public USceneComponent
{
	GENERATED_BODY()
public:
	USenseReceiverComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~USenseReceiverComponent() override;

private:
	// NotUproperty
	USenseManager* PrivateSenseManager = nullptr;

protected:
	/**self registration , calls on_begin_play*/
	bool RegisterSelfSense();

	/**self un_registration, calls on_begin_destroy or on_end_play*/
	bool UnRegisterSelfSense();

	//standard component registration
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;

	bool CheckSensorTestToDefaults(TArray<FSenseSysRestoreObject>& Rest, ESensorType InSensor_Type = ESensorType::None);
	void RestoreSensorTestDefaults(TArray<FSenseSysRestoreObject>& Rest);
	void CheckAndRestoreSensorTestDefaults(ESensorType InSensor_Type = ESensorType::None);
	void MarkPackageDirty_Internal() const;

#endif

#if WITH_EDITORONLY_DATA

	//FComponentVisualizer in editor module
	virtual void DrawComponent(const class FSceneView* View, class FPrimitiveDrawInterface* PDI) const;
	virtual void DrawComponentHUD(const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) const;

#endif

	//with remove component only
	virtual void Cleanup();

	/*USceneComponent*/
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;
	virtual void DestroyComponent(bool bPromoteChildren) override;

	//virtual void PostInitProperties() override;
	//void OnComponentCreated() override	{ Super::OnComponentCreated();}
	//void InitializeComponent() override	{ Super::InitializeComponent();}
	//void OnChildAttached(USceneComponent* ChildComponent) override { Super::OnChildAttached(ChildComponent);}
	//void OnChildDetached(USceneComponent* ChildComponent) override { Super::OnChildDetached(ChildComponent);}

	/************************************/

	/**Get Sense Manager*/
	USenseManager* GetSenseManager() const;

	/**enable-disable component*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SenseReceiver")
	bool bEnableSenseReceiver = true;

	/**take information about the orientation in space of the component from the controller*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SenseReceiver")
	bool bSensorRotationFromController = false;

	/************************************/


	UPROPERTY()
	bool bContainsSenseThreadSensors = false;

	void UpdateContainsSenseThreadSensors();
	bool ContainsSenseThreadSensors() const;

	/************************************/

	/** Realtime Update Sensors */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "SenseReceiver|Sensors")
	TArray<TObjectPtr<UActiveSensor>> ActiveSensors;

	/** Event Update Sensors */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "SenseReceiver|Sensors")
	TArray<TObjectPtr<UPassiveSensor>> PassiveSensors;

	/** Manual Update Sensors */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "SenseReceiver|Sensors")
	TArray<TObjectPtr<UActiveSensor>> ManualSensors;

	/************************************/
	// clang-format off

	/** CreateNewSensor in runtime */
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseReceiver", meta = (ExpandEnumAsExecs = SuccessState,DeterminesOutputType = "SensorClass", Keywords = "Create Add New Sensor"))
	USensorBase* CreateNewSensor(
		TSubclassOf<USensorBase> SensorClass,
		ESensorType Sensor_Type,
		FName Tag,
		ESensorThreadType ThreadType,
		bool bEnableNewSensor,
		ESuccessState& SuccessState);

	/** DestroySensor in runtime */
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseReceiver", meta = (DeterminesOutputType = "SensorClass", Keywords = "Destroy Remove Delete Sensor"))
	bool DestroySensor(ESensorType Sensor_Type, FName Tag);

	const TArray<TObjectPtr<USensorBase>>& GetSensorsByType(ESensorType Sensor_Type) const;
	TArray<TObjectPtr<USensorBase>>& GetSensorsByType(ESensorType Sensor_Type);
	
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = (Keywords = "Get Sensors By Type"))
	TArray<TObjectPtr<USensorBase>> GetSensorsByType_BP(ESensorType Sensor_Type) const;


	/*Get Sensor with force validation By Class*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = (ExpandEnumAsExecs = SuccessState, DeterminesOutputType = "SensorClass", Keywords = "Get Sensors By Class"))
	USensorBase* GetSensor_ByClass(TSubclassOf<USensorBase> SensorClass, ESuccessState& SuccessState) const;

	/*Get Sensor with force validation By Tag*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = (ExpandEnumAsExecs = SuccessState, Keywords = "Get Sensors By Tag"))
	USensorBase* GetSensor_ByTag(ESensorType Sensor_Type, FName Tag, ESuccessState& SuccessState) const;

	/*Get Sensor with force validation By Class and Tag*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = (ExpandEnumAsExecs = SuccessState, DeterminesOutputType = "SensorClass", Keywords = "Get Sensors By Tag Class"))
	USensorBase* GetSensor_ByTagAndClass(ESensorType Sensor_Type, FName Tag, TSubclassOf<USensorBase> SensorClass, ESuccessState& SuccessState) const;

	
	/*Get Active Sensor with force validation By Class*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = (ExpandEnumAsExecs = SuccessState, DeterminesOutputType = "SensorClass", Keywords = "Get Active Sensors By Class"))
	USensorBase* GetActiveSensor_ByClass(TSubclassOf<UActiveSensor> SensorClass, ESuccessState& SuccessState) const;

	/*Get Passive with force validation By Class*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = (ExpandEnumAsExecs = SuccessState, DeterminesOutputType = "SensorClass", Keywords = "Get Passive Sensors By Class"))
	USensorBase* GetPassiveSensor_ByClass(TSubclassOf<UPassiveSensor> SensorClass, ESuccessState& SuccessState) const;

	/*Get Manual Sensor with force validation By Class*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = (ExpandEnumAsExecs = SuccessState, DeterminesOutputType = "SensorClass", Keywords = "Get Manual Sensors By Class"))
	USensorBase* GetManualSensor_ByClass(TSubclassOf<UActiveSensor> SensorClass, ESuccessState& SuccessState) const;

	
	/*Get Active Sensor with force validation By Tag*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = (ExpandEnumAsExecs = SuccessState, Keywords = "Get Active Sensors By Tag"))
	UActiveSensor* GetActiveSensor_ByTag(FName Tag, ESuccessState& SuccessState) const;
	
	/*Get Passive Sensor with force validation By Tag*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = (ExpandEnumAsExecs = SuccessState, Keywords = "Get Passive Sensors By Tag"))
	USensorBase* GetPassiveSensor_ByTag(FName Tag, ESuccessState& SuccessState) const;
	
	/*Get Manual Sensor with force validation By Tag*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = (ExpandEnumAsExecs = SuccessState, Keywords = "Get Manual Sensors By Tag"))
	UActiveSensor* GetManualSensor_ByTag(FName Tag, ESuccessState& SuccessState) const;

	
	/**find where the actor in sensor*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = (DeprecatedFunction, ExpandEnumAsExecs = SuccessState, Keywords = "Find Sensed Actor"))
	void FindSensedActor(const AActor* Actor, ESensorType Sensor_Type, FName SensorTag, ESensorArrayByType SenseEventType, FSensedStimulus& Out, ESuccessState& SuccessState) const;

	/**find where the actor, iterate all sensors and all sensors array*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = (DeprecatedFunction, ExpandEnumAsExecs = SuccessState, Keywords = "Find Sensed Actor All Sensor"))
	void FindActorInAllSensors(const AActor* Actor, ESensorType& Sensor_Type, FName& SensorTag, ESensorArrayByType& SenseEventType, FSensedStimulus& Out, ESuccessState& SuccessState) const;

	
	/**find where the Stimulus in sensors*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = ( ExpandEnumAsExecs = SuccessState, Keywords = "Find"))
	TArray<FStimulusFindResult> FindStimulusInSensor(const USenseStimulusBase* StimulusComponent, ESensorType Sensor_Type, FName SensorTag, ESuccessState& SuccessState) const;

	/**find where the actor in sensors*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = ( ExpandEnumAsExecs = SuccessState, Keywords = "Find"))
	TArray<FStimulusFindResult> FindActorInSensor(const AActor* Actor, ESensorType Sensor_Type, FName SensorTag, ESuccessState& SuccessState) const;

	// clang-format on
	/************************************/

public:
	UPROPERTY(BlueprintAssignable)
	FOnSensorUpdateDelegate OnNewSense;

	UPROPERTY(BlueprintAssignable)
	FOnSensorUpdateDelegate OnCurrentSense;

	UPROPERTY(BlueprintAssignable)
	FOnSensorUpdateDelegate OnLostSense;

	UPROPERTY(BlueprintAssignable)
	FOnSensorUpdateDelegate OnForgetSense;

	UPROPERTY(BlueprintAssignable)
	FOnSensorUpdateDelegate OnUnregisterSense;

	FOnSensorUpdateDelegate& GetDelegateBySenseEvent(EOnSenseEvent SenseEvent);
	const FOnSensorUpdateDelegate& GetDelegateBySenseEvent(EOnSenseEvent SenseEvent) const;

	/************************************/

protected:
	TArray<USenseStimulusBase*> TrackTargetComponents; //sorted by hash //todo TSet?

public:
	UPROPERTY(BlueprintAssignable)
	FOnMainTargetStatusChanged OnMainTargetStatusChanged;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseReceiver", meta = (Keywords = "Get Track Target Component Components"))
	const TArray<USenseStimulusBase*>& GetTrackTargetComponents() const;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseReceiver", meta = (Keywords = "Get Track Target Actor Actors"))
	TArray<AActor*> GetTrackTargetActors() const;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseReceiver", meta = (Keywords = "Reset Empty Remove Clear Track Target"))
	void ClearTrackTargets() { TrackTargetComponents.Empty(); }

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseReceiver", meta = (Keywords = "Add Track Target Component"))
	void AddTrackTargetComponent(USenseStimulusBase* Comp);
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseReceiver", meta = (Keywords = "Add Track Target Actor"))
	void AddTrackTargetActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseReceiver", meta = (Keywords = "Remove Track Target Component"))
	void RemoveTrackTargetComponent(USenseStimulusBase* Comp);
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseReceiver", meta = (Keywords = "Remove Track Target Actor"))
	void RemoveTrackTargetActor(AActor* Actor);

	/************************************/

	virtual void TrackTargets(const TArray<FSensedStimulus>& InSensedStimulus, EOnSenseEvent SenseEvent) const;

	/************************************/

	UFUNCTION()
	void OnUnregisterStimulus(UObject* Obj);

	/************************************/

	/**Get Sensor Transform, it is called once before each sensor update, the sensor uses this transform to determine its location */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "SenseSystem|SenseReceiver", meta = (Keywords = "Get Sensor Transform Tag"))
	FTransform GetSensorTransform(FName SensorTag) const;
	virtual FTransform GetSensorTransform_Implementation(FName SensorTag) const;

	virtual FTransform GetSensorTransformDefault(FName SensorTag) const;
	virtual FVector GetSensorLocation(FName SensorTag) const;
	virtual FQuat GetSensorRotation(FName SensorTag) const;

	FQuat GetOwnerControllerRotation() const;

	/************************************/

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseReceiver", meta = (Keywords = " Set Enable Sense Receiver SenseReceiver"))
	void SetEnableSenseReceiver(bool bEnable);

	void SetEnableAllSensors(bool bEnable, bool bForget = true);

	UFUNCTION()
	void BindOnReportStimulusEvent(uint16 StimulusID, FName SensorTag);

	/************************************/

private:
	/** ObjectByClassPredicate */
	struct FObjectByClassPredicate
	{
		explicit FObjectByClassPredicate(const TSubclassOf<class UObject> InClass) : Class(InClass) {}
		const TSubclassOf<class UObject> Class = nullptr;
		FORCEINLINE bool operator()(const class UObject* S) const { return S && S->IsA(Class.Get()); }
	};
	/** SensorByTagPredicate */
	struct FSensorByTagPredicate
	{
		explicit FSensorByTagPredicate(const FName Tag) : FindTag(Tag) {}
		const FName FindTag = NAME_None;
		FORCEINLINE bool operator()(const USensorBase* S) const { return S && S->SensorTag == FindTag; }
	};

	template<class T>
	static USensorBase* FindInArray_ByTag(const TArray<TObjectPtr<T>>& InArr, FName Tag);
	template<class T>
	static USensorBase* FindInArray_ByClass(const TArray<TObjectPtr<T>>& InArr, TSubclassOf<USensorBase> SensorClass);

	template<class T>
	static const TArray<TObjectPtr<USensorBase>>& TypeArr(const TArray<TObjectPtr<T>>& InArr);
	template<class T>
	static TArray<TObjectPtr<USensorBase>>& TypeArr(TArray<TObjectPtr<T>>& InArr);
};


template<class T>
FORCEINLINE USensorBase* USenseReceiverComponent::FindInArray_ByTag(const TArray<TObjectPtr<T>>& InArr, const FName Tag)
{
	const auto& Arr = TypeArr(InArr);
	const int32 ID = Arr.IndexOfByPredicate(FSensorByTagPredicate(Tag));
	return ID != INDEX_NONE ? Arr[ID] : nullptr;
}
template<class T>
FORCEINLINE USensorBase* USenseReceiverComponent::FindInArray_ByClass(const TArray<TObjectPtr<T>>& InArr, const TSubclassOf<USensorBase> SensorClass)
{
	const auto& Arr = TypeArr(InArr);
	const int32 ID = Arr.IndexOfByPredicate(FObjectByClassPredicate(SensorClass));
	return ID != INDEX_NONE ? Arr[ID] : nullptr;
}

template<class T>
FORCEINLINE const TArray<TObjectPtr<USensorBase>>& USenseReceiverComponent::TypeArr(const TArray<TObjectPtr<T>>& InArr)
{
	return reinterpret_cast<const TArray<TObjectPtr<USensorBase>>&>(InArr);
}
template<class T>
FORCEINLINE TArray<TObjectPtr<USensorBase>>& USenseReceiverComponent::TypeArr(TArray<TObjectPtr<T>>& InArr)
{
	return reinterpret_cast<TArray<TObjectPtr<USensorBase>>&>(InArr);
}


FORCEINLINE void USenseReceiverComponent::UpdateContainsSenseThreadSensors()
{
	bContainsSenseThreadSensors = false;
	for (uint8 i = 1; i < 4; i++)
	{
		const auto& SensorsArr = GetSensorsByType(static_cast<ESensorType>(i));
		for (const auto It : SensorsArr)
		{
			if (It && (It->SensorThreadType == ESensorThreadType::Sense_Thread || It->SensorThreadType == ESensorThreadType::Sense_Thread_HighPriority))
			{
				bContainsSenseThreadSensors = true;
				return;
			}
		}
	}
}

FORCEINLINE bool USenseReceiverComponent::ContainsSenseThreadSensors() const
{
	return bContainsSenseThreadSensors;
}


FORCEINLINE FTransform USenseReceiverComponent::GetSensorTransform_Implementation(const FName SensorTag) const
{
	return GetSensorTransformDefault(SensorTag);
}

FORCEINLINE FTransform USenseReceiverComponent::GetSensorTransformDefault(const FName SensorTag) const
{
	FTransform Out = GetComponentTransform();
	if (bSensorRotationFromController)
	{
		Out.SetRotation(GetOwnerControllerRotation());
	}
	//UE_LOG(LogSenseSys, Warning, TEXT("GetOwnerControllerRotation(): %s"), *Out.ToString());
	return Out;
}

FORCEINLINE FVector USenseReceiverComponent::GetSensorLocation(const FName SensorTag) const
{
	return GetSensorTransformDefault(SensorTag).GetLocation();
}

FORCEINLINE FQuat USenseReceiverComponent::GetSensorRotation(const FName SensorTag) const
{
	//if array of sensor transform, what will be the rotation?
	return GetSensorTransformDefault(SensorTag).GetRotation();
}