// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEAzSpeech

using System.IO;
using UnrealBuildTool;

public class AzureWrapper : ModuleRules
{
	public AzureWrapper(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		bEnableExceptions = true;

		PublicIncludePaths.AddRange(new[]
		{
			Path.Combine(ModuleDirectory, "include", "c_api"),
			Path.Combine(ModuleDirectory, "include", "cxx_api")
		});

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "libs", "Win", "Microsoft.CognitiveServices.Speech.core.lib"));

			PublicDelayLoadDLLs.Add("Microsoft.CognitiveServices.Speech.core.dll");
			PublicDelayLoadDLLs.Add("Microsoft.CognitiveServices.Speech.extension.audio.sys.dll");
			PublicDelayLoadDLLs.Add("Microsoft.CognitiveServices.Speech.extension.kws.dll");
			PublicDelayLoadDLLs.Add("Microsoft.CognitiveServices.Speech.extension.lu.dll");
			PublicDelayLoadDLLs.Add("Microsoft.CognitiveServices.Speech.extension.mas.dll");
			PublicDelayLoadDLLs.Add("Microsoft.CognitiveServices.Speech.extension.codec.dll");

			RuntimeDependencies.Add(Path.Combine(ModuleDirectory, "libs", "Win", "Runtime", "Microsoft.CognitiveServices.Speech.core.dll"));
			RuntimeDependencies.Add(Path.Combine(ModuleDirectory, "libs", "Win", "Runtime", "Microsoft.CognitiveServices.Speech.extension.audio.sys.dll"));
			RuntimeDependencies.Add(Path.Combine(ModuleDirectory, "libs", "Win", "Runtime", "Microsoft.CognitiveServices.Speech.extension.kws.dll"));
			RuntimeDependencies.Add(Path.Combine(ModuleDirectory, "libs", "Win", "Runtime", "Microsoft.CognitiveServices.Speech.extension.lu.dll"));
			RuntimeDependencies.Add(Path.Combine(ModuleDirectory, "libs", "Win", "Runtime", "Microsoft.CognitiveServices.Speech.extension.mas.dll"));
			RuntimeDependencies.Add(Path.Combine(ModuleDirectory, "libs", "Win", "Runtime", "Microsoft.CognitiveServices.Speech.extension.codec.dll"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "AzSpeech_UPL_Android.xml"));

			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "libs", "Android", "libMicrosoft.CognitiveServices.Speech.core.so"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "libs", "Android", "libMicrosoft.CognitiveServices.Speech.extension.audio.sys.so"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "libs", "Android", "libMicrosoft.CognitiveServices.Speech.extension.kws.so"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "libs", "Android", "libMicrosoft.CognitiveServices.Speech.extension.lu.so"));
		}
	}
}