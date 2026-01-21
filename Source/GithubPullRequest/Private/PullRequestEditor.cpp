#include "PullRequestEditor.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Widgets/Input/SComboBox.h"
#include "EditorStyleSet.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "ISourceControlOperation.h"
#include "SourceControlOperations.h"
#include "ISourceControlProvider.h"
#include "ISourceControlModule.h"

#include "SGitCommitWidget.h"
#include "SGitListFileChangedWidget.h"
#include "SPullRequestEditor.h"

#include "GithubApi.h"

#define LOCTEXT_NAMESPACE "SPullRequestsEditor"
static const FName GithubPR_List_TabId = FName(TEXT("GithubPR_List_TabId"));

//TSharedRef<ITableRow> SPullRequestEditor::GenerateFileRowWidget(TSharedPtr<FFileItem> InItem, const TSharedRef<class STableViewBase>& OwnerTable)
//{
//    static const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 12);
//
//    const FSlateBrush* IconSamples = FAppStyle::Get().GetBrush(TEXT("Graph.StateNode.Icon"));
//
//    if (InItem->status == "added")
//    {
//        IconSamples = FAppStyle::Get().GetBrush(TEXT("SourceControl.Add"));
//    }
//    else if (InItem->status == "modified")
//    {
//        IconSamples = FAppStyle::Get().GetBrush(TEXT("SourceControl.Edit"));
//    }
//    else if (InItem->status == "renamed")
//    {
//        IconSamples = FAppStyle::Get().GetBrush(TEXT("SourceControl.Integrate"));
//    }
//    return SNew(STableRow<TSharedPtr<FPullRequestInformation>>, OwnerTable)
//        [
//            SNew(SBox)
//            .Padding(8)
//        [
//            SNew(SHorizontalBox)
//            + SHorizontalBox::Slot()
//        .Padding(0, 0, 4, 0)
//        .AutoWidth()
//        [
//            SNew(SImage)
//            .Image(IconSamples)
//        ]
//    + SHorizontalBox::Slot()
//        .FillWidth(1.0f)
//        [
//            SNew(STextBlock)
//            .Text(FText::FromString(InItem->filename))
//        .Font(Font)
//        ]
//        ]
//        ];
//}


