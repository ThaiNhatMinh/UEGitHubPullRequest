#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "JsonObjectConverter.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GithubApi.generated.h"

UENUM()
enum class EPullRequestQueryState : uint8
{
	Open UMETA(DisplayName="open"),
	Closed UMETA(DisplayName="closed"),
	All UMETA(DisplayName="all"),
};
ENUM_RANGE_BY_FIRST_AND_LAST(EPullRequestQueryState, EPullRequestQueryState::Open, EPullRequestQueryState::All);


struct FPullRequestQueryParameter
{
	/**
	 * @brief Either open, closed, or all to filter by state.
		Default: open
		Can be one of: open, closed, all
	*/
	EPullRequestQueryState State = EPullRequestQueryState::Open;
	/**
	 * @brief Filter pulls by head user or head organization and branch name in the format of user:ref-name or organization:ref-name.
	 * For example: github:new-script-format or octocat:test-branch.
	*/
	FString head;
	/**
	 * @brief Filter pulls by base branch name. Example: gh-pages.
	*/
	FString base;
	FString sort;
	FString Direction;
	int32 NumberOfResult = 30;
	int32 Page = 1;
};

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

	static bool GetPullRequests(FOnPullRequestListAvailable OnPullRequestListAvailable, EPullRequestQueryState state = EPullRequestQueryState::Open, int Page = 1, int NumberPage = 100);
	static bool GetFilesInPullRequest(int PullNumber, FOnFilesListAvailable OnFilesListAvailable, int NumFile = 300, int Page = 1);
	static bool DownloadFile(FOnFileDownloadComplete OnFileDownloadComplete, const FString& FilePath, const FString& Sha = "");
	static bool DownloadAvatar(FOnFileDownloadComplete OnFileDownloadComplete, const FString& Url, const FString& FileName);
};
