//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/SensorTouch.h"
#include "Sensors/Tests/SensorBoolTest.h"
#include "SenseReceiverComponent.h"
#include "SenseStimulusComponent.h"
#include "SenseSystemBPLibrary.h"
#include "SenseManager.h"

#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

#if WITH_EDITORONLY_DATA
	#include "Components/CapsuleComponent.h"
	#include "Components/BoxComponent.h"
	#include "Components/SphereComponent.h"

	#include "Engine/Engine.h"
	#include "CanvasTypes.h"
	#include "SceneManagement.h"
	#include "SceneView.h"
	#include "UnrealClient.h"
#endif


USensorTouch::USensorTouch(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SensorTests.SetNum(1);
	SensorTests[0] = CreateDefaultSubobject<USensorBoolTest>(TEXT("BoolTest"));
	auto BT = Cast<USensorBoolTest>(SensorTests[0]);
	if (BT)
	{
		BT->TestResult = ESenseTestResult::Sensed;
	}
}

USensorTouch::~USensorTouch()
{
}

void USensorTouch::BeginDestroy()
{
	UnBindHitEvent(TouchCollisions);
	TouchCollisions.Empty();
	Super::BeginDestroy();
}

void USensorTouch::Cleanup()
{
	UnBindHitEvent(TouchCollisions);
	Super::Cleanup();
}

#if WITH_EDITOR
void USensorTouch::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
}
#endif

#if WITH_EDITORONLY_DATA
void USensorTouch::DrawSensor(const class FSceneView* View, class FPrimitiveDrawInterface* PDI) const
{
	const UWorld* World = GetWorld();
	if (!bEnable && !World) return;

	FSenseSysDebugDraw DebugDraw;
	if (GetSenseManager() && World->IsGameWorld())
	{
		DebugDraw = GetSenseManager()->GetSenseSysDebugDraw(SensorTag);
	}
	else
	{
		DebugDraw = FSenseSysDebugDraw(false);
		DebugDraw.Sensor_DebugTest = true;
	}

	if (DebugDraw.Sensor_DebugTest)
	{
		const FLinearColor Color = FLinearColor::Yellow;
		for (const auto Prim : TouchCollisions)
		{
			if (Prim != nullptr)
			{
				const FTransform T = Prim->GetComponentTransform();
				const FVector Loc = T.GetLocation();
				const FQuat Q = T.GetRotation();

				const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Prim);
				if (Capsule)
				{
					DrawWireCapsule(PDI, Loc, Q.GetForwardVector(), Q.GetRightVector(), Q.GetUpVector(), Color, Capsule->GetScaledCapsuleRadius() + 1.f, Capsule->GetScaledCapsuleHalfHeight() + 1.f, 8, SDPG_World);
					continue;
				}
				const UBoxComponent* Box = Cast<UBoxComponent>(Prim);
				if (Box)
				{
					DrawOrientedWireBox(
						PDI,
						Loc,
						Q.GetForwardVector(),
						Q.GetRightVector(),
						Q.GetUpVector(),
						Box->GetScaledBoxExtent() + 1.f,
						Color,
						SDPG_World);
					continue;
				}
				const USphereComponent* Sphere = Cast<USphereComponent>(Prim);
				if (Sphere)
				{
					DrawWireSphere(PDI, T, Color, Sphere->GetScaledSphereRadius() + 1.f, 8, SDPG_World);
				}
			}
		}
	}
	Super::DrawSensor(View, PDI);
}

void USensorTouch::DrawSensorHUD(const class FViewport* Viewport, const class FSceneView* View, class FCanvas* Canvas) const
{
	Super::DrawSensorHUD(Viewport, View, Canvas);
}

#endif

/************************************/

void USensorTouch::SetTouchCollision(UPrimitiveComponent* Prim)
{
	if (IsValid(Prim))
	{
		//UnBindHitEvent(Prim);
		FScopeLock Lock_CriticalSection(&SensorCriticalSection);
		RemoveTouchCollisions(TouchCollisions);
		TouchCollisions.Empty();
		AddTouchCollision(Prim);
		TouchCollisions.Shrink();
	}
}

void USensorTouch::SetTouchCollisions(TArray<UPrimitiveComponent*> InTouchCollisions)
{
	//UnBindHitEvent(TouchCollisions);
	FScopeLock Lock_CriticalSection(&SensorCriticalSection);
	RemoveTouchCollisions(TouchCollisions);
	TouchCollisions.Reset();
	AddTouchCollisions(InTouchCollisions);
	TouchCollisions.Shrink();
}

/************************************/

