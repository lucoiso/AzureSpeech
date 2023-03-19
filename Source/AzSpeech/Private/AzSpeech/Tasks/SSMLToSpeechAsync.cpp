// Author: Lucas Vilas-Boas
// Year: 2023
// Repo: https://github.com/lucoiso/UEAzSpeech

#include "AzSpeech/Tasks/SSMLToSpeechAsync.h"

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(SSMLToSpeechAsync)
#endif

USSMLToSpeechAsync* USSMLToSpeechAsync::SSMLToSpeech_DefaultOptions(UObject* WorldContextObject, const FString& SynthesisSSML)
{
	return SSMLToSpeech_CustomOptions(WorldContextObject, SynthesisSSML, FAzSpeechSettingsOptions());
}

USSMLToSpeechAsync* USSMLToSpeechAsync::SSMLToSpeech_CustomOptions(UObject* WorldContextObject, const FString& SynthesisSSML, const FAzSpeechSettingsOptions& Options)
{
	USSMLToSpeechAsync* const NewAsyncTask = NewObject<USSMLToSpeechAsync>();
	NewAsyncTask->WorldContextObject = WorldContextObject;
	NewAsyncTask->TaskOptions = GetValidatedOptions(Options);
	NewAsyncTask->SynthesisText = SynthesisSSML;
	NewAsyncTask->bIsSSMLBased = true;
	NewAsyncTask->TaskName = *FString(__func__);
	NewAsyncTask->RegisterWithGameInstance(WorldContextObject);

	return NewAsyncTask;
}