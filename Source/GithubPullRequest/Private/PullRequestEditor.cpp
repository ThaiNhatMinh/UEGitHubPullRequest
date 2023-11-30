#include "PullRequestEditor.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Widgets/Input/SComboBox.h"
#include "GithubApi.h"

#include "IAssetTools.h"
#include "AssetToolsModule.h"

#define LOCTEXT_NAMESPACE "SPullRequestsEditor"

static const FName GithubPR_ListFile_TabId = FName(TEXT("GithubPR_Listfiles_TabId"));
static const FName GithubPR_Description_TabId = FName(TEXT("GithubPR_Description_TabId"));
static const FName GithubPR_Commits_TabId = FName(TEXT("GithubPR_Commits_TabId"));
static const FName GithubPR_List_TabId = FName(TEXT("GithubPR_List_TabId"));

void SReviewChanges::Construct(const FArguments& InArgs)
{
    PullRequestData = InArgs._PullRequestId;
    SAssignNew(CommentWidget, SMultiLineEditableText)
        .Text(FText::FromString(TEXT("ASDASDSDSD")))
        .WrapTextAt(200.0f);

    ChildSlot
        [
            SNew(SBox)
            .Padding(10)
            [
            SNew(SVerticalBox)
            +SVerticalBox::Slot()
            .AutoHeight()
            .Padding(20)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("SPullRequestsEditor", "Finish your review"))
                .Font(FCoreStyle::GetDefaultFontStyle("Normal", 15))
            ]
            +SVerticalBox::Slot()
            .MaxHeight(300)
            .Padding(0, 0, 0, 10)
            [
                SNew(SBorder)
                [
                    CommentWidget.ToSharedRef()
                ]
            ]
            +SVerticalBox::Slot()
            .AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .Padding(5)
                    [
                        SNew(SButton).Text(FText::FromString(TEXT("Comments")))
                        .OnClicked(this, &SReviewChanges::OnCommentBtnClick, FString(TEXT("COMMENT")))
                    ]
                    + SHorizontalBox::Slot()
                    .Padding(5)
                    [
                        SNew(SButton).Text(FText::FromString(TEXT("Approve")))
                        .OnClicked(this, &SReviewChanges::OnCommentBtnClick, FString(TEXT("APPROVE")))
                    ]
                    + SHorizontalBox::Slot()
                    .Padding(5)
                    [
                        SNew(SButton).Text(FText::FromString(TEXT("Request change")))
                        .OnClicked(this, &SReviewChanges::OnCommentBtnClick, FString(TEXT("REQUEST_CHANGES")))
                    ]
                ]
            ]
        ];
}

FReply SReviewChanges::OnCommentBtnClick(FString Event) const
{
    if (CommentWidget->GetText().IsEmptyOrWhitespace() || !PullRequestData.IsValid())
        return FReply::Unhandled();
    auto Lamda = FOnCreateReviewComplete::CreateLambda([this](int Code, const FString& Content)
    {
        int a = 0;
        if (Code == 200)
        {
            int b = 0;
        }
    });
    UGithubApi::CreateReview(Lamda, PullRequestData->Info.number, Event, CommentWidget->GetText().ToString());
    return FReply::Handled();
}


