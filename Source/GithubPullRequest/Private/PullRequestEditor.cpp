#include "PullRequestEditor.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SSearchBox.h"
#include "GithubApi.h"

#include "IAssetTools.h"
#include "AssetToolsModule.h"

#define LOCTEXT_NAMESPACE "SPullRequestsEditor"

void SPullRequestsEditor::Construct(const FArguments& InArgs)
{
    FToolBarBuilder ToolBarBuilder(TSharedPtr< const FUICommandList >(), FMultiBoxCustomization::None);
    ToolBarBuilder.AddToolBarButton(
        FUIAction(
            FExecuteAction::CreateSP(this, &SPullRequestsEditor::RefreshPullRequest)
        )
        , NAME_None
        , LOCTEXT("SPullRequestsEditor", "Refresh")
        , LOCTEXT("PrevDiffTooltip", "Refresh pull request list")
        , FSlateIcon(FEditorStyle::GetStyleSetName(), "Icons.Refresh")
    );
    ToolBarBuilder.AddToolBarButton(
        FUIAction(
            FExecuteAction::CreateSP(this, &SPullRequestsEditor::CleanCache)
        )
        , NAME_None
        , LOCTEXT("SPullRequestsEditor", "Clean cache")
        , LOCTEXT("SPullRequestsEditor", "Remove cache in save dir")
        , FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentReference.Clear")
    );
    Toolbar = 
        SNew(SHorizontalBox)
        +SHorizontalBox::Slot()
        [
            ToolBarBuilder.MakeWidget()
        ]
    +SHorizontalBox::Slot()
        .HAlign(HAlign_Right)
        .AutoWidth()
        [
            SNew(SBorder)
            .VAlign(VAlign_Center)
        .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
        .Padding(FMargin(0.0f))
        ];

    ChildSlot
    [
        SNew(SVerticalBox)
            +SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 2.0f, 0.0f, 2.0f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .Padding(4.f)
                .AutoWidth()
                [
                    Toolbar.ToSharedRef()
                ]
            ]
            +SVerticalBox::Slot()
            .Padding(10.0f, 2.0f, 10.0f, 20.0f)
            .AutoHeight()
            [
                SNew(SSearchBox)
                .OnTextCommitted(this, &SPullRequestsEditor::OnFilterTextCommitted)
            ]
            +SVerticalBox::Slot()
            .Padding(15.0f)
            [
                SNew(SSplitter)
                + SSplitter::Slot()
                .Value(.2f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    .VAlign(VAlign_Top)
                    [
					    SAssignNew(Widget, SListView<TSharedPtr<FPullRequestItem>>)
					    .ListItemsSource(&PullRequestItems)
				        .ItemHeight(20)
				        .SelectionMode(ESelectionMode::Single)
				        .OnGenerateRow(this, &SPullRequestsEditor::GenerateCategoryRowWidget)
				        .OnMouseButtonDoubleClick(this, &SPullRequestsEditor::OnMarkerListDoubleClicked)
                        .HeaderRow
                        (
                            SNew(SHeaderRow)

                            + SHeaderRow::Column(
                                "Title")
                            .DefaultLabel(
                                LOCTEXT(
                                    "ValidationListSeverityColumnHeader",
                                    "Title"))
                            + SHeaderRow::Column(
                                "Author")
                            .DefaultLabel(
                                LOCTEXT(
                                    "ValidationListMessageColumnHeader",
                                    "Author"))
                            .FillWidth(1.0f)
                        )
                    ]
                ]
                + SSplitter::Slot()
				.Value(.8f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    .VAlign(VAlign_Top)
                    [
                        SAssignNew(ListFileWidget, SListView<TSharedPtr<FFileItem>>)
                        .ListItemsSource(&ListFileItems)
                        .ItemHeight(15)
                        .SelectionMode(ESelectionMode::Single)
                        .OnGenerateRow(this, &SPullRequestsEditor::GenerateFileRowWidget)
                        .OnMouseButtonDoubleClick(this, &SPullRequestsEditor::OnFileDoubleClicked)
                        .ScrollbarVisibility(EVisibility::Visible)
                    ]
                ]
        ]
    ];
    RefreshPullRequest();
}

TSharedRef<ITableRow> SPullRequestsEditor::GenerateCategoryRowWidget(TSharedPtr<FPullRequestItem> InItem, const TSharedRef<class STableViewBase>& OwnerTable)
{
    return SNew(SPullRequestRow, OwnerTable, InItem);
}

TSharedRef<ITableRow> SPullRequestsEditor::GenerateFileRowWidget(TSharedPtr<FFileItem> InItem, const TSharedRef<class STableViewBase>& OwnerTable)
{
	static const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 12);

	const FSlateBrush* IconSamples = FEditorStyle::GetBrush(TEXT("Graph.StateNode.Icon"));

    if (InItem->Info.status == "added")
    {
        IconSamples = FEditorStyle::GetBrush(TEXT("SourceControl.Add"));
    }
    else if (InItem->Info.status == "modified")
    {
        IconSamples = FEditorStyle::GetBrush(TEXT("SourceControl.Edit"));
    }
    else if (InItem->Info.status == "renamed")
    {
        IconSamples = FEditorStyle::GetBrush(TEXT("SourceControl.Integrate"));
    }
	return SNew(STableRow<TSharedPtr<FPullRequestItem>>, OwnerTable)
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
			.Text(FText::FromString(InItem->Info.filename))
		.Font(Font)
		]
		]
		];
}

