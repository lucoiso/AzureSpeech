// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEAzSpeech

#include "AzSpeech/SSMLToStreamAsync.h"
#include "AzSpeechInternalFuncs.h"
#include "Async/Async.h"

USSMLToStreamAsync* USSMLToStreamAsync::SSMLToStream(const UObject* WorldContextObject, const FString& SSMLString)
{
	USSMLToStreamAsync* const NewAsyncTask = NewObject<USSMLToStreamAsync>();
	NewAsyncTask->WorldContextObject = WorldContextObject;
	NewAsyncTask->SynthesisText = SSMLString;
	NewAsyncTask->bIsSSMLBased = true;

	return NewAsyncTask;
}

bool USSMLToStreamAsync::StartAzureTaskWork_Internal()
{
	if (!Super::StartAzureTaskWork_Internal())
	{
		return false;
	}

	if (HasEmptyParam(SynthesisText))
	{
		return false;
	}

	const auto AudioConfig = Microsoft::CognitiveServices::Speech::Audio::AudioConfig::FromStreamOutput(Microsoft::CognitiveServices::Speech::Audio::AudioOutputStream::CreatePullStream());
	if (!InitializeSynthesizer(AudioConfig))
	{
		return false;
	}

	StartSynthesisWork();

	return true;
}

void USSMLToStreamAsync::OnSynthesisUpdate(const Microsoft::CognitiveServices::Speech::SpeechSynthesisEventArgs& SynthesisEventArgs)
{
	Super::OnSynthesisUpdate(SynthesisEventArgs);

	if (!UAzSpeechTaskBase::IsTaskStillValid(this))
	{
		return;
	}

	if (CanBroadcastWithReason(SynthesisEventArgs.Result->Reason))
	{
		const TArray<uint8> OutputStream = GetLastSynthesizedStream();

#if ENGINE_MAJOR_VERSION >= 5
		OutputSynthesisResult(!OutputStream.IsEmpty());
#else
		OutputSynthesisResult(OutputStream.Num() != 0);
#endif

		AsyncTask(ENamedThreads::GameThread, [=] { SynthesisCompleted.Broadcast(OutputStream); });
	}
}