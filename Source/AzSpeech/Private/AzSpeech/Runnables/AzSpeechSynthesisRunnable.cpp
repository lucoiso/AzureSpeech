// Author: Lucas Vilas-Boas
// Year: 2023
// Repo: https://github.com/lucoiso/UEAzSpeech

#include "AzSpeech/Runnables/AzSpeechSynthesisRunnable.h"
#include "AzSpeech/Tasks/Bases/AzSpeechSynthesizerTaskBase.h"
#include "AzSpeech/AzSpeechSettings.h"
#include "LogAzSpeech.h"
#include <Async/Async.h>

FAzSpeechSynthesisRunnable::FAzSpeechSynthesisRunnable(UAzSpeechTaskBase* InOwningTask, const std::shared_ptr<Microsoft::CognitiveServices::Speech::Audio::AudioConfig>& InAudioConfig) : Super(InOwningTask, InAudioConfig)
{
}

uint32 FAzSpeechSynthesisRunnable::Run()
{
	FScopeLock Lock(&Mutex);

#if !UE_BUILD_SHIPPING
	const int64 StartTime = GetTimeInMilliseconds();
#endif
	
	if (Super::Run() == 0u)
	{
		UE_LOG(LogAzSpeech_Internal, Error, TEXT("Thread: %s; Function: %s; Message: Run returned 0"), *GetThreadName(), *FString(__func__));
		return 0u;
	}
	
	if (!IsSpeechSynthesizerValid())
	{
		return 0u;
	}

	UAzSpeechSynthesizerTaskBase* const SynthesizerTask = GetOwningSynthesizerTask();

	if (!UAzSpeechTaskStatus::IsTaskStillValid(SynthesizerTask))
	{
		return 0u;
	}

	UE_LOG(LogAzSpeech_Debugging, Display, TEXT("Thread: %s; Function: %s; Message: Using text: %s"), *GetThreadName(), *FString(__func__), *SynthesizerTask->GetSynthesisText());

	const std::string SynthesisStr = TCHAR_TO_UTF8(*SynthesizerTask->GetSynthesisText());
	std::future<std::shared_ptr<Microsoft::CognitiveServices::Speech::SpeechSynthesisResult>> Future;
	if (SynthesizerTask->IsSSMLBased())
	{
		Future = SpeechSynthesizer->StartSpeakingSsmlAsync(SynthesisStr);
	}
	else
	{
		Future = SpeechSynthesizer->StartSpeakingTextAsync(SynthesisStr);
	}

	UE_LOG(LogAzSpeech_Internal, Display, TEXT("Thread: %s; Function: %s; Message: Starting synthesis."), *GetThreadName(), *FString(__func__));
	if (Future.wait_for(GetTaskTimeout()); Future.valid())
	{
		UE_LOG(LogAzSpeech_Internal, Display, TEXT("Thread: %s; Function: %s; Message: Synthesis started."), *GetThreadName(), *FString(__func__));
	}
	else
	{
		UE_LOG(LogAzSpeech_Internal, Error, TEXT("Thread: %s; Function: %s; Message: Synthesis failed to start."), *GetThreadName(), *FString(__func__));
		AsyncTask(ENamedThreads::GameThread, [SynthesizerTask] { SynthesizerTask->SynthesisFailed.Broadcast(); });
		return 0u;
	}
	
#if !UE_BUILD_SHIPPING
	const int64 ActivationDelay = GetTimeInMilliseconds() - StartTime;
#endif

	const float SleepTime = GetThreadUpdateInterval();
	
	while (!IsPendingStop())
	{
#if !UE_BUILD_SHIPPING
		PrintDebugInformation(StartTime, ActivationDelay, SleepTime);
#endif
		FPlatformProcess::Sleep(SleepTime);
	}

	return 1u;
}

void FAzSpeechSynthesisRunnable::Exit()
{
	FScopeLock Lock(&Mutex);

	Super::Exit();
	
	if (SpeechSynthesizer)
	{
		SpeechSynthesizer->StopSpeakingAsync().wait_for(GetTaskTimeout());
	}

	SpeechSynthesizer = nullptr;
}

const bool FAzSpeechSynthesisRunnable::IsSpeechSynthesizerValid() const
{
	if (!SpeechSynthesizer)
	{
		UE_LOG(LogAzSpeech_Internal, Error, TEXT("Thread: %s; Function: %s; Message: Invalid synthesizer"), *GetThreadName(), *FString(__func__));
	}

	return SpeechSynthesizer != nullptr;
}

UAzSpeechSynthesizerTaskBase* FAzSpeechSynthesisRunnable::GetOwningSynthesizerTask() const
{
	if (!GetOwningTask())
	{
		return nullptr;
	}

	return Cast<UAzSpeechSynthesizerTaskBase>(GetOwningTask());
}

