//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

class FPrimitiveDrawInterface;
class FSceneView;

/**
 * 
 */
class SENSESYSTEMEDITOR_API FSenseSysComponentVisualizer : public FComponentVisualizer
{
public:
	//~ Begin FComponentVisualizer Interface

	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	/** Draw HUD on viewport for the supplied component */
	virtual void DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;
	//~ End FComponentVisualizer Interface
};