void SPullRequestEditor::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab)
{
    PullRequestData = InArgs._PullRequestId;

    FToolBarBuilder ToolBarBuilder(TSharedPtr< const FUICommandList >(), FMultiBoxCustomization::None);
    ToolBarBuilder.AddComboButton(FUIAction(), FOnGetContent::CreateRaw(this, &SPullRequestEditor::MakeReviewChanges),
        LOCTEXT("SPullRequestsEditor", "Review changes"),
        LOCTEXT("SPullRequestsEditor", "Comments/Approve/Request change"),
        FSlateIcon(FAppStyle::Get().GetStyleSetName(), "BlueprintDiff.ToolbarIcon"));
    // Set up a tab view so we can split the content into different views
    TabManager = FGlobalTabmanager::Get()->NewTabManager(ConstructUnderMajorTab);

    TabManager->RegisterTabSpawner(GithubPR_ListFile_TabId,
        FOnSpawnTab::CreateRaw(this, &SPullRequestEditor::CreateListfilesTab))
        .SetDisplayName(LOCTEXT("SPullRequestsEditor", "Files changed"));

    TabManager->RegisterTabSpawner(GithubPR_Commits_TabId,
        FOnSpawnTab::CreateRaw(this, &SPullRequestEditor::CreateCommitsTab))
        .SetDisplayName(LOCTEXT("SPullRequestsEditor", "Commits"));

    TabManager->RegisterTabSpawner(GithubPR_Description_TabId,
        FOnSpawnTab::CreateRaw(this, &SPullRequestEditor::CreateDescriptionTab))
        .SetDisplayName(LOCTEXT("SPullRequestsEditor", "Description"));

    // Create a default layout for the tab manager, this allows us to open the tabs by default
    const TSharedRef<FTabManager::FLayout> DefaultLayout = FTabManager::NewLayout("GithubPR_Layout")
        ->AddArea(
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Horizontal)
            ->Split
            (
                FTabManager::NewStack()
                ->AddTab(GithubPR_Description_TabId, ETabState::OpenedTab)
                ->AddTab(GithubPR_Commits_TabId, ETabState::OpenedTab)
                ->AddTab(GithubPR_ListFile_TabId, ETabState::OpenedTab)
            )
        );

    const auto GithubPRView = TabManager->RestoreFrom(DefaultLayout, nullptr).ToSharedRef();

    Toolbar =
		SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
		[
			ToolBarBuilder.MakeWidget()
		]
	    + SHorizontalBox::Slot()
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
                .Padding(20)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("%s #%d"), *PullRequestData->Info.title, PullRequestData->Info.number)))
                    .HighlightText(FText::FromString(FString::Printf(TEXT("#%d"), PullRequestData->Info.number)))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
                ]
            + SVerticalBox::Slot()
                .Padding(20, 0)
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("%s"), *PullRequestData->Info.state)))
                    .HighlightText(FText::FromString(FString::Printf(TEXT("%s"), *PullRequestData->Info.state)))
                    .Font(FCoreStyle::GetDefaultFontStyle("Normal", 15))
                ]
            +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 2.0f, 0.0f, 2.0f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .Padding(10.f)
                    .AutoWidth()
                    [
                        Toolbar.ToSharedRef()
                    ]
                ]
            +SVerticalBox::Slot()
                .Padding(10.f)
                .FillHeight(1)
                [
                    SNew(SBorder)
                    .Padding(10)
                    [
                        GithubPRView
                    ]
                ]
		];
}

TSharedRef<SWidget> SPullRequestEditor::MakeReviewChanges()
{
    return SNew(SReviewChanges).PullRequestId(PullRequestData);
}

TSharedRef<SDockTab> SPullRequestEditor::CreateCommitsTab(const FSpawnTabArgs& Args)
{
    auto ListCommitWidget = SNew(SVerticalBox);
    auto NewWidget = SNew(SVerticalBox)
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f)
        [
            SNew(SSearchBox)
            .OnTextCommitted(this, &SPullRequestEditor::OnFilterTextCommitted)
        ]
        +SVerticalBox::Slot()
        .Padding(10.0f)
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        .FillHeight(1)
        [
            ListCommitWidget
        ];

    auto DockTab = SNew(SDockTab)
        .TabRole(ETabRole::PanelTab)
        .OnCanCloseTab_Lambda([](){return false;})
        [
            NewWidget
        ];

    int Page = 1;
    ListCommitItems.Empty();
    FOnCommitListAvailable OnCommitList;
    OnCommitList = FOnCommitListAvailable::CreateLambda([this, Page, OnCommitList, DockTab, ListCommitWidget](const TArray<FCommitFullData>& Commits, int Code, const FString& Content)
        {
            if (Commits.IsEmpty())
                return;
            for (auto& Item : Commits)
            {
                auto ListItem = MakeShared<FCommitItem>();
                ListItem->CommitInfo = Item;
                ListItem->PRInfo = PullRequestData;
                ListCommitItems.Add(ListItem);
                
                ListCommitWidget->AddSlot()
                    .AutoHeight()
                    .Padding(10)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(FString::Printf(TEXT("%s"), *Item.commit.message)))
                    ];
            }
            DockTab->SetLabel(FText::FromString(FString::Printf(TEXT("Commits (%d)"), ListCommitItems.Num())));
            UGithubApi::ListCommitFromPull(OnCommitList, PullRequestData->Info.number, Page + 1);

        });

    UGithubApi::ListCommitFromPull(OnCommitList, PullRequestData->Info.number, Page);
    return DockTab;
}