void FAzSpeechSynthesisRunnable::ClearSignals()
{
	FScopeLock Lock(&Mutex);

	Super::ClearSignals();
	
	if (!IsSpeechSynthesizerValid())
	{
		return;
	}

	SignalDisconnecter_T(SpeechSynthesizer->VisemeReceived);
	SignalDisconnecter_T(SpeechSynthesizer->Synthesizing);
	SignalDisconnecter_T(SpeechSynthesizer->SynthesisStarted);
	SignalDisconnecter_T(SpeechSynthesizer->SynthesisCompleted);
	SignalDisconnecter_T(SpeechSynthesizer->SynthesisCanceled);
}

void FAzSpeechSynthesisRunnable::RemoveBindings()
{
	FScopeLock Lock(&Mutex);

	Super::RemoveBindings();

	UAzSpeechSynthesizerTaskBase* const SynthesizerTask = GetOwningSynthesizerTask();

	if (!UAzSpeechTaskStatus::IsTaskStillValid(SynthesizerTask))
	{
		return;
	}

	DelegateDisconnecter_T(SynthesizerTask->VisemeReceived);
	DelegateDisconnecter_T(SynthesizerTask->SynthesisUpdated);
	DelegateDisconnecter_T(SynthesizerTask->SynthesisStarted);
}

const bool FAzSpeechSynthesisRunnable::ApplySDKSettings(const std::shared_ptr<Microsoft::CognitiveServices::Speech::SpeechConfig>& InConfig) const
{
	FScopeLock Lock(&Mutex);

	if (!Super::ApplySDKSettings(InConfig))
	{
		return false;
	}

	UAzSpeechSynthesizerTaskBase* const SynthesizerTask = GetOwningSynthesizerTask();

	if (!UAzSpeechTaskStatus::IsTaskStillValid(SynthesizerTask))
	{
		return false;
	}

	InConfig->SetProperty("SpeechSynthesis_KeepConnectionAfterStopping", "false");
	InConfig->SetSpeechSynthesisOutputFormat(GetOutputFormat());

	if (SynthesizerTask->IsUsingAutoLanguage())
	{
		return true;
	}

	const std::string UsedLang = TCHAR_TO_UTF8(*SynthesizerTask->GetLanguageID());
	const std::string UsedVoice = TCHAR_TO_UTF8(*SynthesizerTask->GetVoiceName());

	UE_LOG(LogAzSpeech_Internal, Display, TEXT("Thread: %s; Function: %s; Message: Using language: %s"), *GetThreadName(), *FString(__func__), *SynthesizerTask->GetLanguageID());
	InConfig->SetSpeechSynthesisLanguage(UsedLang);

	UE_LOG(LogAzSpeech_Internal, Display, TEXT("Thread: %s; Function: %s; Message: Using voice: %s"), *GetThreadName(), *FString(__func__), *SynthesizerTask->GetVoiceName());
	InConfig->SetSpeechSynthesisVoiceName(UsedVoice);

	return true;
}

bool FAzSpeechSynthesisRunnable::InitializeAzureObject()
{
	FScopeLock Lock(&Mutex);

	if (!Super::InitializeAzureObject())
	{
		return false;
	}
	
	UAzSpeechSynthesizerTaskBase* const SynthesizerTask = GetOwningSynthesizerTask();

	if (!UAzSpeechTaskStatus::IsTaskStillValid(SynthesizerTask))
	{
		return false;
	}
	
	UE_LOG(LogAzSpeech_Internal, Display, TEXT("Thread: %s; Function: %s; Message: Creating synthesizer object"), *GetThreadName(), *FString(__func__));

	const auto SpeechConfig = CreateSpeechConfig();

	if (!SpeechConfig)
	{
		UE_LOG(LogAzSpeech_Internal, Error, TEXT("Thread: %s; Function: %s; Message: Invalid speech config"), *GetThreadName(), *FString(__func__));	
		return false;
	}
	
	ApplySDKSettings(SpeechConfig);

	if (SynthesizerTask->IsUsingAutoLanguage())
	{
		UE_LOG(LogAzSpeech_Internal, Display, TEXT("Thread: %s; Function: %s; Message: Initializing auto language detection"), *GetThreadName(), *FString(__func__));

		SpeechSynthesizer = Microsoft::CognitiveServices::Speech::SpeechSynthesizer::FromConfig(SpeechConfig, Microsoft::CognitiveServices::Speech::AutoDetectSourceLanguageConfig::FromOpenRange(), GetAudioConfig());
	}
	else
	{
		SpeechSynthesizer = Microsoft::CognitiveServices::Speech::SpeechSynthesizer::FromConfig(SpeechConfig, GetAudioConfig());
	}

	return ConnectVisemeSignal() && ConnectSynthesisStartedSignal() && ConnectSynthesisUpdateSignals();
}

