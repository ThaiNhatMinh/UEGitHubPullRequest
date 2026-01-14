// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GithubApi.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class GITHUBPULLREQUEST_API SGitCommitWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGitCommitWidget)
	{}
		SLATE_ATTRIBUTE(FCommitFullData, CommitData)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void OnCommitDetailsAvailable(const FCommitFullData& Commit, int Code, const FString& Content);

protected:
	TSharedPtr<SBorder> CommitHeader;
	TSharedPtr<SVerticalBox> CommitDetail;
	TSharedPtr<class SGitListFileChangedWidget> FilesContainer;
	TSharedPtr<STextBlock> EmptyFileText;
	FCommitFullData CommitData;
};
