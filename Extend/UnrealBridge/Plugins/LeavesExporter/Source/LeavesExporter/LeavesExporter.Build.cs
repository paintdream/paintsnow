// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class LeavesExporter : ModuleRules
	{
		private string ThirdPartyPath
		{
			get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../../../../Build/")); }
		}

		public LeavesExporter(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivatePCHHeaderFile = "Private/LeavesExporterPCH.h";
			MinFilesUsingPrecompiledHeaderOverride = 1;
			bFasterWithoutUnity = true;
			PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"Developer/LeavesExporter/Private",
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core", "CoreUObject", "Engine", "InputCore", "SlateCore", "Slate", "UnrealEd",
					"LevelEditor", "EditorStyle", "Projects", "SceneOutliner", "Foliage", "Landscape"
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
				}
				);


			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);

			LoadPaintsNow(Target);
		}

		// from: https://answers.unrealengine.com/questions/76792/link-to-3rd-party-libraries.html
		public bool LoadPaintsNow(ReadOnlyTargetRules Target)
		{
			bool isLibrarySupported = false;

			/////where to pick the library if we're building for windows (32 or 64)
			if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
			{
				isLibrarySupported = true;
				string LibraryPath = ThirdPartyPath;

				UnrealTargetConfiguration configure = Target.Configuration;
				/*
				if (configure == UnrealTargetConfiguration.DebugGame)
				{
					LibraryPath = Path.Combine(LibraryPath, "Debug");
				}
				else
				{
					LibraryPath = Path.Combine(LibraryPath, "Release");
				}*/
				LibraryPath = Path.Combine(LibraryPath, "Release");

				PublicLibraryPaths.Add(LibraryPath);
				PublicAdditionalLibraries.Add("LibEvent.lib");
				PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "PaintsNow.lib"));
				PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "BridgeSunset.lib"));
				PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "GalaxyWeaver.lib"));
				PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "MythForest.lib"));
				PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "SnowyStream.lib"));
				// PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "PaintsNow", "include"));
			}

			// and here we go.
			// Definitions.Add(string.Format("WITH_SIXENSE_BINDING={0}", isLibrarySupported ? 1 : 0));
			return isLibrarySupported;
		}
	}
}
