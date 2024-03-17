//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectBaseUtility.h"
#include "Components/SceneComponent.h"

#include "SenseSystem.h"
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
class SENSESYSTEM_API USenseReceiverComponent final : public USceneComponent
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
	virtual EDataValidationResult IsDataValid(FDataValidationContext& ValidationErrors) override;

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


	/************************************/

	/**Get Sense Manager*/
	USenseManager* GetSenseManager() const;

	/**enable-disable component*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SenseReceiver")
	bool bEnableSenseReceiver = true;

	/**take information about the orientation in space of the component from the controller*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SenseReceiver")
	bool bSensorRotationFromController = false;

	UPROPERTY(EditDefaultsOnly)
	bool bAllowRuntimeSensorCreation = false;

	/************************************/


	UPROPERTY()
	bool bContainsSenseThreadSensors = false;

	void UpdateContainsSenseThreadSensors();
	bool ContainsSenseThreadSensors() const;

	/************************************/

	/** Realtime Update Sensors */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "SenseReceiver|Sensors")
	TArray<UActiveSensor*> ActiveSensors; //todo: Instanced Array?

	/** Event Update Sensors */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "SenseReceiver|Sensors")
	TArray<UPassiveSensor*> PassiveSensors; //todo: Instanced Array?

	/** Manual Update Sensors */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "SenseReceiver|Sensors")
	TArray<UActiveSensor*> ManualSensors; //todo: Instanced Array?

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

	TArray<USensorBase*> GetSensorsByType(ESensorType Sensor_Type) const;
	TArrayView<USensorBase*> GetSensorsByType(ESensorType Sensor_Type);
	
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SenseSystem|SenseReceiver", meta = (Keywords = "Get Sensors By Type"))
	TArray<USensorBase*> GetSensorsByType_BP(ESensorType Sensor_Type) const;


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
	UPassiveSensor* GetPassiveSensor_ByTag(FName Tag, ESuccessState& SuccessState) const;
	
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
	TArray<USenseStimulusBase*> GetTrackTargetComponents() const;

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
	void BindOnReportStimulusEvent(int32 StimulusID, FName SensorTag);

	/************************************/

private:
	template<class T>
	static T* FindInSensorArray_ByTag(const TArray<T*>& InArr, FName Tag)
	{
		return *InArr.FindByPredicate([Tag](const T* S) { return S && S->SensorTag == Tag; });
	}
	template<class T>
	static T* FindInSensorArray_ByClass(const TArray<T*>& InArr, TSubclassOf<USensorBase> SensorClass)
	{
		return *InArr.FindByPredicate([&SensorClass](const T* S) { return S && S->IsA(SensorClass.Get()); });
	}

	template<typename T, typename U>
	static FORCEINLINE TArrayView<T*> TypeArr(TArray<U*>& InArr)
	{
		return TArrayView<T*>(reinterpret_cast<T**>(InArr.GetData()), InArr.Num());
	}
	template<typename InvokeFunction, typename... Args>
	FORCEINLINE void ForSensorType(ESensorType SensorType, InvokeFunction Function, Args... args)
	{
		for (USensorBase* const It : GetSensorsByType(SensorType))
		{
			if (IsValid(It))
			{
				Invoke(Function, It, Forward<Args>(args)...);
			}
		}
	}
	template<typename InvokeFunction, typename... Args>
	FORCEINLINE void ForEachSensor(InvokeFunction Function, Args... args)
	{
		for (uint8 i = 1; i < 4; i++)
		{
			ForSensorType<InvokeFunction, Args...>(static_cast<ESensorType>(i), Function, Forward<Args>(args)...);
		}
	}
};

FORCEINLINE TArray<USensorBase*> USenseReceiverComponent::GetSensorsByType(const ESensorType Sensor_Type) const
{
	switch (Sensor_Type)
	{
		case ESensorType::Active: return TArray<USensorBase*>(ActiveSensors);
		case ESensorType::Passive: return TArray<USensorBase*>(PassiveSensors);
		case ESensorType::Manual: return TArray<USensorBase*>(ManualSensors);
		default: break;
	}
	return {};
}
FORCEINLINE TArrayView<USensorBase*> USenseReceiverComponent::GetSensorsByType(const ESensorType Sensor_Type)
{
	switch (Sensor_Type)
	{
		case ESensorType::Active: return TypeArr<USensorBase>(ActiveSensors);
		case ESensorType::Passive: return TypeArr<USensorBase>(PassiveSensors);
		case ESensorType::Manual: return TypeArr<USensorBase>(ManualSensors);
		default: break;
	}
	return TArrayView<USensorBase*>();
}
FORCEINLINE TArray<USensorBase*> USenseReceiverComponent::GetSensorsByType_BP(const ESensorType Sensor_Type) const
{
	return TArray<USensorBase*>(GetSensorsByType(Sensor_Type));
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