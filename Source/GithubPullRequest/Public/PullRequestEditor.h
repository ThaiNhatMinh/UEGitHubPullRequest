#pragma once
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Views/SListView.h"
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

class SPullRequestsEditor : public SCompoundWidget
{
    SLATE_BEGIN_ARGS(SPullRequestsEditor)
    {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    TSharedRef<ITableRow> GenerateCategoryRowWidget(TSharedPtr<FPullRequestItem> InItem, const TSharedRef<class STableViewBase>& OwnerTable);
    TSharedRef<ITableRow> GenerateFileRowWidget(TSharedPtr<FFileItem> InItem, const TSharedRef<class STableViewBase>& OwnerTable);
    void OnMarkerListDoubleClicked(TSharedPtr<FPullRequestItem> Entry);
    void OnFileDoubleClicked(TSharedPtr<FFileItem> Entry);

    void OpenDiff();
    void RefreshPullRequest();
    void CleanCache();
    void OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType);
private:
    TSharedPtr<SWidget> Toolbar;
    TSharedPtr<SListView<TSharedPtr<FPullRequestItem>>> Widget;
    TSharedPtr<SListView<TSharedPtr<FFileItem>>> ListFileWidget;
    TArray<TSharedPtr<FPullRequestItem>> PullRequestItems;
    TArray<TSharedPtr<FFileItem>> ListFileItems;
    FOpenDiffAssetStatusEffect OpenDiffStatus;
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