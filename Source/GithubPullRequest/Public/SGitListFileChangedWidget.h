// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GithubApi.h"
#include "Widgets/SCompoundWidget.h"


struct FOpenDiffAssetStatus
{
	bool bIsOpening = false;
	int NumFileDownload = 0;
	FString BaseVersionFile;
	bool bBaseVersionDownloadSuccess = false;
	FString HeadVersionFile;
	bool bHeadVersionDownloadSuccess = false;
	TSharedPtr<FCommitFile> Item;
};

/**
 * 
 */
class GITHUBPULLREQUEST_API SGitListFileChangedWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGitListFileChangedWidget)
	{}
		SLATE_ATTRIBUTE(bool, ShowSearchBox)
		SLATE_ATTRIBUTE(FString, Parent)
		SLATE_ATTRIBUTE(FString, Head)
		SLATE_ATTRIBUTE(bool, ShowViewedMarker)
		SLATE_ATTRIBUTE(TArray<FCommitFile>, Files)
		SLATE_ATTRIBUTE(TSharedPtr<FPullRequestInformation>, PullRequestInfo)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void SetListFiles(const TArray<FCommitFile> NewFiles, bool bAppend = false);

	int32 GetTotalFile() const { return Files.Num(); }

protected:
	void OpenDiff();

	void OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType);
	TSharedRef<ITableRow> GenerateFileRowWidget(TSharedPtr<FCommitFile> InItem, const TSharedRef<class STableViewBase>& OwnerTable);
	void OnFileDoubleClicked(TSharedPtr<FCommitFile> Entry);

protected:
	TSharedPtr<FPullRequestInformation> PRInfo;

	bool bShowSearchBox = true;
	TSharedPtr<SListView<TSharedPtr<FCommitFile>>> ListFileWidget;

	TArray<TSharedPtr<FCommitFile>> ListFileItems;
	FOpenDiffAssetStatus OpenDiffStatus;

	FString Parent;
	FString Head;
	bool bShowViewedMarker = false;
	TArray<FCommitFile> Files;
	TArray<TWeakObjectPtr<UPackage>> OpenedFiles;
};
