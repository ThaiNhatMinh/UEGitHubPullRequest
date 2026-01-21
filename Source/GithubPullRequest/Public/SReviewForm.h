// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "GithubApi.h"

/**
 * 
 */
class GITHUBPULLREQUEST_API SReviewForm : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReviewForm)
	{}
        SLATE_ARGUMENT(TSharedPtr<FPullRequestInformation>, PullRequestId)
    SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

    FReply OnSubmitReview();
    void OnReviewTypeChanged(FString ReviewType);
    ECheckBoxState IsReviewTypeSelected(FString ReviewType) const;

protected:
    TSharedPtr<FPullRequestInformation> PullRequestData;
    TSharedPtr<SMultiLineEditableText> CommentWidget;
    FString SelectedReviewType = TEXT("COMMENT");
};
