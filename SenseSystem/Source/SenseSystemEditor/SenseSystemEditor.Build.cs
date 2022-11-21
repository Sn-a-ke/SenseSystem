//Copyright 2020 Alexandr Marchenko. All Rights Reserved. 

using UnrealBuildTool;

public class SenseSystemEditor : ModuleRules
{
    public SenseSystemEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrecompileForTargets = PrecompileTargetsType.Any;

        PublicIncludePaths.AddRange(new string[]
            {
                ModuleDirectory + "/Public"
            });


        PrivateIncludePaths.AddRange(new string[]
            {
                "SenseSystemEditor/Private",
            });

        PublicDependencyModuleNames.AddRange(new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
            });

        PrivateDependencyModuleNames.AddRange(new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "EditorStyle",
                "Slate",
                "SlateCore",
                "PropertyEditor",
                "InputCore",
                "ComponentVisualizers",
                "SenseSystem",
            });

        //PrivateIncludePathModuleNames.AddRange(new string[]{});
        //DynamicallyLoadedModuleNames.AddRange(new string[]{});
    }
}