bool FAzSpeechSynthesisRunnable::ConnectVisemeSignal()
{
	FScopeLock Lock(&Mutex);

	if (!UAzSpeechSettings::Get()->bEnableViseme)
	{
		return true;
	}

	UAzSpeechSynthesizerTaskBase* const SynthesizerTask = GetOwningSynthesizerTask();
	if (!IsSpeechSynthesizerValid() || !UAzSpeechTaskStatus::IsTaskStillValid(SynthesizerTask))
	{
		return false;
	}

	bFilterVisemeData = SynthesizerTask->bIsSSMLBased && UAzSpeechSettings::Get()->bFilterVisemeFacialExpression && SynthesizerTask->SynthesisText.Contains("<mstts:viseme type=\"FacialExpression\"/>", ESearchCase::IgnoreCase);

	SpeechSynthesizer->VisemeReceived.Connect([this, SynthesizerTask](const Microsoft::CognitiveServices::Speech::SpeechSynthesisVisemeEventArgs& VisemeEventArgs)
	{
		if (!UAzSpeechTaskStatus::IsTaskStillValid(SynthesizerTask))
		{
			StopAzSpeechRunnableTask();
			return;
		}

		if (bFilterVisemeData && VisemeEventArgs.Animation.empty())
		{
			return;
		}

		FAzSpeechVisemeData LastVisemeData;
		LastVisemeData.VisemeID = VisemeEventArgs.VisemeId;
		LastVisemeData.AudioOffsetMilliseconds = VisemeEventArgs.AudioOffset / 10000;
		LastVisemeData.Animation = UTF8_TO_TCHAR(VisemeEventArgs.Animation.c_str());

		AsyncTask(ENamedThreads::GameThread, [SynthesizerTask, LastVisemeData] { SynthesizerTask->OnVisemeReceived(LastVisemeData); });
	});

	return true;
}

bool FAzSpeechSynthesisRunnable::ConnectSynthesisStartedSignal()
{
	FScopeLock Lock(&Mutex);

	UAzSpeechSynthesizerTaskBase* const SynthesizerTask = GetOwningSynthesizerTask();
	if (!IsSpeechSynthesizerValid() || !UAzSpeechTaskStatus::IsTaskStillValid(SynthesizerTask))
	{
		return false;
	}

	const auto SynthesisStarted_Lambda = [this, SynthesizerTask]([[maybe_unused]] const Microsoft::CognitiveServices::Speech::SpeechSynthesisEventArgs& SynthesisEventArgs)
	{
		if (!UAzSpeechTaskStatus::IsTaskStillValid(SynthesizerTask))
		{
			StopAzSpeechRunnableTask();
			return;
		}

		if (SynthesizerTask->SynthesisStarted.IsBound())
		{
			AsyncTask(ENamedThreads::GameThread, [SynthesizerTask] { SynthesizerTask->SynthesisStarted.Broadcast(); });
		}

		SignalDisconnecter_T(SpeechSynthesizer->SynthesisStarted);
	};

	SpeechSynthesizer->SynthesisStarted.Connect(SynthesisStarted_Lambda);

	return true;
}

bool FAzSpeechSynthesisRunnable::ConnectSynthesisUpdateSignals()
{
	FScopeLock Lock(&Mutex);

	UAzSpeechSynthesizerTaskBase* const SynthesizerTask = GetOwningSynthesizerTask();
	if (!IsSpeechSynthesizerValid() || !UAzSpeechTaskStatus::IsTaskStillValid(SynthesizerTask))
	{
		return false;
	}

	const auto SynthesisUpdate_Lambda = [this, SynthesizerTask](const Microsoft::CognitiveServices::Speech::SpeechSynthesisEventArgs& SynthesisEventArgs)
	{
		const bool bValidResult = ProcessSynthesisResult(SynthesisEventArgs.Result);
		if (!UAzSpeechTaskStatus::IsTaskStillValid(SynthesizerTask) || !bValidResult)
		{
			if (!bValidResult)
			{
				AsyncTask(ENamedThreads::GameThread, [SynthesizerTask] { SynthesizerTask->SynthesisFailed.Broadcast(); });
			}

			StopAzSpeechRunnableTask();
			return;
		}
		
		AsyncTask(ENamedThreads::GameThread, [SynthesizerTask, Result = SynthesisEventArgs.Result] { SynthesizerTask->OnSynthesisUpdate(Result); });
	};

	SpeechSynthesizer->Synthesizing.Connect(SynthesisUpdate_Lambda);
	SpeechSynthesizer->SynthesisCompleted.Connect(SynthesisUpdate_Lambda);
	SpeechSynthesizer->SynthesisCanceled.Connect(SynthesisUpdate_Lambda);	
	
	return true;
}

