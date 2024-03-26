#pragma once
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "GithubApi.h"

struct FPullRequestItem
{
    FPullRequestInformation Info;
};

struct FFileItem
{
    TSharedPtr<FPullRequestItem> PRInfo;
    FFileChangeInformation Info;
};

struct FCommitItem
{
    TSharedPtr<FPullRequestItem> PRInfo;
    FCommitFullData CommitInfo;
};

struct FOpenDiffAssetStatusEffect
{
    bool bIsOpening = false;
    int NumFileDownload = 0;
    FString BaseVersionFile;
    bool bBaseVersionDownloadSuccess = false;
    FString HeadVersionFile;
    bool bHeadVersionDownloadSuccess = false;
    TSharedPtr<FFileItem> Item;
};

class SReviewChanges : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SReviewChanges) {}
        SLATE_ARGUMENT(TSharedPtr<FPullRequestItem>, PullRequestId)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    FReply OnCommentBtnClick(FString Event) const;

protected:
    TSharedPtr<FPullRequestItem>  PullRequestData;
    TSharedPtr<SMultiLineEditableText> CommentWidget;

};

class SPullRequestEditor : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SPullRequestEditor) {}
        SLATE_ARGUMENT(TSharedPtr<FPullRequestItem>, PullRequestId)
    SLATE_END_ARGS()
    void Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab);
    ~SPullRequestEditor();

    void ReviewChanges() {};
    void Checkout();
    void Refresh();
    void OnCheckoutComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
    void OnFilesListAvailable(const TArray<FFileChangeInformation>& Files, int Code, const FString& Content, int Page);

    TSharedRef<SDockTab> CreateListfilesTab(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> CreateDescriptionTab(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> CreateCommitsTab(const FSpawnTabArgs& Args);
    void OnFileDoubleClicked(TSharedPtr<FFileItem> Entry);
    TSharedRef<ITableRow> GenerateFileRowWidget(TSharedPtr<FFileItem> InItem, const TSharedRef<class STableViewBase>& OwnerTable);
    void OpenDiff();
    void OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType);
    TSharedRef<SWidget> MakeReviewChanges();

protected:
    TSharedPtr<FPullRequestItem>  PullRequestData;
    TSharedPtr<FTabManager> TabManager;
    TSharedPtr<SListView<TSharedPtr<FFileItem>>> ListFileWidget;
    TArray<TSharedPtr<FFileItem>> ListFileItems;
    TArray<TSharedPtr<FCommitItem>> ListCommitItems;
    FOpenDiffAssetStatusEffect OpenDiffStatus;
    TSharedPtr<SWidget> Toolbar;

    TArray<TWeakObjectPtr<UPackage>> OpenedFiles;
};


class SPullRequestsEditor : public SCompoundWidget
{
public:
    SPullRequestsEditor();

    SLATE_BEGIN_ARGS(SPullRequestsEditor)
    {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, const FTabId& ConstructUnderMajorTab,
        const TSharedPtr<SWindow>& ConstructUnderWindow);
    TSharedRef<ITableRow> GenerateCategoryRowWidget(TSharedPtr<FPullRequestItem> InItem, const TSharedRef<class STableViewBase>& OwnerTable);
    void OnMarkerListDoubleClicked(TSharedPtr<FPullRequestItem> Entry);

    void RefreshPullRequest();
    void CleanCache();
    void OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType);
    void OnStateChanged(FName InItem, ESelectInfo::Type InSeletionInfo);
    FText GetSelectedStateAsText() const;

private:
    TSharedPtr<SWidget> Toolbar;
    TSharedPtr<SListView<TSharedPtr<FPullRequestItem>>> Widget;
    TArray<TSharedPtr<FPullRequestItem>> PullRequestItems;
    TArray<FName> QueryOptions;

    TSharedPtr<FTabManager> TabManager;
    TSharedPtr<SDockTab> MajorTab;
    FName QueryState = "Open";
};


class SPullRequestRow : public SMultiColumnTableRow<TSharedPtr<FPullRequestItem>> {
public:
    SLATE_BEGIN_ARGS(SPullRequestRow) {}
    SLATE_END_ARGS()

public:
    void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, TSharedPtr<FPullRequestItem> InItem);

    virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
    const FSlateBrush* GetAvatar() const;

private:
    TSharedPtr<FPullRequestItem> Item;
    TSharedPtr<struct FSlateDynamicImageBrush> Avatar;
};