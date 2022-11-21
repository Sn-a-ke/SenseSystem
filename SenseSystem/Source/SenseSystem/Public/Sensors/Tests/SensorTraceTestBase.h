//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sensors/Tests/SensorLocationTestBase.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"

#include "SensorTraceTestBase.generated.h"


/**
* TraceTestParam
*/
UENUM(BlueprintType)
enum class ETraceTestParam : uint8
{
	BoolTraceTest = 0 UMETA(DisplayName = "BoolTraceTest"),
	MultiTraceTest    UMETA(DisplayName = "MultiTraceTest"),
};

class UWorld;

/**
 * SensorTraceTestBase
 */
UCLASS(abstract, BlueprintType, EditInlineNew, HideDropdown)
class SENSESYSTEM_API USensorTraceTestBase : public USensorLocationTestBase
{
	GENERATED_BODY()
public:
	USensorTraceTestBase(const FObjectInitializer& ObjectInitializer);

	virtual void BeginDestroy() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest")
	ETraceTestParam TraceTestParam = ETraceTestParam::BoolTraceTest;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest")
	TEnumAsByte<ECollisionChannel> CollisionChannel = ECollisionChannel::ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest")
	bool bTraceComplex = false;
	
	virtual void InitializeFromSensor() override;
	//virtual void OnSensorChanged() override;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SensorTest")
	void SetBaseTraceParam(ETraceTestParam TraceParam, ECollisionChannel Collision, bool TraceComplex);

protected:
	FCollisionQueryParams Collision_Params;

	bool ScoreFromTransparency(const TArray<struct FHitResult>& OutHits, float& InScore) const;

	virtual void InitCollisionParams();

	virtual bool LineMultiTraceTest(
		const UWorld* World, const FCollisionQueryParams& CollisionParams, const FVector& StartTrace, FSensedStimulus& SensedStimulus) const;

	virtual bool LineBoolTraceTest(
		const UWorld* World, const FCollisionQueryParams& CollisionParams, const FVector& StartTrace, FSensedStimulus& SensedStimulus) const;

	UE_DEPRECATED(4.23, "unused")
	static FHitResult* GetBlockingHit(TArray<struct FHitResult>& Hits);
};