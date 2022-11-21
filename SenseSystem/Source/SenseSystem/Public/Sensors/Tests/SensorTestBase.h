//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SensedStimulStruct.h"
#include "UObject/Object.h"


#if WITH_EDITORONLY_DATA
	#include "SceneManagement.h"
	#include "SceneView.h"
#endif

#include "SensorTestBase.generated.h"


class USensorBase;
class USenseReceiverComponent;
class USenseStimulusBase;
class AActor;


/** UpdateReady */
UENUM(BlueprintType)
enum class EUpdateReady : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Ready    UMETA(DisplayName = "Ready"),
	Skip     UMETA(DisplayName = "Skip"),
	Fail     UMETA(DisplayName = "Fail")
};


/**
 *	Base CLass for Sensor Test
 */
UCLASS(abstract, BlueprintType, EditInlineNew, HideDropdown, meta = (BlueprintThreadSafe = false)) //, Within = SensorBase
class SENSESYSTEM_API USensorTestBase : public UObject
{
	GENERATED_BODY()

public:
	USensorTestBase(const FObjectInitializer& ObjectInitializer);
	virtual ~USensorTestBase() override;
	virtual void BeginDestroy() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

#if WITH_EDITORONLY_DATA
	virtual void DrawTest(const FSceneView* View, FPrimitiveDrawInterface* PDI) const {}
	virtual void DrawTestHUD(const class FViewport* Viewport, const class FSceneView* View, class FCanvas* Canvas) const {}
	virtual void DrawDebug(float Duration) const {}
#endif

	/**Toggle enable Test*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest")
	bool bEnableTest = true;

	/**Force Skip Test*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest")
	bool bSkipTest = false;

	/**Skip test if input score < MinScore*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest")
	float MinScore = 0.f;

	bool NeedTest() const;

	/** Initialize from Sensor for this test */
	virtual void InitializeFromSensor();

	/** Initialize from Sensor, called once on sensor initialized */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "SenseSystem|SensorTest", meta = (DisplayName = "Initialize"))
	bool InitializeFromSensorBP(USensorBase* Sensor);

	virtual FBox GetSensorTestBoundBox() const { return FBox(FVector::ZeroVector, FVector::ZeroVector); }
	virtual float GetSensorTestRadius() const { return 0.f; }

	/** GetWorld */
	virtual class UWorld* GetWorld() const override;

	const FTransform& GetSensorTransform() const;

	FName GetSensorTag() const;

	USensorBase* GetSensorOwner() const;


	/** Prepare for the test, called once per update,  before RunTest, cache dynamic data for test  */
	virtual EUpdateReady GetReadyToTest() { return EUpdateReady::Ready; }

	/** PreTest */
	virtual bool PreTest();

	/** full test implementation */
	virtual ESenseTestResult RunTest(FSensedStimulus& SensedStimulus) const;

protected:
	/** Cache Static data for test , called once on test creation*/
	virtual void InitializeCacheTest() {}

	class AActor* GetReceiverActor() const;
	class USenseReceiverComponent* GetSenseReceiverComponent() const;
	const FTransform& GetReceiverTransform() const;

	static FVector NormalWithSizeSquared(const FVector& Vec, float SizeSquared);

	static bool IsObstacleInterface(const AActor* Actor);
	static bool IsStimulusInterface(const AActor* Actor);

	FTransform SensorTransform = FTransform::Identity;

	// NotUproperty
	USensorBase* PrivateSensorOwner = nullptr;

#if SENSESYSTEM_ENABLE_VECTORINTRINSICS

	union FVectorTransformConst
	{
	private:
		const FTransform* T;
		const VectorRegister* V;

	public:
		explicit FVectorTransformConst(const FTransform& InT) : T(&InT) {}
		const VectorRegister& GetRotation() const { return V[0]; }
		const VectorRegister& GetLocation() const { return V[1]; }
		const VectorRegister& GetScale() const { return V[2]; }
	};

#endif
};

FORCEINLINE bool USensorTestBase::NeedTest() const
{
	return bEnableTest && !bSkipTest;
}

FORCEINLINE USensorBase* USensorTestBase::GetSensorOwner() const
{
	return PrivateSensorOwner;
}

FORCEINLINE FVector USensorTestBase::NormalWithSizeSquared(const FVector& Vec, const float SizeSquared)
{
	if (SizeSquared == 1.f)
	{
		return Vec;
	}
	if (SizeSquared < KINDA_SMALL_NUMBER)
	{
		return FVector::ForwardVector;
	}
	return Vec * FMath::InvSqrt(SizeSquared);
}

FORCEINLINE const FTransform& USensorTestBase::GetSensorTransform() const
{
	return SensorTransform;
}

//todo: FilterAndScoring settings for TestBase
//UENUM(BlueprintType)
//enum class ESTType : uint8
//{
//	FilterAndScoring = 0	UMETA(DisplayName = "FilterAndScoring"), //all
//	FilterOnly				UMETA(DisplayName = "Filter"), //skipScoring
//	ScoringOnly				UMETA(DisplayName = "Scoring"), //skip filter
//};