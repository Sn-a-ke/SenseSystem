//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "SenseSystemEditor.h"
#include "BitFlag64_Customization.h"
#include "SenseSysComponentVisualizer.h"
#include "SenseReceiverComponent.h"
#include "SenseStimulusComponent.h"
#include "SenseSysSettings.h"

#include "Templates/SharedPointer.h"
#include "ISettingsModule.h"
#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "EditorModes.h"


#define LOCTEXT_NAMESPACE "FSenseSystemEditorModule"

void FSenseSystemEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		"BitFlag64_SenseSys",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FBitFlag64_Customization::MakeInstance));

	RegisterComponentVisualizer(USenseReceiverComponent::StaticClass()->GetFName(), MakeShareable(new FSenseSysComponentVisualizer()));
	RegisterComponentVisualizer(USenseStimulusComponent::StaticClass()->GetFName(), MakeShareable(new FSenseSysComponentVisualizer()));

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings(
			"Project",
			"Plugins",
			"SenseSystem",
			LOCTEXT("RuntimeSettingsName", "SenseSystem"),
			LOCTEXT("RuntimeSettingsDescription", "Configure the SenseSystem plugin"),
			GetMutableDefault<USenseSysSettings>());
	}
}

void FSenseSystemEditorModule::ShutdownModule()
{
	if (GUnrealEd)
	{
		for (const FName ClassName : RegisteredComponentClassNames)
		{
			GUnrealEd->UnregisterComponentVisualizer(ClassName);
		}
	}
}

void FSenseSystemEditorModule::RegisterComponentVisualizer(const FName ComponentClassName, TSharedPtr<class FComponentVisualizer> Visualizer)
{
	if (GUnrealEd)
	{
		GUnrealEd->RegisterComponentVisualizer(ComponentClassName, Visualizer);
	}

	RegisteredComponentClassNames.Add(ComponentClassName);

	if (Visualizer.IsValid())
	{
		Visualizer->OnRegister();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSenseSystemEditorModule, SenseSystemEditor);
