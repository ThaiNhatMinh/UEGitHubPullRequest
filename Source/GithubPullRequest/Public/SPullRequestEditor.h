// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "GithubApi.h"

struct FOpenDiffAssetStatusEffect
{
    bool bIsOpening = false;
    int NumFileDownload = 0;
    FString BaseVersionFile;
    bool bBaseVersionDownloadSuccess = false;
    FString HeadVersionFile;
    bool bHeadVersionDownloadSuccess = false;
    TSharedPtr<FCommitFullData> Item;
};

class SPullRequestEditor : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SPullRequestEditor) {}
        SLATE_ARGUMENT(TSharedPtr<FPullRequestInformation>, PullRequestId)
    SLATE_END_ARGS()
    void Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab);
    ~SPullRequestEditor();

    void ReviewChanges() {};
    void Checkout();
    void Refresh();
    void UpdateInfo(const FPullRequestInformation& PullRequest, int Code, const FString& Content);
    //void OnCheckoutComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
    void OnFilesListAvailable(const TArray<FCommitFile>& Files, int Code, const FString& Content, int Page);

    TSharedRef<SDockTab> CreateListfilesTab(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> CreateDescriptionTab(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> CreateActiviesTab(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> CreateCommitsTab(const FSpawnTabArgs& Args);
    TSharedRef<SWidget> CreateCommitRowWidget(const FCommitFullData& CommitData);
    void OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType);
    TSharedRef<SWidget> MakeReviewChanges();

    void UpdateState();
    void UpdateComment();

protected:
    TSharedPtr<FPullRequestInformation>  PullRequestData;
    TSharedPtr<FTabManager> TabManager;
    //TSharedPtr<SListView<TSharedPtr<FFileItem>>> ListFileWidget;

    TSharedPtr<SVerticalBox> ListCommitWidget;
    TSharedPtr<SBorder> Status;
    TSharedPtr<STextBlock> StatusText;
    TSharedPtr<class SRichTextBlock> DescriptionText;
    TSharedPtr<class SGitListFileChangedWidget> ListFileWidget;

    TArray<TSharedPtr<FCommitFullData>> ListCommitItems;
    FOpenDiffAssetStatusEffect OpenDiffStatus;
    TSharedPtr<SWidget> Toolbar;

    TArray<TWeakObjectPtr<UPackage>> OpenedFiles;
};