TSharedRef<SDockTab> SPullRequestEditor::CreateListfilesTab(const FSpawnTabArgs& Args)
{
    if (!ListFileWidget.IsValid())
    {
        SAssignNew(ListFileWidget, SListView<TSharedPtr<FFileItem>>)
            .ListItemsSource(&ListFileItems)
            .ItemHeight(15)
            .SelectionMode(ESelectionMode::Single)
            .OnGenerateRow(this, &SPullRequestEditor::GenerateFileRowWidget)
            .OnMouseButtonDoubleClick(this, &SPullRequestEditor::OnFileDoubleClicked)
            .ScrollbarVisibility(EVisibility::Visible);
    }

    auto NewWidget = SNew(SVerticalBox)
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f)
        [
            SNew(SSearchBox)
            .OnTextCommitted(this, &SPullRequestEditor::OnFilterTextCommitted)
        ]
        +SVerticalBox::Slot()
        .Padding(10.0f)
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        .FillHeight(1)
        [
            ListFileWidget.ToSharedRef()
        ];

    auto DockTab = SNew(SDockTab)
        .OnCanCloseTab_Lambda([](){return false;})
        .TabRole(ETabRole::PanelTab)
        [
            NewWidget
        ];

    int Page = 1;
    ListFileItems.Empty();
    FOnFilesListAvailable UpdateListFile;
    UpdateListFile = FOnFilesListAvailable::CreateLambda([this, Page, UpdateListFile, DockTab](const TArray<FFileChangeInformation>& Files, int Code, const FString& Content)
        {
            if (Files.IsEmpty())
                return;
            for (auto& Item : Files)
            {
                auto ListItem = MakeShared<FFileItem>();
                ListItem->Info = Item;
                ListItem->PRInfo = PullRequestData;
                ListFileItems.Add(ListItem);
            }
            DockTab->SetLabel(FText::FromString(FString::Printf(TEXT("Files changed (%d)"), ListFileItems.Num())));
            if (ListFileWidget.IsValid())
                ListFileWidget->RequestListRefresh();
            UGithubApi::GetFilesInPullRequest(PullRequestData->Info.number, UpdateListFile, 9999, Page + 1);

        });

    UGithubApi::GetFilesInPullRequest(PullRequestData->Info.number, UpdateListFile);
    return DockTab;
}

TSharedRef<SDockTab> SPullRequestEditor::CreateDescriptionTab(const FSpawnTabArgs& Args)
{

    auto WidgetTag = SNew(SBox).Padding(10.0f)
        [
            SNew(SRichTextBlock)
            .DecoratorStyleSet(&FEditorStyle::Get())
            .Text(FText::FromString(PullRequestData->Info.body))
            .TextStyle(&FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>( "Text.Large" ))
        ];
    return SNew(SDockTab)
        .OnCanCloseTab_Lambda([](){return false;})
        .TabRole(ETabRole::PanelTab)
        [
            WidgetTag
        ];
}

void SPullRequestEditor::OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType)
{
    if (InCommitType != ETextCommit::OnEnter)
        return;

}

TSharedRef<ITableRow> SPullRequestEditor::GenerateFileRowWidget(TSharedPtr<FFileItem> InItem, const TSharedRef<class STableViewBase>& OwnerTable)
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


void SPullRequestEditor::OnFileDoubleClicked(TSharedPtr<FFileItem> Entry)
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

void SPullRequestEditor::OpenDiff()
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




SPullRequestsEditor::SPullRequestsEditor()
{
    for (EPullRequestQueryState State : TEnumRange<EPullRequestQueryState>())
    {
        auto ParamName = UEnum::GetValueAsString((EPullRequestQueryState)State);
        ParamName.Split("::", nullptr, &ParamName);
        QueryOptions.Add(FName(*ParamName));
    }
}

void SPullRequestsEditor::Construct(const FArguments& InArgs, const FTabId& ConstructUnderMajorTab,
    const TSharedPtr<SWindow>& ConstructUnderWindow)
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

    // Set up a tab view so we can split the content into different views
    MajorTab = SNew(SDockTab).TabRole(ETabRole::MajorTab);
    TabManager = FGlobalTabmanager::Get()->NewTabManager(MajorTab.ToSharedRef());

    // Create a default layout for the tab manager, this allows us to open the tabs by default
    const TSharedRef<FTabManager::FLayout> DefaultLayout = FTabManager::NewLayout("GithubPRList_Layout")
        ->AddArea(
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Horizontal)
            ->Split
            (
                FTabManager::NewStack()
                    ->AddTab(GithubPR_List_TabId, ETabState::ClosedTab)
            )
        );

    const auto GithubPRList = TabManager->RestoreFrom(DefaultLayout, ConstructUnderWindow).ToSharedRef();

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
                    GithubPRList
                ]
        ]
    ];
    RefreshPullRequest();
}

TSharedRef<ITableRow> SPullRequestsEditor::GenerateCategoryRowWidget(TSharedPtr<FPullRequestItem> InItem, const TSharedRef<class STableViewBase>& OwnerTable)
{
    return SNew(SPullRequestRow, OwnerTable, InItem);
}

void SPullRequestsEditor::OnMarkerListDoubleClicked(TSharedPtr<FPullRequestItem> Entry)
{
    TSharedPtr<SDockTab> NewTab = SNew(SDockTab).Label(FText::FromString(Entry->Info.title))
        [
            // add a wrapper widget here, that says the name of the object/component for pinned tabs 
            SNew(SPullRequestEditor, MajorTab.ToSharedRef()).PullRequestId(Entry)
        ];


    TabManager->InsertNewDocumentTab(GithubPR_List_TabId, FTabManager::FRequireClosedTab(), NewTab.ToSharedRef());
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