//void SPullRequestEditor::OnFileDoubleClicked(TSharedPtr<FFileItem> Entry)
//{
//    if (OpenDiffStatus.bIsOpening)
//        return;
//
//    OpenDiffStatus.bIsOpening = true;
//    OpenDiffStatus.Item = Entry;
//    OpenDiffStatus.NumFileDownload = 0;
//    UGithubApi::DownloadFile(FOnFileDownloadComplete::CreateLambda([this](const FString& GamePath, int Code, const FString& Content)
//        {
//            OpenDiffStatus.bHeadVersionDownloadSuccess = !GamePath.IsEmpty();
//            OpenDiffStatus.HeadVersionFile = GamePath;
//            OpenDiffStatus.NumFileDownload++;
//            if (OpenDiffStatus.NumFileDownload == 2)
//            {
//                OpenDiff();
//            }
//
//        }), Entry->filename, Entry->PRInfo->head.sha);
//
//    UGithubApi::DownloadFile(FOnFileDownloadComplete::CreateLambda([this](const FString& GamePath, int Code, const FString& Content)
//        {
//            OpenDiffStatus.bBaseVersionDownloadSuccess = !GamePath.IsEmpty();
//            OpenDiffStatus.BaseVersionFile = GamePath;
//            OpenDiffStatus.NumFileDownload++;
//            if (OpenDiffStatus.NumFileDownload == 2)
//            {
//                OpenDiff();
//            }
//
//        }), Entry->filename, Entry->PRInfo->base.sha);
//}
//
//void SPullRequestEditor::OpenDiff()
//{
//    if (!OpenDiffStatus.bIsOpening)
//        return;
//    UPackage* BaseTempPkg = nullptr;
//    UPackage* HeadTempPkg = nullptr;
//    UObject* HeadAsset = nullptr;
//    UObject* BaseAsset = nullptr;
//    FString AssetName = FPaths::GetBaseFilename(OpenDiffStatus.Item->filename, true);
//    if (OpenDiffStatus.bHeadVersionDownloadSuccess)
//    {
//        HeadTempPkg = LoadPackage(NULL, *OpenDiffStatus.HeadVersionFile, LOAD_ForDiff | LOAD_DisableCompileOnLoad);
//        HeadAsset = FindObject<UObject>(HeadTempPkg, *AssetName);
//        OpenedFiles.AddUnique(HeadTempPkg);
//    }
//
//    if (OpenDiffStatus.bBaseVersionDownloadSuccess)
//    {
//        BaseTempPkg = LoadPackage(NULL, *OpenDiffStatus.BaseVersionFile, LOAD_ForDiff | LOAD_DisableCompileOnLoad);
//        BaseAsset = FindObject<UObject>(BaseTempPkg, *AssetName);
//        OpenedFiles.AddUnique(BaseTempPkg);
//    }
//
//    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
//    if (HeadAsset != NULL && BaseAsset != NULL)
//    {
//        FRevisionInfo OldRevision = { OpenDiffStatus.Item->PRInfo->base.sha, 0, {} };
//        FRevisionInfo CurrentRevision = { OpenDiffStatus.Item->PRInfo->head.sha, 1, {}};
//        AssetToolsModule.Get().DiffAssets(BaseAsset, HeadAsset, OldRevision, CurrentRevision);
//    }
//    else if (HeadAsset != NULL)
//    {
//        AssetToolsModule.Get().OpenEditorForAssets({HeadAsset});
//    }
//    OpenDiffStatus.bIsOpening = false;
//}




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
        , FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.Refresh")
    );

    TAttribute<FText> TooltipAttribute;
    TooltipAttribute.BindRaw(this, &SPullRequestsEditor::GetCleanCacheTooltip);
    ToolBarBuilder.AddToolBarButton(
        FUIAction(
            FExecuteAction::CreateSP(this, &SPullRequestsEditor::CleanCache)
        )
        , NAME_None
        , LOCTEXT("SPullRequestsEditor", "Clean cache")
        ,  TooltipAttribute
        , FSlateIcon(FAppStyle::Get().GetStyleSetName(), "ContentReference.Clear")
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
                    .Padding(0, 0, 0, 10)
                    [
                        SNew(SVerticalBox)
                        // State filter
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0, 0, 0, 8)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0, 0, 0, 4)
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("StateFilterLabel", "State"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
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
                        // Author filter
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0, 0, 0, 4)
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("AuthorFilterLabel", "Author"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SAssignNew(AuthorComboBox, SComboBox<TSharedPtr<FString>>)
                                .OptionsSource(&AuthorOptions)
                                .OnSelectionChanged(this, &SPullRequestsEditor::OnAuthorChanged)
                                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Author)
                                    {
                                        return SNew(STextBlock).Text(FText::FromString(*Author));
                                    })
                                .Content()
                                [
                                    SNew(STextBlock).Text(this, &SPullRequestsEditor::GetSelectedAuthorAsText)
                                ]
                            ]
                        ]
                    ]
                    + SScrollBox::Slot()
                    [
					    SAssignNew(Widget, SListView<TSharedPtr<FPullRequestInformation>>)
					    .ListItemsSource(&PullRequestItems)
				        //.ItemHeight(20)
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

TSharedRef<ITableRow> SPullRequestsEditor::GenerateCategoryRowWidget(TSharedPtr<FPullRequestInformation> InItem, const TSharedRef<class STableViewBase>& OwnerTable)
{
    return SNew(SPullRequestRow, OwnerTable, InItem);
}

void SPullRequestsEditor::OnMarkerListDoubleClicked(TSharedPtr<FPullRequestInformation> Entry)
{
    TSharedPtr<SDockTab> NewTab = SNew(SDockTab).Label(FText::FromString(Entry->title))
        [
            SNew(SPullRequestEditor, MajorTab.ToSharedRef()).PullRequestId(Entry)
        ];

    FName TabId = *FString::Printf(TEXT("GithubPR_%d"), (int32)Entry->number);
    if(TSharedPtr<SDockTab> ActiveTab = TabManager->FindExistingLiveTab(TabId))
    {
        ActiveTab->RequestCloseTab();
    }

    TabManager->InsertNewDocumentTab(GithubPR_List_TabId, TabId, FTabManager::FRequireClosedTab(), NewTab.ToSharedRef());
}

void SPullRequestsEditor::RefreshPullRequest()
{
    auto EnumObject = FindObject<UEnum>(nullptr, TEXT("/Script/GithubPullRequest.EPullRequestQueryState"), true);
    auto State = (EPullRequestQueryState)EnumObject->GetValueByName(QueryState);

    FOnPullRequestListAvailable Delegate;
    AllPullRequestItems.Empty();
    PullRequestItems.Empty();
    int Page = 1;
    Delegate = FOnPullRequestListAvailable::CreateLambda([this, Delegate, State, Page](const TArray<FPullRequestInformation>& ListPullRequest, int Code, const FString& Content)
    {
        if (ListPullRequest.IsEmpty())
        {
            return;
        }
        for (auto& Item : ListPullRequest)
        {
            AllPullRequestItems.Add(MakeShared<FPullRequestInformation>(Item));
        }
        UGithubApi::GetPullRequests(Delegate, State, Page + 1);

        UpdateAuthorList();
        ApplyFilters();
    });

    UGithubApi::GetPullRequests(Delegate, State, Page);
}

void SPullRequestsEditor::UpdateAuthorList()
{
    AuthorOptions.Empty();
    AuthorOptions.Add(MakeShared<FString>(TEXT("All Authors")));

    TSet<FString> UniqueAuthors;
    for (const auto& Item : AllPullRequestItems)
    {
        if (!Item->user.login.IsEmpty())
        {
            UniqueAuthors.Add(Item->user.login);
        }
    }

    for (const FString& Author : UniqueAuthors)
    {
        AuthorOptions.Add(MakeShared<FString>(Author));
    }

    if (AuthorComboBox.IsValid())
    {
        AuthorComboBox->RefreshOptions();
    }
}

void SPullRequestsEditor::ApplyFilters()
{
    PullRequestItems.Empty();

    for (const auto& Item : AllPullRequestItems)
    {
        bool bPassesFilter = true;

        // Apply author filter
        if (SelectedAuthor.IsValid() && *SelectedAuthor != TEXT("All Authors"))
        {
            if (Item->user.login != *SelectedAuthor)
            {
                bPassesFilter = false;
            }
        }

        if (bPassesFilter)
        {
            PullRequestItems.Add(Item);
        }
    }

    if (Widget.IsValid())
    {
        Widget->RequestListRefresh();
    }
}

void SPullRequestsEditor::CleanCache()
{
    IFileManager& FileManager = IFileManager::Get();
    TArray<FString> Files;
    auto CacheDir = FPaths::DiffDir();
    FileManager.FindFiles(Files, *CacheDir);
    for (auto& File : Files)
    {
        if (!FileManager.Delete(*FPaths::Combine(CacheDir, File)))
        {
            UE_LOG(LogTemp, Warning, TEXT("Can not delete file: %s"), *File);
        }
    }

}

void SPullRequestsEditor::OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType)
{
}

