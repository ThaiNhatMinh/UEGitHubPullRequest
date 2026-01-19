// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/SMultiLineEditableText.h"

/**
 * 
 */
class GITHUBPULLREQUEST_API SFileReviewComments : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFileReviewComments)
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

protected:
	TSharedPtr<SMultiLineEditableText> CommentWidget;
};
