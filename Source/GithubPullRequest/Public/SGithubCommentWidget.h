// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GithubApi.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/SMultiLineEditableText.h"

/**
 * 
 */
class GITHUBPULLREQUEST_API SGithubCommentWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGithubCommentWidget)
	{}
		SLATE_ARGUMENT(FComment, CommentData)
		SLATE_ARGUMENT(bool, ShowReplyInput)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	FReply OnReply();

	void SetReply(const TArray<FComment>& ReplyComments);

protected:
	const FSlateBrush* GetAvatar() const;

protected:
	bool bShowReplyInput = true;
	FComment CommentData;
	TSharedPtr<struct FSlateDynamicImageBrush> Avatar;

	TSharedPtr<SMultiLineEditableText> ReplyInputWidget;
	TSharedPtr<SVerticalBox> RepliesWidget;
};