void SPullRequestsEditor::OnStateChanged(FName InItem, ESelectInfo::Type InSeletionInfo)
{
    QueryState = InItem;
    RefreshPullRequest();
}

void SPullRequestsEditor::OnAuthorChanged(TSharedPtr<FString> InItem, ESelectInfo::Type InSeletionInfo)
{
    SelectedAuthor = InItem;
    ApplyFilters();
}

FText SPullRequestsEditor::GetSelectedStateAsText() const
{
    return FText::FromName(QueryState);
}

FText SPullRequestsEditor::GetSelectedAuthorAsText() const
{
    if (SelectedAuthor.IsValid())
    {
        return FText::FromString(*SelectedAuthor);
    }
    return FText::FromString(TEXT("All Authors"));
}

FText SPullRequestsEditor::GetCleanCacheTooltip() const
{
    auto CacheDir = FPaths::DiffDir();
    IFileManager& FileManager = IFileManager::Get();
    TArray<FString> Files;
    FileManager.FindFiles(Files, *CacheDir);

    return FText::FromString(FString::Printf(TEXT("Remove cache in save dir (%d files)"), Files.Num()));
}

void SPullRequestRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, TSharedPtr<FPullRequestInformation> InItem)
{
    Item = InItem;
    UGithubApi::DownloadAvatar(FOnFileDownloadComplete::CreateLambda([this](const FString& GamePath, int Code, const FString& InContent)
        {
            if (GamePath.IsEmpty())
                return;
            Avatar = MakeShared<FSlateDynamicImageBrush>(FSlateDynamicImageBrush(FName(GamePath), FVector2D(16, 16)));
        }),
        Item->user.avatar_url, Item->user.id + ".png");
    SMultiColumnTableRow<TSharedPtr<FPullRequestInformation>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef<SWidget> SPullRequestRow::GenerateWidgetForColumn(const FName& ColumnName)
{
    static const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 12);

    const FSlateBrush* IconSamples = FAppStyle::Get().GetBrush(TEXT("Graph.StateNode.Icon"));
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
                    .Text(FText::FromString(Item->title))
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
                        .Text(FText::FromString(Item->user.login))
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
