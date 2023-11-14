//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sensors/PassiveSensor.h"
#include "Components/PrimitiveComponent.h"
#include "TimerManager.h"

#include "SensorTouch.generated.h"

/**
 * SensorTouch
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, HideCategories = (SensorTests))
class SENSESYSTEM_API USensorTouch : public UPassiveSensor
{
	GENERATED_BODY()
public:
	USensorTouch(const FObjectInitializer& ObjectInitializer);

	virtual ~USensorTouch() override;

	virtual void BeginDestroy() override;
	virtual void Cleanup() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

#if WITH_EDITORONLY_DATA
	virtual void DrawSensor(const class FSceneView* View, class FPrimitiveDrawInterface* PDI) const override;
	virtual void DrawSensorHUD(const class FViewport* Viewport, const class FSceneView* View, class FCanvas* Canvas) const override;
#endif

	/************************************/

	UPROPERTY(BlueprintReadOnly, Category = "Sensor")
	TArray<TObjectPtr<UPrimitiveComponent>> TouchCollisions;

	/************************************/

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTouch")
	bool bActorRootCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTouch")
	bool bOnComponentHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTouch")
	bool bOnComponentBeginOverlap = true;

	/************************************/


	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor|SensorTouch")
	void SetTouchCollision(UPrimitiveComponent* Prim);
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor|SensorTouch")
	void SetTouchCollisions(TArray<UPrimitiveComponent*> InTouchCollisions);

	/************************************/

	UFUNCTION()
	void AddTouchCollision(UPrimitiveComponent* InTouchCollision);
	UFUNCTION()
	void AddTouchCollisions(TArray<UPrimitiveComponent*> InTouchCollisions);
	UFUNCTION()
	void RemoveTouchCollision(UPrimitiveComponent* InTouchCollision);
	UFUNCTION()
	void RemoveTouchCollisions(TArray<UPrimitiveComponent*> InTouchCollisions);

	/************************************/

	UFUNCTION()
	void OnTouchHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION()
	void OnBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/************************************/

	void BindHitEvent(UPrimitiveComponent* InTouchCollision);
	void UnBindHitEvent(UPrimitiveComponent* InTouchCollision);

	void BindHitEvent(TArray<UPrimitiveComponent*> InTouchCollisions);
	void UnBindHitEvent(TArray<UPrimitiveComponent*> InTouchCollisions);

	/************************************/

	virtual void InitializeFromReceiver(USenseReceiverComponent* InSenseReceiver) override;

	TSet<USenseStimulusBase*> NeedLost;
	void LostCurrentSensed();
	void UpdateNeedLost();
};
