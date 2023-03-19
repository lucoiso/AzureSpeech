// Author: Lucas Vilas-Boas
// Year: 2023
// Repo: https://github.com/lucoiso/UEAzSpeech

#include "AzSpeech/Tasks/TextToSpeechAsync.h"

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(TextToSpeechAsync)
#endif

UTextToSpeechAsync* UTextToSpeechAsync::TextToSpeech_DefaultOptions(UObject* WorldContextObject, const FString& SynthesisText, const FString& VoiceName, const FString& LanguageID)
{
	return TextToSpeech_CustomOptions(WorldContextObject, SynthesisText, FAzSpeechSettingsOptions(*LanguageID, *VoiceName));
}

UTextToSpeechAsync* UTextToSpeechAsync::TextToSpeech_CustomOptions(UObject* WorldContextObject, const FString& SynthesisText, const FAzSpeechSettingsOptions& Options)
{
	UTextToSpeechAsync* const NewAsyncTask = NewObject<UTextToSpeechAsync>();
	NewAsyncTask->WorldContextObject = WorldContextObject;
	NewAsyncTask->SynthesisText = SynthesisText;
	NewAsyncTask->TaskOptions = GetValidatedOptions(Options);
	NewAsyncTask->bIsSSMLBased = false;
	NewAsyncTask->TaskName = *FString(__func__);
	NewAsyncTask->RegisterWithGameInstance(WorldContextObject);

	return NewAsyncTask;
}