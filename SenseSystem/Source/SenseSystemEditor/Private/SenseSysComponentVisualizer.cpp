//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "SenseSysComponentVisualizer.h"
#include "Components/ActorComponent.h"
#include "SceneView.h"
#include "SceneManagement.h"
#include "SenseReceiverComponent.h"
#include "SenseStimulusBase.h"
#include "CanvasTypes.h"
#include "UnrealClient.h"

void FSenseSysComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
#if WITH_EDITORONLY_DATA
	if (View->Family->EngineShowFlags.VisualizeSenses)
	{

		if (const auto Senses = Cast<const USenseReceiverComponent>(Component))
		{
			Senses->DrawComponent(View, PDI);
		}
		else if (const auto Stimulus = Cast<const USenseStimulusBase>(Component))
		{
			Stimulus->DrawComponent(View, PDI);
		}
	}
#endif
}

void FSenseSysComponentVisualizer::DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
#if WITH_EDITORONLY_DATA
	if (View->Family->EngineShowFlags.VisualizeSenses)
	{

		if (const auto Senses = Cast<const USenseReceiverComponent>(Component))
		{
			Senses->DrawComponentHUD(Viewport, View, Canvas);
		}
		else if (const auto Stimulus = Cast<const USenseStimulusBase>(Component))
		{
			Stimulus->DrawComponentHUD(Viewport, View, Canvas);
		}
	}
#endif
}
