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

USTRUCT()
struct FQueryResultError
{
	GENERATED_BODY();


	UPROPERTY()
	FString message;

	UPROPERTY()
	FString documentation_url;

	UPROPERTY()
	TArray<FString> errors;

	UPROPERTY()
	int status;
};

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
	int number = 0;
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

	TArray<FComment> AllComments;
	FSimpleMulticastDelegate OnCommentAvailable;
};

USTRUCT()
struct FReviewComment
{
	GENERATED_BODY();
	UPROPERTY()
	FString path;
	UPROPERTY()
	int position = 0;
	UPROPERTY()
	FString body;
	UPROPERTY()
	int line = 0;
	UPROPERTY()
	FString side;
	UPROPERTY()
	int start_line = 0;
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
struct FCommentBody
{
	GENERATED_BODY();

	UPROPERTY()
	FString body;

	UPROPERTY()
	FString commit_id;

	UPROPERTY()
	FString path;

	UPROPERTY()
	int position = 1;

	UPROPERTY()
	FString side = "RIGHT"; // LEFT / RIGHT

	UPROPERTY()
	int line = 1;

	UPROPERTY()
	FString in_reply_to;

	UPROPERTY()
	FString start_side = "RIGHT";

	UPROPERTY()
	FString subject_type = "file";
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

	UPROPERTY()
	FString html_url;
};

USTRUCT()
struct FCommitVerification
{
	GENERATED_BODY();

	UPROPERTY()
	bool verified = false;

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
	int comment_count = 0;

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
    int id = 0;
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
    bool site_admin = false;
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
	FString sha;

	UPROPERTY()
	FString filename;
	UPROPERTY()
	int additions = 0;
	UPROPERTY()
	int deletions = 0;
	UPROPERTY()
	int changes = 0;
	UPROPERTY()
	FString status;
	UPROPERTY()
	FString raw_url;
	UPROPERTY()
	FString blob_url;
	UPROPERTY()
	FString content_url;
	UPROPERTY()
	FString patch;

	TArray<FComment> AllComments;
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

USTRUCT()
struct FIssueComment
{
	GENERATED_BODY();

	UPROPERTY()
	int id;

	UPROPERTY()
	FString node_id;

	UPROPERTY()
	FString url;

	UPROPERTY()
	FString html_url;

	UPROPERTY()
	FString body;

	UPROPERTY()
	FCommitUserDetail user;

	UPROPERTY()
	FString created_at;

	UPROPERTY()
	FString updated_at;

	UPROPERTY()
	FString issue_url;

	UPROPERTY()
	FString author_association;
};

USTRUCT()
struct FComment
{
	GENERATED_BODY();
	
	UPROPERTY()
	FString url;

	UPROPERTY()
	int pull_request_review_id;

	UPROPERTY()
	int id;

	UPROPERTY()
	FString node_id;

	UPROPERTY()
	FString diff_hunk;

	UPROPERTY()
	FString path;

	UPROPERTY()
	int position;

	UPROPERTY()
	int original_position;

	UPROPERTY()
	FString commit_id;

	UPROPERTY()
	FString original_commit_id;

	UPROPERTY()
	int in_reply_to_id;
	
	UPROPERTY()
	FCommitUserDetail user;

	UPROPERTY()
	FString body;

	UPROPERTY()
	FString created_at;

	UPROPERTY()
	FString updated_at;

	UPROPERTY()
	FString html_url;

	UPROPERTY()
	FString pull_request_url;

	UPROPERTY()
	FString author_association;

	UPROPERTY()
	int start_line = 0;

	UPROPERTY()
	int original_start_line = 0;

	UPROPERTY()
	FString start_side;

	UPROPERTY()
	int line = 0;

	UPROPERTY()
	int original_line = 0;

	UPROPERTY()
	FString side;
};

DECLARE_DELEGATE_ThreeParams(FOnPullRequestListAvailable, const TArray<FPullRequestInformation>& ListPullRequest, int Code, const FString& Content);
DECLARE_DELEGATE_ThreeParams(FOnListCommentAvailable, const TArray<FComment>& ListComment, int Code, const FString& Content);
DECLARE_DELEGATE_ThreeParams(FOnListIssueCommentAvailable, const TArray<FIssueComment>& ListComment, int Code, const FString& Content);
DECLARE_DELEGATE_ThreeParams(FOnPullRequestAvailable, const FPullRequestInformation& PullRequest, int Code, const FString& Content);
DECLARE_DELEGATE_ThreeParams(FOnFilesListAvailable, const TArray<FCommitFile>& Files, int Code, const FString& Content);
DECLARE_DELEGATE_ThreeParams(FOnCommitListAvailable, const TArray<FCommitFullData>& Commits, int Code, const FString& Content);
DECLARE_DELEGATE_ThreeParams(FOnCommitDetailsAvailable, const FCommitFullData& Commit, int Code, const FString& Content);
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
	static bool GetPullRequest(FOnPullRequestAvailable OnPullRequestListAvailable, int PullNumber);
	static bool GetFilesInPullRequest(int PullNumber, FOnFilesListAvailable OnFilesListAvailable, int NumFile = 300, int Page = 1);
	static bool DownloadFile(FOnFileDownloadComplete OnFileDownloadComplete, const FString& FilePath, const FString& Sha = "");
	//static bool DownloadFile(FOnFileDownloadComplete OnFileDownloadComplete, const FCommitFile& CommitFile);
	static bool DownloadAvatar(FOnFileDownloadComplete OnFileDownloadComplete, const FString& Url, const FString& FileName);

	/*
	* https://docs.github.com/en/rest/pulls/reviews?apiVersion=2022-11-28#create-a-review-for-a-pull-request
	* 
	*/
	static bool CreateReview(FOnCreateReviewComplete OnCreateReviewComplete, int PullNumber, const FString& Event, const FString& Body, const FString& CommitId = "", const TArray<FReviewComment>& Comments = {});


	/*
	* https://docs.github.com/en/rest/pulls/comments?apiVersion=2022-11-28#create-a-review-comment-for-a-pull-request
	*/
	static bool CreateComment(FOnCreateReviewComplete OnCreateReviewComplete, int PullNumber, const FString& Body, const FString& CommitId, const FString& FilePath);


	/*
	* https://docs.github.com/en/rest/pulls/comments?apiVersion=2022-11-28#list-review-comments-on-a-pull-request
	*/
	static bool ListComment(FOnListCommentAvailable OnListCommentAvailable, int PullNumber, int Page = 1, int NumberPage = 100);


	/*
	* https://docs.github.com/en/rest/issues/comments?apiVersion=2022-11-28#list-issue-comments
	*/
	static bool ListIssueComment(FOnListIssueCommentAvailable OnListCommentAvailable, int PullNumber, int Page = 1, int NumberPage = 100);

	/*
	* https://docs.github.com/en/rest/commits/commits?apiVersion=2022-11-28#list-commits
	* Query parameters
	* 
	* sha string
	* SHA or branch to start listing commits from. Default: the repository�s default branch (usually main).
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

	/**
	 * Get details of a single commit including files changed
	 * https://docs.github.com/en/rest/commits/commits?apiVersion=2022-11-28#get-a-commit
	 */
	static bool GetCommitDetails(FOnCommitDetailsAvailable OnCommitDetailsAvailable, const FString& CommitSha);
};
