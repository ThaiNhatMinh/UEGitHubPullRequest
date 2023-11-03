#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SourceControlHelpers.h"
#include "GithubPRDeveloperSettings.generated.h"

/**
 *
 */
UCLASS(config=EditorPerProjectUserSettings, defaultconfig)
class GITHUBPULLREQUEST_API UGithubPullRequestSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UGithubPullRequestSettings();
	void PostEditChangeChainProperty( struct FPropertyChangedChainEvent& PropertyChangedEvent);
	UPROPERTY(Config, EditDefaultsOnly)
	FString GithubToken;
	UPROPERTY(Config, EditDefaultsOnly)
	FString GithubUrl = "https://api.github.com/";
};
