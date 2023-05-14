// Copyright Ather Labs, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GithubPRDeveloperSettings.generated.h"

/**
 * 
 */
UCLASS(config = Editor, defaultconfig)
class GITHUBPULLREQUEST_API UGithubPullRequestSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(Config, EditDefaultsOnly)
	FString GithubToken;
	UPROPERTY(Config, EditDefaultsOnly)
	FString GithubUrl = "https://api.github.com/";
};
