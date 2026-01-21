#pragma once
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "ISourceControlOperation.h"
#include "ISourceControlProvider.h"
#include "GithubApi.h"

struct FFileItem
{
    TSharedPtr<FPullRequestInformation> PRInfo;
    FCommitFile Info;
};

struct FCommitItem
{
    TSharedPtr<FPullRequestInformation> PRInfo;
    FCommitFullData CommitInfo;
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
    TSharedRef<ITableRow> GenerateCategoryRowWidget(TSharedPtr<FPullRequestInformation> InItem, const TSharedRef<class STableViewBase>& OwnerTable);
    void OnMarkerListDoubleClicked(TSharedPtr<FPullRequestInformation> Entry);

    void RefreshPullRequest();
    void CleanCache();
    void OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType);
    void OnStateChanged(FName InItem, ESelectInfo::Type InSeletionInfo);
    void OnAuthorChanged(TSharedPtr<FString> InItem, ESelectInfo::Type InSeletionInfo);
    void UpdateAuthorList();
    void ApplyFilters();

private:
    FText GetSelectedStateAsText() const;
    FText GetSelectedAuthorAsText() const;
    FText GetCleanCacheTooltip() const;

private:
    TSharedPtr<SWidget> Toolbar;
    TSharedPtr<SListView<TSharedPtr<FPullRequestInformation>>> Widget;
    TArray<TSharedPtr<FPullRequestInformation>> PullRequestItems;
    TArray<TSharedPtr<FPullRequestInformation>> AllPullRequestItems;
    TArray<FName> QueryOptions;
    TArray<TSharedPtr<FString>> AuthorOptions;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> AuthorComboBox;

    TSharedPtr<FTabManager> TabManager;
    TSharedPtr<SDockTab> MajorTab;
    FName QueryState = "Open";
    TSharedPtr<FString> SelectedAuthor;
};


class SPullRequestRow : public SMultiColumnTableRow<TSharedPtr<FPullRequestInformation>> {
public:
    SLATE_BEGIN_ARGS(SPullRequestRow) {}
    SLATE_END_ARGS()

public:
    void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, TSharedPtr<FPullRequestInformation> InItem);

    virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
    const FSlateBrush* GetAvatar() const;

private:
    TSharedPtr<FPullRequestInformation> Item;
    TSharedPtr<struct FSlateDynamicImageBrush> Avatar;
};