void SPullRequestsEditor::OnMarkerListDoubleClicked(TSharedPtr<FPullRequestItem> Entry)
{
    UGithubApi::GetFilesInPullRequest(Entry->Info.number, FOnFilesListAvailable::CreateLambda([this, Entry](const TArray<FFileChangeInformation>& Files, int Code, const FString& Content)
    {
            if (Files.IsEmpty())
                return;
            ListFileItems.Empty();
            for (auto& Item : Files)
            {
                auto ListItem = MakeShared<FFileItem>();
                ListItem->Info = Item;
                ListItem->PRInfo = Entry;
                ListFileItems.Add(ListItem);
            }
            ListFileWidget->RequestListRefresh();
    }));
}

void SPullRequestsEditor::OnFileDoubleClicked(TSharedPtr<FFileItem> Entry)
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

        }), Entry->Info.filename, Entry->PRInfo->Info.head.sha);

    UGithubApi::DownloadFile(FOnFileDownloadComplete::CreateLambda([this](const FString& GamePath, int Code, const FString& Content)
        {
            OpenDiffStatus.bBaseVersionDownloadSuccess = !GamePath.IsEmpty();
            OpenDiffStatus.BaseVersionFile = GamePath;
            OpenDiffStatus.NumFileDownload++;
            if (OpenDiffStatus.NumFileDownload == 2)
            {
                OpenDiff();
            }

        }), Entry->Info.filename, Entry->PRInfo->Info.base.sha);
}

void SPullRequestsEditor::OpenDiff()
{
    if (!OpenDiffStatus.bIsOpening)
        return;
    UPackage* BaseTempPkg = nullptr;
    UPackage* HeadTempPkg = nullptr;
    UObject* HeadAsset = nullptr;
    UObject* BaseAsset = nullptr;
    FString AssetName = FPaths::GetBaseFilename(OpenDiffStatus.Item->Info.filename, true);
    if (OpenDiffStatus.bHeadVersionDownloadSuccess)
    {
        HeadTempPkg = LoadPackage(NULL, *OpenDiffStatus.HeadVersionFile, LOAD_ForDiff | LOAD_DisableCompileOnLoad);
        HeadAsset = FindObject<UObject>(HeadTempPkg, *AssetName);
    }

    if (OpenDiffStatus.bBaseVersionDownloadSuccess)
    {
        BaseTempPkg = LoadPackage(NULL, *OpenDiffStatus.BaseVersionFile, LOAD_ForDiff | LOAD_DisableCompileOnLoad);
        BaseAsset = FindObject<UObject>(BaseTempPkg, *AssetName);
    }

    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
    if (HeadAsset != NULL && BaseAsset != NULL)
    {
        FRevisionInfo OldRevision = { OpenDiffStatus.Item->PRInfo->Info.base.sha, 0, {} };
        FRevisionInfo CurrentRevision = { OpenDiffStatus.Item->PRInfo->Info.head.sha, 1, {}};
        AssetToolsModule.Get().DiffAssets(BaseAsset, HeadAsset, OldRevision, CurrentRevision);
    }
    else if (HeadAsset != NULL)
    {
        AssetToolsModule.Get().OpenEditorForAssets({HeadAsset});
    }
    OpenDiffStatus.bIsOpening = false;
}

void SPullRequestsEditor::RefreshPullRequest()
{
	UGithubApi::GetPullRequests(FOnPullRequestListAvailable::CreateLambda([this](const TArray<FPullRequestInformation>& ListPullRequest, int Code, const FString& Content)
		{
			if (ListPullRequest.IsEmpty())
				return;
			PullRequestItems.Empty();
			for (auto& Item : ListPullRequest)
			{
				auto ListItem = MakeShared<FPullRequestItem>();
				ListItem->Info = Item;
				PullRequestItems.Add(ListItem);
			}
			Widget->RequestListRefresh();
		}));
}

void SPullRequestsEditor::CleanCache()
{
}

void SPullRequestsEditor::OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType)
{
}

void SPullRequestRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, TSharedPtr<FPullRequestItem> InItem)
{
    Item = InItem;
    SMultiColumnTableRow<TSharedPtr<FPullRequestItem>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef<SWidget> SPullRequestRow::GenerateWidgetForColumn(const FName& ColumnName)
{

    static const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 12);

    const FSlateBrush* IconSamples = FEditorStyle::GetBrush(TEXT("Graph.StateNode.Icon"));
    if (ColumnName == "Title")
    {
        return SNew(SBox)
            .Padding(8)
            [
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
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
                    .Text(FText::FromString(Item->Info.title))
                    .Font(Font)
                ]
                ];
    }
    else if (ColumnName == "Author")
    {
        return SNew(SBox)
            .Padding(8)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Item->Info.user.login))
                .Font(Font)
            ];
    }
    return SNullWidget::NullWidget;
}
