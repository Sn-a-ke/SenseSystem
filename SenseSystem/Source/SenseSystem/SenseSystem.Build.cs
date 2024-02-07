//Copyright 2020 Alexandr Marchenko. All Rights Reserved. 

using UnrealBuildTool;

public class SenseSystem : ModuleRules
{
    public SenseSystem(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrecompileForTargets = PrecompileTargetsType.Any;

        PublicIncludePaths.AddRange(new string[]
            {
				ModuleDirectory + "/Public",
                ModuleDirectory + "/Public/Sensors",
                ModuleDirectory + "/Public/Sensors/Tests",
            });


        PrivateIncludePaths.AddRange(new string[]
            {
                "SenseSystem/Private",
                "SenseSystem/Private/Sensors",
                "SenseSystem/Private/Sensors/Tests",
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
			});


        DynamicallyLoadedModuleNames.AddRange(new string[]
            {
			});

    }
}

/*
"WhitelistPlatforms":
[
    "Win32",
    "Win64",
    "HoloLens",
    "Mac",
    "XboxOne",
    "PS4",
    "IOS",
    "Android",
    "Linux",
    "LinuxAArch64",
    "AllDesktop",
    "TVOS",
    "Switch",
    "Quail",
    "Lumin"
]
*/