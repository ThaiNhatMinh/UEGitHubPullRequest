// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "GithubApi.h"

/**
 * 
 */
class GITHUBPULLREQUEST_API SGitFileDetail : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGitFileDetail)
	{}
		SLATE_ARGUMENT(TSharedPtr<FCommitFile>, FileItem)
		SLATE_ARGUMENT(TSharedPtr<FPullRequestInformation>, PullRequestInfo)
		SLATE_ARGUMENT(FString, CommitSha)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	FText GetFileNumCommentText() const;

	void UpdateCommentForFile();

protected:
	TSharedPtr<FPullRequestInformation> PRInfo;
	TSharedPtr<FCommitFile> FileItem;
	FString CommitSha;

	TSharedPtr<SVerticalBox> FileDetail;

};
