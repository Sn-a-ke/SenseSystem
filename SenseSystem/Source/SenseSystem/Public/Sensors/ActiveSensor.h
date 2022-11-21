//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SenseSystem.h"
#include "Sensors/SensorBase.h"

#include "ActiveSensor.generated.h"

class AActor;

//delegates for manual sensor update
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSenseTestActor, class AActor*, Actor);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSensorAsyncComplete, class UActiveSensor*, ActiveSensor);

/**
* Base Class for Active Sensor
* Realtime Update Sensor
*/
UCLASS(BlueprintType, EditInlineNew) /*abstract, Blueprintable, HideDropdown,*/
class SENSESYSTEM_API UActiveSensor : public USensorBase
{
	GENERATED_BODY()

public:
	UActiveSensor(const FObjectInitializer& ObjectInitializer);
	virtual ~UActiveSensor() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

	virtual void BeginDestroy() override;
	virtual void InitializeFromReceiver(USenseReceiverComponent* FromReceiver) override;

	virtual void TickSensor(const float DeltaTime) override { Super::TickSensor(DeltaTime); }

	virtual void OnSensorUpdated() override;
	virtual void OnAgeUpdated() override;

	/************************************/

	/**Run Manual Sensor Detect*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Run Async Manual Sensor"))
	void RunManualSensor(bool bOverride_SenseState = true);

	/**Async Manual Run Sensor Detect*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Run Async Manual Sensor"))
	void AsyncManualSensor(UPARAM(DisplayName = "Event") FOnSensorAsyncComplete Delegate, bool bOverride_SenseState = true);

	/**Async Manual Run Sensor Detect*/
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor", meta = (Keywords = "Run Async Manual Sensor"))
	void AsyncManualSensorDetect_BestActor(UPARAM(DisplayName = "Event") FOnSenseTestActor Delegate, bool bOverride_SenseState = true);

	/************************************/

	virtual bool IsOverrideSenseState() const override
	{
		if (SensorType == ESensorType::Manual && bRequestManualUpdate)
		{
			return bOverrideSenseState;
		}
		return Super::IsOverrideSenseState();
	}

protected:
	/************************************/

	TArray<FOnSenseTestActor> CallbackTargetsSingleResult;
	TArray<FOnSensorAsyncComplete> Callback;
	bool bRequestManualUpdate = false;
	bool bOverrideSenseState = true;
};
