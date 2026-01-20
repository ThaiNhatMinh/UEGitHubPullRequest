// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "GithubApi.h"

/**
 * 
 */
class GITHUBPULLREQUEST_API SFileReviewComments : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFileReviewComments)
	{}
	SLATE_ATTRIBUTE(TSharedPtr<FCommitFile>, CommitFile)
	SLATE_ATTRIBUTE(TSharedPtr<FPullRequestInformation>, PullRequestInfo)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

protected:
	FReply OnAddSingleComment();

	FReply OnStartReview();

protected:
	TSharedPtr<SMultiLineEditableText> CommentWidget;
	TSharedPtr<FPullRequestInformation> PRInfo;
	TSharedPtr<FCommitFile> CommitFile;
};
