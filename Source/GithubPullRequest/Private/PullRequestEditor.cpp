#include "PullRequestEditor.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SSearchBox.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Widgets/Input/SComboBox.h"
#include "GithubApi.h"

#include "IAssetTools.h"
#include "AssetToolsModule.h"

#define LOCTEXT_NAMESPACE "SPullRequestsEditor"

SPullRequestsEditor::SPullRequestsEditor()
{
    for (EPullRequestQueryState State : TEnumRange<EPullRequestQueryState>())
    {
        auto ParamName = UEnum::GetValueAsString((EPullRequestQueryState)State);
        ParamName.Split("::", nullptr, &ParamName);
        QueryOptions.Add(FName(*ParamName));
    }
}

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
                    [
                        SNew(SHorizontalBox)
                        +SHorizontalBox::Slot()
                        [
                            SNew(SComboBox<FName>)
                            .OptionsSource(&QueryOptions)
                            .OnSelectionChanged(this, &SPullRequestsEditor::OnStateChanged)
                            .OnGenerateWidget_Lambda([](FName State)
                                {
                                    return SNew(STextBlock).Text(FText::FromName(State));
                                })
                            .Content()
                            [
                                SNew(STextBlock).Text(this, &SPullRequestsEditor::GetSelectedStateAsText)
                            ]
                        ]
                        +SHorizontalBox::Slot()
                        [
                            SNew(SComboBox<FName>)
                            .OptionsSource(&QueryOptions)
                            .OnSelectionChanged(this, &SPullRequestsEditor::OnStateChanged)
                            .OnGenerateWidget_Lambda([](FName State)
                                {
                                    return SNew(STextBlock).Text(FText::FromName(State));
                                })
                            .Content()
                            [
                                SNew(STextBlock).Text(this, &SPullRequestsEditor::GetSelectedStateAsText)
                            ]
                        ]
                    ]
                    + SScrollBox::Slot()
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
    int Page = 1;
    ListFileItems.Empty();
    FOnFilesListAvailable UpdateListFile;
    UpdateListFile = FOnFilesListAvailable::CreateLambda([this, Entry, Page, UpdateListFile](const TArray<FFileChangeInformation>& Files, int Code, const FString& Content)
    {
        if (Files.IsEmpty())
            return;
        for (auto& Item : Files)
        {
            auto ListItem = MakeShared<FFileItem>();
            ListItem->Info = Item;
            ListItem->PRInfo = Entry;
            ListFileItems.Add(ListItem);
        }
        ListFileWidget->RequestListRefresh();
        UGithubApi::GetFilesInPullRequest(Entry->Info.number, UpdateListFile, 300, Page + 1);

    });

    UGithubApi::GetFilesInPullRequest(Entry->Info.number, UpdateListFile);
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
    auto EnumObject = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPullRequestQueryState"), true);
    auto State = (EPullRequestQueryState)EnumObject->GetValueByName(QueryState);

    FOnPullRequestListAvailable Delegate;
    PullRequestItems.Empty();
    int Page = 1;
    Delegate = FOnPullRequestListAvailable::CreateLambda([this, Delegate, State, Page](const TArray<FPullRequestInformation>& ListPullRequest, int Code, const FString& Content)
    {
        if (ListPullRequest.IsEmpty())
            return;
        for (auto& Item : ListPullRequest)
        {
            auto ListItem = MakeShared<FPullRequestItem>();
            ListItem->Info = Item;
            PullRequestItems.Add(ListItem);
        }
        UGithubApi::GetPullRequests(Delegate, State, Page + 1);
        Widget->RequestListRefresh();
    });

	UGithubApi::GetPullRequests(Delegate, State, Page);
}

void SPullRequestsEditor::CleanCache()
{
}

void SPullRequestsEditor::OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType)
{
}

void SPullRequestsEditor::OnStateChanged(FName InItem, ESelectInfo::Type InSeletionInfo)
{
    QueryState = InItem;
}

FText SPullRequestsEditor::GetSelectedStateAsText() const
{
    return FText::FromName(QueryState);
}

void SPullRequestRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, TSharedPtr<FPullRequestItem> InItem)
{
    Item = InItem;
    UGithubApi::DownloadAvatar(FOnFileDownloadComplete::CreateLambda([this](const FString& GamePath, int Code, const FString& InContent)
        {
            if (GamePath.IsEmpty())
                return;
            Avatar = MakeShared<FSlateDynamicImageBrush>(FSlateDynamicImageBrush(FName(GamePath), FVector2D(16, 16)));
        }),
        Item->Info.user.avatar_url, Item->Info.user.id + ".png");
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
                    SNew(SHorizontalBox)
                    +SHorizontalBox::Slot()
                    .Padding(0, 0, 4, 0)
                    .AutoWidth()
                    [
                        SNew(SImage)
                        .Image(this, &SPullRequestRow::GetAvatar)
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(Item->Info.user.login))
                        .Font(Font)
                    ]
                ];
    }
    return SNullWidget::NullWidget;
}

const FSlateBrush* SPullRequestRow::GetAvatar() const
{
    return Avatar.Get();
}
