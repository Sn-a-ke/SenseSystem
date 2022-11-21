//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GameFramework/Actor.h"
#include "SenseStimulusBase.h"

#include "SenseStimulusComponent.generated.h"


/**
* SensorArrayByType
*/
UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ESSInterfaceFlag : uint8
{
	ESS_None                 = 0,
	ESS_SensePointsInterface = 0x04,
	ESS_SinglePointInterface = 0x08
};

/**
 * SenseStimulusComponent for Actor Owner SenseStimulusInterface
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup = (SenseSystem), meta = (BlueprintSpawnableComponent))
class SENSESYSTEM_API USenseStimulusComponent : public USenseStimulusBase
{
	GENERATED_BODY()

public:
	USenseStimulusComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~USenseStimulusComponent() override;

#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

	virtual void OnRegister() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SenseStimulus", 	meta = (Bitmask, BitmaskEnum = "ESSInterfaceFlag"))
	uint8 InterfaceOwnerBitFlags = 0;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SenseStimulus", meta = (Keywords = "Set Interface Flag Stimulus"))
	void SetInterfaceFlag(ESSInterfaceFlag Flag, bool bValue);

protected:
	/**Interface*/
	//UPROPERTY()//VisibleAnywhere, BlueprintReadOnly, Category = "SenseStimulus"
	bool bOwnerSenseStimulusInterface;

	void InitOwnerInterface();

	/**Get Main Point for Sensing*/
	virtual FVector GetSingleSensePoint(FName SensorTag) const override;

	/**Get Points for Sensing*/
	virtual TArray<FVector> GetSensePoints(FName SensorTag) const override;
};
