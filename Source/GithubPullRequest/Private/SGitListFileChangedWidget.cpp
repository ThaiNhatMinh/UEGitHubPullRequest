// Fill out your copyright notice in the Description page of Project Settings.


#include "SGitListFileChangedWidget.h"
#include "Widgets/Input/SSearchBox.h"
#include "SFileReviewComments.h"

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

    for (auto& File : Files)
    {
        ListFileItems.Add(MakeShared<FCommitFile>(File));
    }

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
    ListFileWidget->RebuildList();
}


void SGitListFileChangedWidget::OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType)
{
}

TSharedRef<ITableRow> SGitListFileChangedWidget::GenerateFileRowWidget(TSharedPtr<FCommitFile> InItem, const TSharedRef<class STableViewBase>& OwnerTable)
{
    static const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 12);

    const FSlateBrush* IconSamples = FAppStyle::Get().GetBrush(TEXT("Graph.StateNode.Icon"));

    if (InItem->status == "added")
    {
        IconSamples = FAppStyle::Get().GetBrush(TEXT("SourceControl.Add"));
    }
    else if (InItem->status == "modified")
    {
        IconSamples = FAppStyle::Get().GetBrush(TEXT("SourceControl.Edit"));
    }
    else if (InItem->status == "renamed")
    {
        IconSamples = FAppStyle::Get().GetBrush(TEXT("SourceControl.Integrate"));
    }
    return SNew(STableRow<TSharedPtr<FCommitFile>>, OwnerTable)
        [
            SNew(SBox)
                .Padding(8)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .Padding(0, 0, 4, 0)
                        .AutoWidth()
                        [
                            SNew(SImage)
                                .Image(IconSamples)
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(STextBlock)
                                .Text(FText::FromString(InItem->filename))
                                .Font(Font)
                        ]
                        +SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(5, 0)
                        [
                            SNew(STextBlock)
                                .Text(FText::FromString(FString::Printf(TEXT("+%d -%d"), InItem->additions, InItem->deletions)))
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                                .ColorAndOpacity(FSlateColor(FLinearColor::Gray))
                        ]
                        +SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(5, 0)
                        [
                            SNew(SBorder)
                            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromString("Viewed"))
                                        .Font(Font)
                                ]
                                + SHorizontalBox::Slot()
                                [
                                    SNew(SCheckBox)
                                ]
                            ]

                        ]
                        +SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(5, 0)
                        [
                            // cooking mode menu
                            SNew(SComboButton)
                                .ButtonContent()
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromString("Comment"))
                                ]
                                .ContentPadding(FMargin(6.0, 2.0))
                                .MenuContent()
                                [
                                    SNew(SFileReviewComments)
                                    .CommitSha(Head)
                                    .CommitFile(InItem)
                                    .PullRequestInfo(PRInfo)
                                ]
                        ]
                ]
        ];
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