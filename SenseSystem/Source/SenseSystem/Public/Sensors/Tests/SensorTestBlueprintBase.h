//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sensors/Tests/SensorLocationTestBase.h"

#include "SensorTestBlueprintBase.generated.h"

class USensorBase;
class USenseReceiverComponent;
class AActor;


/**
 * Sensor Test Blueprint Base
 */
UCLASS(abstract, Blueprintable, BlueprintType, EditInlineNew, HideDropdown, meta = (DontUseGenericSpawnObject, BlueprintThreadSafe))
class SENSESYSTEM_API USensorTestBlueprintBase : public USensorLocationTestBase
{
	GENERATED_BODY()
public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

	/**Get Sense Receiver Component*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SensorTest", meta = (DisplayName = "GetSenseReceiver"))
	USenseReceiverComponent* GetSenseReceiverComponentBP() const;

	/**Sensor Owner*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SensorTest", meta = (DisplayName = "GetSensor"))
	USensorBase* GetSensorOwnerBP() const;

	/**Get Receiver Owner Actor*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SensorTest", meta = (DisplayName = "GetReceiverActor"))
	AActor* GetReceiverActorBP() const;

	/**Get Receiver Transform*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SensorTest", meta = (DisplayName = "GetReceiverTransform"))
	FTransform GetReceiverTransformBP() const;

	/**Get Cached Sensor Transform, (!) only if sensor Ready*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SensorTest", meta = (DisplayName = "GetSensorTransform"))
	FTransform GetSensorTransformBP() const;


	/********************************/
	/*	Blueprint Implementable		*/
	/********************************/

	/**
	*	Blueprint Thread Safe
	*	Prepare for the test
	*/
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "SenseSystem|SensorTest", meta = (DisplayName = "GetReadyToTest"))
	EUpdateReady GetReadyToTestBP();

	/**
	*	(!) Not Blueprint Thread Safe!!!
	*	PreTest Implementation
	*/
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "SenseSystem|SensorTest", meta = (DisplayName = "PreTest"))
	bool PreTest_BP();

	/**
	*	(!) Not Blueprint Thread Safe!!!
	*	TestBody Implementation
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SenseSystem|SensorTest", meta = (DisplayName = "RunTest"))
	ESenseTestResult RunTestBP(UPARAM(ref) FSensedStimulus& SensedStimulus) const;
	virtual ESenseTestResult RunTestBP_Implementation(UPARAM(ref) FSensedStimulus& SensedStimulus) const;

	/**
	*	(!) Not Blueprint Thread Safe!!!
	*	Single LocationTest Implementation
	*/
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "SenseSystem|SensorTest", meta = (DisplayName = "RunLocationTest"))
	ESenseTestResult RunLocationTestBP(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "SenseSystem|SensorTest", meta = (DisplayName = "Get Sensor Test Bound Box"))
	FBox GetSensorTestBoundBoxBP() const;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "SenseSystem|SensorTest", meta = (DisplayName = "Get Sensor Test Radius"))
	float GetSensorTestRadiusBP() const;

private:
	virtual EUpdateReady GetReadyToTest() override;
	virtual bool PreTest() override;
	virtual FBox GetSensorTestBoundBox() const override;
	virtual float GetSensorTestRadius() const override;

	virtual ESenseTestResult RunTest(FSensedStimulus& SensedStimulus) const override;
	virtual ESenseTestResult RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const override;
};

FORCEINLINE USenseReceiverComponent* USensorTestBlueprintBase::GetSenseReceiverComponentBP() const
{
	return GetSenseReceiverComponent();
}

FORCEINLINE USensorBase* USensorTestBlueprintBase::GetSensorOwnerBP() const
{
	return GetSensorOwner();
}

FORCEINLINE AActor* USensorTestBlueprintBase::GetReceiverActorBP() const
{
	return GetReceiverActor();
}

FORCEINLINE FTransform USensorTestBlueprintBase::GetReceiverTransformBP() const
{
	return GetReceiverTransform();
}
