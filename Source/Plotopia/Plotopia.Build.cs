// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Plotopia : ModuleRules
{
	public Plotopia(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "NetCore", "StructUtils", "CoreUObject", "Engine", "InputCore", "EnhancedInput" ,"GameplayAbilities","GameplayTasks","GameplayTags","UMG"});

		PrivateDependencyModuleNames.AddRange(new string[] { "AIModule", "Slate", "SlateCore"});

		// 编辑器专用依赖：仅编辑器构建时链接（蓝图诊断/清理命令let使用）
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "BlueprintGraph", "Kismet", "UMGEditor" });
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
