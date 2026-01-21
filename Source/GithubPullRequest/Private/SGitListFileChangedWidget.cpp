// Fill out your copyright notice in the Description page of Project Settings.


#include "SGitListFileChangedWidget.h"
#include "Widgets/Input/SSearchBox.h"
#include "SAddFileComment.h"
#include "SGitFileDetail.h"

#include "SlateOptMacros.h"
#include "AssetToolsModule.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGitListFileChangedWidget::Construct(const FArguments& InArgs)
{
    Parent = InArgs._Parent.Get();
    Head = InArgs._Head.Get();
    Files = InArgs._Files.Get();
    bShowSearchBox = InArgs._ShowSearchBox.Get();
    bShowViewedMarker = InArgs._ShowViewedMarker.Get();
	PRInfo = InArgs._PullRequestInfo.Get();
    SetListFiles(Files);
 
    TSharedPtr<SSearchBox> SearchBox;
    SAssignNew(ListFileWidget, SListView<TSharedPtr<FCommitFile>>)
        .ListItemsSource(&ListFileItems)
        //.ItemHeight(15)
        .SelectionMode(ESelectionMode::Single)
        .OnGenerateRow(this, &SGitListFileChangedWidget::GenerateFileRowWidget)
        .OnMouseButtonDoubleClick(this, &SGitListFileChangedWidget::OnFileDoubleClicked)
        .ScrollbarVisibility(EVisibility::Visible);
	ChildSlot
	[
        SNew(SVerticalBox)
            +SVerticalBox::Slot()
            .AutoHeight()
            .Padding(10.0f)
            [
                SAssignNew(SearchBox, SSearchBox)
                    .OnTextCommitted(this, &SGitListFileChangedWidget::OnFilterTextCommitted)
            ]
            +SVerticalBox::Slot()
            .Padding(10.0f)
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            .FillHeight(1)
            [
                ListFileWidget.ToSharedRef()
            ]
	];
    if (!bShowSearchBox)
        SearchBox->SetVisibility(EVisibility::Collapsed);
	
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SGitListFileChangedWidget::SetListFiles(const TArray<FCommitFile> NewFiles, bool bAppend)
{
    if (bAppend)
    {
        Files += NewFiles;
    }
    else
    {
        ListFileItems.Reset();
        Files = NewFiles;
    }
    for (auto& File : Files)
    {
        ListFileItems.Add(MakeShared<FCommitFile>(File));
    }
    if (ListFileWidget)
        ListFileWidget->RebuildList();
}


void SGitListFileChangedWidget::OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType)
{
}

TSharedRef<ITableRow> SGitListFileChangedWidget::GenerateFileRowWidget(TSharedPtr<FCommitFile> InItem, const TSharedRef<class STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FCommitFile>>, OwnerTable)
        [
            SNew(SGitFileDetail)
                .PullRequestInfo(PRInfo)
                .CommitSha(Head)
                .FileItem(InItem)
        ];
}

FText SGitListFileChangedWidget::GetFileNumCommentText(TSharedPtr<FCommitFile> InItem) const
{
    auto Comments = GetCommentForFile(InItem);
    if (Comments.IsEmpty())
        return {};
	return FText::AsNumber(Comments.Num());
}

void SGitListFileChangedWidget::OnFileDoubleClicked(TSharedPtr<FCommitFile> Entry)
{
    if (OpenDiffStatus.bIsOpening)
        return;

    OpenDiffStatus.bIsOpening = true;
    OpenDiffStatus.Item = Entry;
    OpenDiffStatus.NumFileDownload = 0;
    UGithubApi::DownloadFile(FOnFileDownloadComplete::CreateLambda([this](const FString& GamePath, int Code, const FString& Content)
        {
            OpenDiffStatus.bHeadVersionDownloadSuccess = !GamePath.IsEmpty();
            OpenDiffStatus.HeadVersionFile = GamePath;
            OpenDiffStatus.NumFileDownload++;
            if (OpenDiffStatus.NumFileDownload == 2)
            {
                OpenDiff();
            }

        }), Entry->filename, Head);

    UGithubApi::DownloadFile(FOnFileDownloadComplete::CreateLambda([this](const FString& GamePath, int Code, const FString& Content)
        {
            OpenDiffStatus.bBaseVersionDownloadSuccess = !GamePath.IsEmpty();
            OpenDiffStatus.BaseVersionFile = GamePath;
            OpenDiffStatus.NumFileDownload++;
            if (OpenDiffStatus.NumFileDownload == 2)
            {
                OpenDiff();
            }

        }), Entry->filename, Parent);
}

TArray<FComment> SGitListFileChangedWidget::GetCommentForFile(TSharedPtr<FCommitFile> InItem) const
{
    if (!PRInfo || PRInfo->AllComments.IsEmpty())
        return {};
    TArray<FComment> Results;

    for (auto& Comment : PRInfo->AllComments)
    {
		if (Comment.path != InItem->filename)
			continue;
        Results.Add(Comment);
    }
    return Results;
}


void SGitListFileChangedWidget::OpenDiff()
{
    if (!OpenDiffStatus.bIsOpening)
        return;
    UPackage* BaseTempPkg = nullptr;
    UPackage* HeadTempPkg = nullptr;
    UObject* HeadAsset = nullptr;
    UObject* BaseAsset = nullptr;
    FString AssetName = FPaths::GetBaseFilename(OpenDiffStatus.Item->filename, true);
    if (OpenDiffStatus.bHeadVersionDownloadSuccess)
    {
        HeadTempPkg = LoadPackage(NULL, *OpenDiffStatus.HeadVersionFile, LOAD_ForDiff | LOAD_DisableCompileOnLoad);
        HeadAsset = FindObject<UObject>(HeadTempPkg, *AssetName);
        OpenedFiles.AddUnique(HeadTempPkg);
    }

    if (OpenDiffStatus.bBaseVersionDownloadSuccess)
    {
        BaseTempPkg = LoadPackage(NULL, *OpenDiffStatus.BaseVersionFile, LOAD_ForDiff | LOAD_DisableCompileOnLoad);
        BaseAsset = FindObject<UObject>(BaseTempPkg, *AssetName);
        OpenedFiles.AddUnique(BaseTempPkg);
    }

    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
    if (HeadAsset != NULL && BaseAsset != NULL)
    {
        FRevisionInfo OldRevision = { Parent, 0, {} };
        FRevisionInfo CurrentRevision = { Head, 1, {}};
        AssetToolsModule.Get().DiffAssets(BaseAsset, HeadAsset, OldRevision, CurrentRevision);
    }
    else if (HeadAsset != NULL)
    {
        AssetToolsModule.Get().OpenEditorForAssets({HeadAsset});
    }
    OpenDiffStatus.bIsOpening = false;
}