void USensorTouch::AddTouchCollision(UPrimitiveComponent* InTouchCollision)
{
	if (IsValid(InTouchCollision))
	{
		TouchCollisions.AddUnique(InTouchCollision);
		BindHitEvent(InTouchCollision);
	}
}
void USensorTouch::RemoveTouchCollision(UPrimitiveComponent* InTouchCollision)
{
	if (InTouchCollision != nullptr)
	{
		UnBindHitEvent(InTouchCollision);
		TouchCollisions.Remove(InTouchCollision);
	}
}

void USensorTouch::AddTouchCollisions(TArray<UPrimitiveComponent*> InTouchCollisions)
{
	for (auto Prim : InTouchCollisions)
	{
		if (IsValid(Prim))
		{
			AddTouchCollision(Prim);
		}
	}
}
void USensorTouch::RemoveTouchCollisions(TArray<UPrimitiveComponent*> InTouchCollisions)
{
	for (auto Prim : InTouchCollisions)
	{
		if (Prim != nullptr)
		{
			RemoveTouchCollision(Prim);
		}
	}
}

/************************************/

void USensorTouch::OnTouchHit(
	UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (IsValidForTest_Short() && bEnable && GetSenseReceiverComponent()->bEnableSenseReceiver)
	{
		USenseStimulusBase* SenseStimulus = USenseSystemBPLibrary::GetStimulusFromActor(OtherActor);
		if (SenseStimulus)
		{
			ReportPassiveEvent(SenseStimulus);
		}
	}
}

void USensorTouch::OnBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (IsValidForTest_Short() && bEnable && GetSenseReceiverComponent()->bEnableSenseReceiver)
	{
		USenseStimulusBase* SenseStimulus = USenseSystemBPLibrary::GetStimulusFromActor(OtherActor);
		if (SenseStimulus)
		{
			ReportPassiveEvent(SenseStimulus);
		}
	}
}

void USensorTouch::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsValidForTest_Short() && bEnable && GetSenseReceiverComponent()->bEnableSenseReceiver)
	{
		USenseStimulusBase* SenseStimulus = USenseSystemBPLibrary::GetStimulusFromActor(OtherActor);
		if (SenseStimulus)
		{
		}
	}
}

/************************************/

void USensorTouch::BindHitEvent(UPrimitiveComponent* InTouchCollision)
{
	if (IsValid(InTouchCollision))
	{
		if (bOnComponentHit)
		{
			InTouchCollision->OnComponentHit.AddUniqueDynamic(this, &USensorTouch::OnTouchHit);
		}
		if (bOnComponentBeginOverlap)
		{
			InTouchCollision->OnComponentBeginOverlap.AddUniqueDynamic(this, &USensorTouch::OnBeginOverlap);
		}
		//InTouchCollision->OnComponentEndOverlap.AddUniqueDynamic(this, &USensorTouch::OnEndOverlap);
	}
}
void USensorTouch::BindHitEvent(TArray<UPrimitiveComponent*> InTouchCollisions)
{
	for (auto Prim : InTouchCollisions)
	{
		if (IsValid(Prim))
		{
			BindHitEvent(Prim);
		}
	}
}
void USensorTouch::UnBindHitEvent(UPrimitiveComponent* InTouchCollision)
{
	if (IsValid(InTouchCollision))
	{
		InTouchCollision->OnComponentHit.RemoveDynamic(this, &USensorTouch::OnTouchHit);
		InTouchCollision->OnComponentBeginOverlap.RemoveDynamic(this, &USensorTouch::OnBeginOverlap);
		InTouchCollision->OnComponentEndOverlap.RemoveDynamic(this, &USensorTouch::OnEndOverlap);
	}
}

void USensorTouch::UnBindHitEvent(TArray<UPrimitiveComponent*> InTouchCollisions)
{
	for (auto Prim : InTouchCollisions)
	{
		if (Prim) UnBindHitEvent(Prim);
	}
}

/************************************/

void USensorTouch::InitializeFromReceiver(USenseReceiverComponent* InSenseReceiver)
{
	Super::InitializeFromReceiver(InSenseReceiver);
	if (GetSenseReceiverComponent() != nullptr && GetSenseReceiverComponent()->GetOwner() != nullptr && bActorRootCollision)
	{
		const auto Root = GetSenseReceiverComponent()->GetOwner()->GetRootComponent();
		if (Root)
		{
			const auto Collision = Cast<UPrimitiveComponent>(Root);
			if (Collision)
			{
				SetTouchCollision(Collision);
			}
		}
	}
}

/*
void USensorTouch::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (SensorTests.Num() > 0)
	{
		auto DSO = Cast<USensorBoolTest>(GetDefaultSubobjectByName(TEXT("BoolTest")));
		if (SensorTests[0])
		{
			if (SensorTests[0] != DSO)  // ...if they don't match, the saved state needs to be fixed up.
			{
				SensorTests[0]->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
				SensorTests[0] = DSO;
			}
		}
		else
		{
			SensorTests[0] = DSO;
		}

	}
}
*/
