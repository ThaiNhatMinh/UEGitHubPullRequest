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

USTRUCT()
struct FReviewComment
{
	GENERATED_BODY();
	UPROPERTY()
	FString path;
	UPROPERTY()
	int position;
	UPROPERTY()
	FString body;
	UPROPERTY()
	int line;
	UPROPERTY()
	FString side;
	UPROPERTY()
	int start_line;
	UPROPERTY()
	FString start_side;
};

USTRUCT()
struct FReviewBody
{
	GENERATED_BODY();
	
	UPROPERTY()
	FString commit_id;
	
	UPROPERTY()
	FString body;
	
	UPROPERTY()
	FString event;
	
	UPROPERTY()
	TArray<FReviewComment> comments;
};

USTRUCT()
struct FCommitUser
{
	GENERATED_BODY();

	UPROPERTY()
	FString name;
	UPROPERTY()
	FString email;
	UPROPERTY()
	FString date;
};

USTRUCT()
struct FCommitTree
{
	GENERATED_BODY();

	UPROPERTY()
	FString url;

	UPROPERTY()
	FString sha;
};

USTRUCT()
struct FCommitVerification
{
	GENERATED_BODY();

	UPROPERTY()
	bool verified;

	UPROPERTY()
	FString reason;

	UPROPERTY()
	FString signature;

	UPROPERTY()
	FString payload;
};

USTRUCT()
struct FCommit
{
	GENERATED_BODY();

	UPROPERTY()
	FString url;

	UPROPERTY()
	FCommitUser author;

	UPROPERTY()
	FCommitUser committer;

	UPROPERTY()
	FString message;

	UPROPERTY()
	FCommitTree tree;

	UPROPERTY()
	int comment_count;

	UPROPERTY()
	FCommitVerification verification;
};

USTRUCT()
struct FCommitUserDetail
{
	GENERATED_BODY();
	UPROPERTY()
    FString login;
    UPROPERTY()
    int id;
    UPROPERTY()
    FString node_id;
    UPROPERTY()
    FString avatar_url;
    UPROPERTY()
    FString gravatar_id;
    UPROPERTY()
    FString url;
    UPROPERTY()
    FString html_url;
    UPROPERTY()
    FString followers_url;
    UPROPERTY()
    FString following_url;
    UPROPERTY()
    FString gists_url;
    UPROPERTY()
    FString starred_url;
    UPROPERTY()
    FString subscriptions_url;
    UPROPERTY()
    FString organizations_url;
    UPROPERTY()
    FString repos_url;
    UPROPERTY()
    FString events_url;
    UPROPERTY()
    FString received_events_url;
    UPROPERTY()
    FString type;
    UPROPERTY()
    bool site_admin;
};

USTRUCT()
struct FCommitStat
{
	GENERATED_BODY();
	UPROPERTY()
	FString additions;
	UPROPERTY()
	FString deletions;
	UPROPERTY()
	FString total;
};

USTRUCT()
struct FCommitFile
{
	GENERATED_BODY();
	UPROPERTY()
	FString filename;
	UPROPERTY()
	int additions;
	UPROPERTY()
	int deletions;
	UPROPERTY()
	int changes;
	UPROPERTY()
	FString status;
	UPROPERTY()
	FString raw_url;
	UPROPERTY()
	FString blob_url;
	UPROPERTY()
	FString patch;
};

USTRUCT()
struct FCommitFullData
{
	GENERATED_BODY();

	UPROPERTY()
	FString url;

	UPROPERTY()
	FString sha;

	UPROPERTY()
	FString node_id;

	UPROPERTY()
	FString html_url;

	UPROPERTY()
	FString comments_url;

	UPROPERTY()
	FCommit commit;

	UPROPERTY()
	FCommitUserDetail author;

	UPROPERTY()
	FCommitUserDetail committer;

	UPROPERTY()
	TArray<FCommitTree> parents;

	UPROPERTY()
	FCommitStat stats;

	UPROPERTY()
	TArray<FCommitFile> files;
};

DECLARE_DELEGATE_ThreeParams(FOnPullRequestListAvailable, const TArray<FPullRequestInformation>& ListPullRequest, int Code, const FString& Content);
DECLARE_DELEGATE_ThreeParams(FOnFilesListAvailable, const TArray<FFileChangeInformation>& Files, int Code, const FString& Content);
DECLARE_DELEGATE_ThreeParams(FOnCommitListAvailable, const TArray<FCommitFullData>& Commits, int Code, const FString& Content);
DECLARE_DELEGATE_ThreeParams(FOnFileDownloadComplete, const FString& GamePath, int Code, const FString& Content);
DECLARE_DELEGATE_TwoParams(FOnCreateReviewComplete, int Code, const FString& Content);

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

	/*
	* https://docs.github.com/en/rest/pulls/reviews?apiVersion=2022-11-28#create-a-review-for-a-pull-request
	* 
	*/
	static bool CreateReview(FOnCreateReviewComplete OnCreateReviewComplete, int PullNumber, const FString& Event, const FString& Body, const FString& CommitId = "", const TArray<FReviewComment>& Comments = {});

	/*
	* https://docs.github.com/en/rest/commits/commits?apiVersion=2022-11-28#list-commits
	* Query parameters
	* 
	* sha string
	* SHA or branch to start listing commits from. Default: the repository’s default branch (usually main).
	* 
	* path string
	* Only commits containing this file path will be returned.
	* 
	* author string
	* GitHub username or email address to use to filter by commit author.
	* 
	* committer string
	* GitHub username or email address to use to filter by commit committer.
	* 
	* since string
	* Only show results that were last updated after the given time. This is a timestamp in ISO 8601 format: YYYY-MM-DDTHH:MM:SSZ.
	* 
	* until string
	* Only commits before this date will be returned. This is a timestamp in ISO 8601 format: YYYY-MM-DDTHH:MM:SSZ.
	* 
	* per_page integer
	* The number of results per page (max 100).
	* Default: 30
	* 
	* page integer
	* Page number of the results to fetch.
	* Default: 1
	* 
	*/
	static bool ListCommit(FOnCommitListAvailable OnListCommitComplete, const FString& sha, const FString& path = "", const FString& author = "", const FString& committer = "", const FString& since = "", const FString& until = "", int page = 1, int per_page = 100);
	static bool ListCommitFromPull(FOnCommitListAvailable OnListCommitComplete, int PullNumber, int page = 1, int per_page = 100);
};
