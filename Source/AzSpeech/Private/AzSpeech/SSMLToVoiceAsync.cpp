// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEAzSpeech

#include "AzSpeech/SSMLToVoiceAsync.h"
#include "AzSpeech.h"
#include "Async/Async.h"
#include "AzSpeech/AzSpeechHelper.h"
#include "AzSpeechInternalFuncs.h"

namespace AzSpeechWrapper
{
	namespace Standard_Cpp
	{
		static bool DoSSMLToVoiceWork(const std::string& InSSML)
		{
			const auto& SpeechSynthesizer = AzSpeech::Internal::GetAzureSynthesizer();

			if (const auto& SpeechSynthesisResult = SpeechSynthesizer->SpeakSsmlAsync(InSSML).get();
				SpeechSynthesisResult->Reason == ResultReason::SynthesizingAudioCompleted)
			{
				UE_LOG(LogAzSpeech, Display,
					   TEXT("AzSpeech - %s: Speech Synthesis task completed"), *FString(__func__));

				return true;
			}

			UE_LOG(LogAzSpeech, Error, TEXT("AzSpeech - %s: Speech Synthesis task failed"), *FString(__func__));
			return false;
		}
	}

	namespace Unreal_Cpp
	{
		static void AsyncSSMLToVoice(const FString& InSSML, FSSMLToVoiceDelegate Delegate)
		{
			if (InSSML.IsEmpty())
			{
				UE_LOG(LogAzSpeech, Error, TEXT("AzSpeech - %s: SSML is empty"), *FString(__func__));
				return;
			}

			UE_LOG(LogAzSpeech, Display, TEXT("AzSpeech - %s: Initializing task"), *FString(__func__));

			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [InSSML, Delegate]
					  {
						  const TFuture<bool>& SSMLToVoiceAsyncWork =
							  Async(EAsyncExecution::Thread, [InSSML]() -> bool
									{
										const std::string& InSSMLStr = TCHAR_TO_UTF8(*InSSML);

										return Standard_Cpp::DoSSMLToVoiceWork(InSSMLStr);
									});

						  SSMLToVoiceAsyncWork.WaitFor(FTimespan::FromSeconds(5));
						  const bool& bOutputValue = SSMLToVoiceAsyncWork.Get();

						  AsyncTask(ENamedThreads::GameThread, [bOutputValue, Delegate]
						  {
							  Delegate.Broadcast(bOutputValue);
						  });

						  if (bOutputValue)
						  {
							  UE_LOG(LogAzSpeech, Display, TEXT("AzSpeech - AsyncSSMLToVoice: Result: Success"));
						  }
						  else
						  {
							  UE_LOG(LogAzSpeech, Error, TEXT("AzSpeech - AsyncSSMLToVoice: Result: Error"));
						  }
					  });
		}
	}
}

USSMLToVoiceAsync* USSMLToVoiceAsync::SSMLToVoice(const UObject* WorldContextObject, const FString& SSMLString)
{
	USSMLToVoiceAsync* SSMLToVoiceAsync = NewObject<USSMLToVoiceAsync>();
	SSMLToVoiceAsync->WorldContextObject = WorldContextObject;
	SSMLToVoiceAsync->SSMLString = SSMLString;

	return SSMLToVoiceAsync;
}

void USSMLToVoiceAsync::Activate()
{
	AzSpeechWrapper::Unreal_Cpp::AsyncSSMLToVoice(SSMLString, TaskCompleted);
}
