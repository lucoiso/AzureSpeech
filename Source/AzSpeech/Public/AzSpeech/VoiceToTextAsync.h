// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEAzSpeech

#pragma once

#include "CoreMinimal.h"
#include "AzSpeechData.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "VoiceToTextAsync.generated.h"

/**
 *
 */
UCLASS(NotPlaceable, Category = "AzSpeech")
class AZSPEECH_API UVoiceToTextAsync final : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/* Task delegate that will be called when completed */
	UPROPERTY(BlueprintAssignable, Category = "AzSpeech")
	FVoiceToTextDelegate TaskCompleted;

	/* Creates a Voice-To-Text task that will convert your speech to string */
	UFUNCTION(BlueprintCallable, Category = "AzSpeech",
		meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject",
			DeprecatedFunction = "true", DeprecationMessage =
			"Use 'Voice To Text' instead - FAzSpeechData will be replaced by AzSpeech Settings (Project Settings)"))
	static UVoiceToTextAsync* VoiceToTextAsync(const UObject* WorldContextObject, const FAzSpeechData Parameters)
	{
		return VoiceToText(WorldContextObject, Parameters.LanguageID);
	}

	/* Creates a Voice-To-Text task that will convert your speech to string */
	UFUNCTION(BlueprintCallable, Category = "AzSpeech",
		meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UVoiceToTextAsync* VoiceToText(const UObject* WorldContextObject, const FString& LanguageId = "Default");

	virtual void Activate() override;

private:
	const UObject* WorldContextObject;
	FString LanguageID;
};
