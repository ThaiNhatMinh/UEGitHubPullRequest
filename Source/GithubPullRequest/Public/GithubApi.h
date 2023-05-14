// Copyright Ather Labs, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "JsonObjectConverter.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GithubApi.generated.h"

USTRUCT()
struct FContentInfo
{
	GENERATED_BODY();

    UPROPERTY()
    FString name;
    UPROPERTY()
    FString path;
    UPROPERTY()
    FString sha;
    UPROPERTY()
    FString size;
    UPROPERTY()
    FString url;
    UPROPERTY()
    FString html_url;
    UPROPERTY()
    FString git_url;
    UPROPERTY()
    FString download_url;
    UPROPERTY()
    FString type;
    UPROPERTY()
    FString content;
    UPROPERTY()
    FString encoding;
};

USTRUCT()
struct FGithubUser
{
	GENERATED_BODY();

	UPROPERTY()
	FString login;
	UPROPERTY()
	FString id;
	UPROPERTY()
	FString avatar_url;
	UPROPERTY()
	FString gravatarid;
	UPROPERTY()
	FString url;
};


USTRUCT()
struct FPullRequestRefInfo
{
	GENERATED_BODY();
	
	UPROPERTY()
	FString label;
	UPROPERTY()
	FString ref;
	UPROPERTY()
	FString sha;
	UPROPERTY()
	FGithubUser user;
	UPROPERTY()
	FString repo;
};

USTRUCT()
struct FPullRequestInformation
{
	GENERATED_BODY();

	UPROPERTY()
	FString url;
	UPROPERTY()
	FString id;
	UPROPERTY()
	FString html_url;
	
	UPROPERTY()
	int number;
	UPROPERTY()
	FString state;
	UPROPERTY()
	FString title;

	UPROPERTY()
	FGithubUser user;
	UPROPERTY()
	FString body;
	UPROPERTY()
	FPullRequestRefInfo head;
	UPROPERTY()
	FPullRequestRefInfo base;
};

USTRUCT()
struct FFileChangeInformation
{
	GENERATED_BODY();
	UPROPERTY()
	FString sha;
	UPROPERTY()
	FString filename;
	UPROPERTY()
	FString status;
	UPROPERTY()
	FString blob_url;
	UPROPERTY()
	FString raw_url;
	UPROPERTY()
	FString content_url;
	UPROPERTY()
	FString patch;
};

DECLARE_DELEGATE_ThreeParams(FOnPullRequestListAvailable, const TArray<FPullRequestInformation>& ListPullRequest, int Code, const FString& Content);
DECLARE_DELEGATE_ThreeParams(FOnFilesListAvailable, const TArray<FFileChangeInformation>& Files, int Code, const FString& Content);
DECLARE_DELEGATE_ThreeParams(FOnFileDownloadComplete, const FString& GamePath, int Code, const FString& Content);

/**
 * 
 */
UCLASS(Config=Editor)
class GITHUBPULLREQUEST_API UGithubApi : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static FString GetBaseUrl();
	UFUNCTION(BlueprintCallable)
	static void BuildPullRequest();

	static bool GetPullRequests(FOnPullRequestListAvailable OnPullRequestListAvailable);
	static bool GetFilesInPullRequest(int PullNumber, FOnFilesListAvailable OnFilesListAvailable);
	static bool DownloadFile(FOnFileDownloadComplete OnFileDownloadComplete, const FString& FilePath, const FString& Sha = "");
};