bool FAzSpeechSynthesisRunnable::ProcessSynthesisResult(const std::shared_ptr<Microsoft::CognitiveServices::Speech::SpeechSynthesisResult>& LastResult)
{
	bool bOutput = true;
	bool bFinishTask = false;

	switch (LastResult->Reason)
	{
		case Microsoft::CognitiveServices::Speech::ResultReason::SynthesizingAudio:
			UE_LOG(LogAzSpeech_Internal, Display, TEXT("Thread: %s; Function: %s; Message: Task running. Reason: SynthesizingAudio"), *GetThreadName(), *FString(__func__));
			bOutput = true;
			bFinishTask = false;
			break;

		case Microsoft::CognitiveServices::Speech::ResultReason::SynthesizingAudioCompleted:
			UE_LOG(LogAzSpeech_Internal, Display, TEXT("Thread: %s; Function: %s; Message: Task completed. Reason: SynthesizingAudioCompleted"), *GetThreadName(), *FString(__func__));
			bOutput = true;
			bFinishTask = true;
			break;
			
		case Microsoft::CognitiveServices::Speech::ResultReason::SynthesizingAudioStarted:
			UE_LOG(LogAzSpeech_Internal, Display, TEXT("Thread: %s; Function: %s; Message: Task started. Reason: SynthesizingAudioStarted"), *GetThreadName(), *FString(__func__));
			bOutput = true;
			bFinishTask = false;
			break;

		default:
			break;
	}

	if (LastResult->Reason == Microsoft::CognitiveServices::Speech::ResultReason::Canceled)
	{
		bOutput = false;
		bFinishTask = true;
		
		UE_LOG(LogAzSpeech_Internal, Error, TEXT("Thread: %s; Function: %s; Message: Task failed. Reason: Canceled"), *GetThreadName(), *FString(__func__));
		
		const auto CancellationDetails = Microsoft::CognitiveServices::Speech::SpeechSynthesisCancellationDetails::FromResult(LastResult);

		UE_LOG(LogAzSpeech_Internal, Error, TEXT("Thread: %s; Function: %s; Message: Cancellation Reason: %s"), *GetThreadName(), *FString(__func__), *CancellationReasonToString(CancellationDetails->Reason));

		if (CancellationDetails->Reason == Microsoft::CognitiveServices::Speech::CancellationReason::Error)
		{
			ProcessCancellationError(CancellationDetails->ErrorCode, CancellationDetails->ErrorDetails);
		}		
	}
	
	if (bFinishTask)
	{
		StopAzSpeechRunnableTask();
	}

	return bOutput;
}

const Microsoft::CognitiveServices::Speech::SpeechSynthesisOutputFormat FAzSpeechSynthesisRunnable::GetOutputFormat() const
{
	if (const UAzSpeechSettings* const Settings = UAzSpeechSettings::Get())
	{
		switch (Settings->SpeechSynthesisOutputFormat)
		{
			case EAzSpeechSynthesisOutputFormat::Riff16Khz16BitMonoPcm:
				return Microsoft::CognitiveServices::Speech::SpeechSynthesisOutputFormat::Riff16Khz16BitMonoPcm;

			case EAzSpeechSynthesisOutputFormat::Riff24Khz16BitMonoPcm:
				return Microsoft::CognitiveServices::Speech::SpeechSynthesisOutputFormat::Riff24Khz16BitMonoPcm;

			case EAzSpeechSynthesisOutputFormat::Riff48Khz16BitMonoPcm:
				return Microsoft::CognitiveServices::Speech::SpeechSynthesisOutputFormat::Riff48Khz16BitMonoPcm;

			case EAzSpeechSynthesisOutputFormat::Riff22050Hz16BitMonoPcm:
				return Microsoft::CognitiveServices::Speech::SpeechSynthesisOutputFormat::Riff22050Hz16BitMonoPcm;

			case EAzSpeechSynthesisOutputFormat::Riff44100Hz16BitMonoPcm:
				return Microsoft::CognitiveServices::Speech::SpeechSynthesisOutputFormat::Riff44100Hz16BitMonoPcm;

			default:
				break;
		}
	}

	return Microsoft::CognitiveServices::Speech::SpeechSynthesisOutputFormat::Riff16Khz16BitMonoPcm;
}
