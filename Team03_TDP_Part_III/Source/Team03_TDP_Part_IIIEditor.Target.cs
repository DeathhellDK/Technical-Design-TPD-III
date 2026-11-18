// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class Team03_TDP_Part_IIIEditorTarget : TargetRules
{
	public Team03_TDP_Part_IIIEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;

		ExtraModuleNames.AddRange( new string[] { "Team03_TDP_Part_III" } );
	}
}
