// Fill out your copyright notice in the Description page of Project Settings.


#include "SPullRequestEditor.h"
#include "SReviewForm.h"
#include "SGitCommitWidget.h"
#include "SGitListFileChangedWidget.h"

#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/SRichTextBlock.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "SlateOptMacros.h"

#include "GithubApi.h"

#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "PackageTools.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SPullRequestsEditor"


static const FName GithubPR_ListFile_TabId = FName(TEXT("GithubPR_Listfiles_TabId"));
static const FName GithubPR_Description_TabId = FName(TEXT("GithubPR_Description_TabId"));
static const FName GithubPR_Commits_TabId = FName(TEXT("GithubPR_Commits_TabId"));
static const FName GithubPR_Activities_TabId = FName(TEXT("GithubPR_Activities_TabId"));

void SPullRequestEditor::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab)
{
    PullRequestData = InArgs._PullRequestId;

    FToolBarBuilder ToolBarBuilder(TSharedPtr< const FUICommandList >(), FMultiBoxCustomization::None);
    ToolBarBuilder.AddComboButton(FUIAction(), FOnGetContent::CreateRaw(this, &SPullRequestEditor::MakeReviewChanges),
        LOCTEXT("SPullRequestsEditor", "Review changes"),
        LOCTEXT("SPullRequestsEditor", "Comments/Approve/Request change"),
        FSlateIcon(FAppStyle::Get().GetStyleSetName(), "BlueprintDiff.ToolbarIcon"));
    ToolBarBuilder.AddToolBarButton(
        FUIAction(
            FExecuteAction::CreateSP(this, &SPullRequestEditor::Checkout)
        )
        , NAME_None
        , LOCTEXT("SPullRequestsEditor", "Checkout")
        , LOCTEXT("SPullRequestsEditor", "Checkout to Pullrequest branch")
        , FSlateIcon(FAppStyle::Get().GetStyleSetName(), "SourceControl.Actions.CheckOut")
    );
    ToolBarBuilder.AddToolBarButton(
        FUIAction(
            FExecuteAction::CreateSP(this, &SPullRequestEditor::Refresh)
        )
        , NAME_None
        , LOCTEXT("SPullRequestsEditor", "Refresh")
        , LOCTEXT("SPullRequestsEditor", "Refresh pull request")
        , FSlateIcon(FAppStyle::Get().GetStyleSetName(), "SourceControl.Actions.Refresh")
    );
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

    TabManager->RegisterTabSpawner(GithubPR_Activities_TabId,
        FOnSpawnTab::CreateRaw(this, &SPullRequestEditor::CreateActiviesTab))
        .SetDisplayName(LOCTEXT("SPullRequestsEditor", "Activities"));

    // Create a default layout for the tab manager, this allows us to open the tabs by default
    const TSharedRef<FTabManager::FLayout> DefaultLayout = FTabManager::NewLayout("GithubPR_Layout")
        ->AddArea(
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Horizontal)
            ->Split
            (
                FTabManager::NewStack()
                ->AddTab(GithubPR_Activities_TabId, ETabState::OpenedTab)
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
                // Header section
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(20, 16, 20, 8)
                [
                    SNew(SVerticalBox)
                        // Title row: Title + PR Number
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromString(PullRequestData->title))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
                                ]
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(10, 0, 0, 0)
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromString(FString::Printf(TEXT("#%d"), PullRequestData->number)))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 24))
                                        .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                                ]
                        ]
                        // Info row: State badge + Author + Branch info
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0, 8, 0, 0)
                        [
                            SNew(SHorizontalBox)
                                // State badge
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                [
                                    SAssignNew(Status,SBorder)
                                        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                                        //.BorderBackgroundColor(StateColor)
                                        .Padding(FMargin(10, 4))
                                        [
                                            SAssignNew(StatusText, STextBlock)
                                                //.Text(FText::FromString(StateText))
                                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                                                .ColorAndOpacity(FLinearColor::White)
                                        ]
                                ]
                                // Author info
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(12, 0, 0, 0)
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromString(FString::Printf(TEXT("%s wants to merge into"), *PullRequestData->user.login)))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                                        .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                                ]
                                // Base branch
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(6, 0, 0, 0)
                                [
                                    SNew(SBorder)
                                        .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
                                        .Padding(FMargin(6, 2))
                                        [
                                            SNew(STextBlock)
                                                .Text(FText::FromString(PullRequestData->base.ref))
                                                .Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
                                                .ColorAndOpacity(FLinearColor(0.3f, 0.5f, 0.8f, 1.0f))
                                        ]
                                ]
                                // Arrow
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(6, 0)
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromString(TEXT("\u2190")))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                                        .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                                ]
                                // Head branch
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                [
                                    SNew(SBorder)
                                        .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
                                        .Padding(FMargin(6, 2))
                                        [
                                            SNew(STextBlock)
                                                .Text(FText::FromString(PullRequestData->head.ref))
                                                .Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
                                                .ColorAndOpacity(FLinearColor(0.3f, 0.5f, 0.8f, 1.0f))
                                        ]
                                ]
                        ]
                ]
                // Separator
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(20, 0)
                [
                    SNew(SSeparator)
                ]
                // Toolbar
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(10, 8)
                [
                    Toolbar.ToSharedRef()
                ]
                // Main content
                + SVerticalBox::Slot()
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

    UpdateState();
    UpdateComment();

    PullRequestData->OnCommentShouldUpdate.BindLambda([WeakThis = AsWeak()]()
    {
        if (auto PullRequestEditor = StaticCastWeakPtr<SPullRequestEditor>(WeakThis).Pin())
        {
            PullRequestEditor->UpdateComment();
        }
    });

    UGithubApi::ListIssueComment(FOnListIssueCommentAvailable::CreateLambda([WeakThis = AsWeak()](const TArray<FIssueComment>& ListComment, int Code, const FString& Content)
        {
            UE_LOG(LogTemp, Log, TEXT("%d"), ListComment.Num());
        }), PullRequestData->number);
}

SPullRequestEditor::~SPullRequestEditor()
{
    TArray<UPackage*> PackagesToUnload;

    for (auto& File : OpenedFiles)
    {
        UPackage* Package = File.Get();
        if (!IsValid(Package))
            continue;
        // Detach the linkers of any loaded packages so that SCC can overwrite the files...
        if (!Package->IsFullyLoaded())
        {
            FlushAsyncLoading();
            Package->FullyLoad();
        }
        ResetLoaders(Package);
        PackagesToUnload.Add(Package);
    }

    // Actually unload the packages to free memory
    if (PackagesToUnload.Num() > 0)
    {
        FText ErrorMessage;
        UPackageTools::UnloadPackages(PackagesToUnload, ErrorMessage, /*bUnloadDirtyPackages=*/true);
    }

    OpenedFiles.Empty();
}

TSharedRef<SWidget> SPullRequestEditor::MakeReviewChanges()
{
    return SNew(SReviewForm).PullRequestId(PullRequestData);
}

void SPullRequestEditor::UpdateState()
{
    // Determine state color (GitHub-style)
    FLinearColor StateColor;
    FString StateText = PullRequestData->state;
    if (StateText.ToLower() == "open")
    {
        StateColor = FLinearColor(0.24f, 0.54f, 0.25f, 1.0f); // Green
        StateText = "Open";
    }
    else if (StateText.ToLower() == "closed")
    {
        StateColor = FLinearColor(0.82f, 0.15f, 0.15f, 1.0f); // Red
        StateText = "Closed";
    }
    else if (StateText.ToLower() == "merged")
    {
        StateColor = FLinearColor(0.54f, 0.24f, 0.70f, 1.0f); // Purple
        StateText = "Merged";
    }
    else
    {
        StateColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f); // Gray
    }

    Status->SetBorderBackgroundColor(StateColor);
    StatusText->SetText(FText::FromString(StateText));
}

void SPullRequestEditor::UpdateComment()
{
    UGithubApi::ListComment(FOnListCommentAvailable::CreateLambda([WeakThis = AsWeak()](const TArray<FComment>& ListComment, int Code, const FString& Content)
        {
            if (ListComment.IsEmpty())
                return;

            if (auto PullRequestEditor = StaticCastWeakPtr<SPullRequestEditor>(WeakThis).Pin())
            {
                PullRequestEditor->PullRequestData->AllComments = ListComment;
                PullRequestEditor->PullRequestData->OnCommentAvailable.Broadcast();
            }

            // TODO: Query next page
        }), PullRequestData->number);
}

void SPullRequestEditor::Refresh()
{
    // Refresh files list
    UGithubApi::GetFilesInPullRequest(PullRequestData->number, FOnFilesListAvailable::CreateRaw(this, &SPullRequestEditor::OnFilesListAvailable, 1));

    // Refresh commits list
    ListCommitItems.Empty();
    if (ListCommitWidget.IsValid())
    {
        ListCommitWidget->ClearChildren();

        FOnCommitListAvailable OnCommitList;
        int Page = 1;
        TSharedPtr<SVerticalBox> CommitWidgetPtr = ListCommitWidget;
        OnCommitList = FOnCommitListAvailable::CreateLambda([this, Page, OnCommitList, CommitWidgetPtr](const TArray<FCommitFullData>& Commits, int Code, const FString& Content)
            {
                if (Commits.IsEmpty())
                    return;
                for (auto& Item : Commits)
                {
                    ListCommitItems.Add(MakeShared<FCommitFullData>(Item));

                    if (CommitWidgetPtr.IsValid())
                    {
                        CommitWidgetPtr->AddSlot()
                            .AutoHeight()
                            .Padding(2)
                            [
                                CreateCommitRowWidget(Item)
                            ];
                    }
                }
                UGithubApi::ListCommitFromPull(OnCommitList, PullRequestData->number, Page + 1);
            });

        UGithubApi::ListCommitFromPull(OnCommitList, PullRequestData->number, Page);
    }

    UGithubApi::GetPullRequest(FOnPullRequestAvailable::CreateRaw(this, &SPullRequestEditor::UpdateInfo), PullRequestData->number);
}

void SPullRequestEditor::UpdateInfo(const FPullRequestInformation& PullRequest, int Code, const FString& Content)
{
    *PullRequestData = PullRequest;
    DescriptionText->SetText(FText::FromString(PullRequestData->body));
    UpdateState();
}

TSharedRef<SWidget> SPullRequestEditor::CreateCommitRowWidget(const FCommitFullData& CommitData)
{
    return SNew(SGitCommitWidget)
        .CommitData(CommitData)
        .PullRequestInfo(PullRequestData);
}

TSharedRef<SDockTab> SPullRequestEditor::CreateCommitsTab(const FSpawnTabArgs& Args)
{
    SAssignNew(ListCommitWidget, SVerticalBox);
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
            SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    ListCommitWidget.ToSharedRef()
                ]
        ];

    auto DockTab = SNew(SDockTab)
        .TabRole(ETabRole::PanelTab)
        .OnCanCloseTab_Lambda([](){return false;})
        .Label_Lambda([this](){return FText::FromString(FString::Printf(TEXT("Commits (%d)"), ListCommitItems.Num()));})
        [
            NewWidget
        ];

    int Page = 1;
    ListCommitItems.Empty();
    FOnCommitListAvailable OnCommitList;
    TSharedPtr<SVerticalBox> CommitWidgetPtr = ListCommitWidget;
    OnCommitList = FOnCommitListAvailable::CreateLambda([this, Page, OnCommitList, CommitWidgetPtr](const TArray<FCommitFullData>& Commits, int Code, const FString& Content)
        {
            if (Commits.IsEmpty())
                return;
            for (auto& Item : Commits)
            {
                ListCommitItems.Add(MakeShared<FCommitFullData>(Item));
                if (CommitWidgetPtr.IsValid())
                {
                    CommitWidgetPtr->AddSlot()
                        .AutoHeight()
                        .Padding(2)
                        [
                            CreateCommitRowWidget(Item)
                        ];
                }
            }
            UGithubApi::ListCommitFromPull(OnCommitList, PullRequestData->number, Page + 1);

        });

    UGithubApi::ListCommitFromPull(OnCommitList, PullRequestData->number, Page);
    return DockTab;
}

TSharedRef<SDockTab> SPullRequestEditor::CreateListfilesTab(const FSpawnTabArgs& Args)
{
    SAssignNew(ListFileWidget, SGitListFileChangedWidget)
        .Parent(PullRequestData->base.sha)
        .Head(PullRequestData->head.sha)
        .PullRequestInfo(PullRequestData);
    auto DockTab = SNew(SDockTab)
        .OnCanCloseTab_Lambda([](){return false;})
        .TabRole(ETabRole::PanelTab)
        .Label_Lambda([this](){return FText::FromString(FString::Printf(TEXT("Files changed (%d)"), ListFileWidget->GetTotalFile()));})
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [
                ListFileWidget.ToSharedRef()
            ]
        ];

    int Page = 1;
    UGithubApi::GetFilesInPullRequest(PullRequestData->number, FOnFilesListAvailable::CreateRaw(this, &SPullRequestEditor::OnFilesListAvailable, Page));
    return DockTab;
}

void SPullRequestEditor::OnFilesListAvailable(const TArray<FCommitFile>& Files, int Code, const FString& Content, int Page)
{
    if (Files.IsEmpty())
        return;
    ListFileWidget->SetListFiles(Files, true);
    UGithubApi::GetFilesInPullRequest(PullRequestData->number, FOnFilesListAvailable::CreateRaw(this, &SPullRequestEditor::OnFilesListAvailable, Page + 1), 9999, Page + 1);
}

TSharedRef<SDockTab> SPullRequestEditor::CreateDescriptionTab(const FSpawnTabArgs& Args)
{
    auto WidgetTag = SNew(SBox).Padding(10.0f)
        [
            SNew(SBorder)
                .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                .Padding(10.0f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [
                        SNew(SRichTextBlock)
                            .DecoratorStyleSet(&FAppStyle::Get())
                            .Text(FText::FromString(PullRequestData->body))
                            .TextStyle(&FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("Text.Large"))
                    ]
                ]
        ];
    return SNew(SDockTab)
        .OnCanCloseTab_Lambda([](){return false;})
        .TabRole(ETabRole::PanelTab)
        [
            WidgetTag
        ];
}

TSharedRef<SDockTab> SPullRequestEditor::CreateActiviesTab(const FSpawnTabArgs& Args)
{
    auto WidgetTag = SNew(SBox).Padding(10.0f)
        [
            SNew(SBorder)
                .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                .Padding(10.0f)
                [
                    SNew(SRichTextBlock)
                        .DecoratorStyleSet(&FAppStyle::Get())
                        .Text(FText::FromString(PullRequestData->body))
                        .TextStyle(&FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("Text.Large"))
                ]
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

void SPullRequestEditor::Checkout()
{
    if (!PullRequestData.IsValid())
    {
        FNotificationInfo Info(LOCTEXT("CheckoutFailed", "No pull request data available."));
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        return;
    }

    FString BranchName = PullRequestData->head.ref;
    if (BranchName.IsEmpty())
    {
        FNotificationInfo Info(LOCTEXT("CheckoutNoBranch", "No branch name found in pull request."));
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        return;
    }

    // Get the project directory
    FString ProjectDir = FPaths::ProjectDir();

    // Show progress notification
    FNotificationInfo ProgressInfo(FText::Format(LOCTEXT("CheckoutProgress", "Checking out branch: {0}"), FText::FromString(BranchName)));
    ProgressInfo.bFireAndForget = false;
    ProgressInfo.ExpireDuration = 0.0f;
    TSharedPtr<SNotificationItem> ProgressNotification = FSlateNotificationManager::Get().AddNotification(ProgressInfo);
    if (ProgressNotification.IsValid())
    {
        ProgressNotification->SetCompletionState(SNotificationItem::CS_Pending);
    }

    // Get all loaded packages
    TSet<UPackage*> FilteredPackages;
    UPackageTools::GetFilteredPackageList(FilteredPackages);

    TArray<UPackage*> PackagesToUnload;
    for (UPackage* Package : FilteredPackages)
    {
        // Skip engine/editor packages
        if (!Package->GetName().StartsWith(TEXT("/Engine/")) &&
            !Package->GetName().StartsWith(TEXT("/Script/")))
        {
            PackagesToUnload.Add(Package);
        }
    }

    FText ErrorMessage;
    UPackageTools::UnloadPackages(PackagesToUnload, ErrorMessage, /*bUnloadDirtyPackages=*/true);
    CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

    // Run git commands asynchronously
    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, BranchName, ProjectDir, ProgressNotification]()
        {
            FString GitPath = TEXT("git");
            FString Output;
            FString Errors;
            int32 ReturnCode = 0;
            bool bSuccess = true;

            // First, fetch the branch from origin
            FString FetchArgs = FString::Printf(TEXT("fetch origin %s"), *BranchName);
            bSuccess = FPlatformProcess::ExecProcess(*GitPath, *FetchArgs, &ReturnCode, &Output, &Errors, *ProjectDir);

            if (!bSuccess || ReturnCode != 0)
            {
                // Try fetching all
                FetchArgs = TEXT("fetch --all");
                bSuccess = FPlatformProcess::ExecProcess(*GitPath, *FetchArgs, &ReturnCode, &Output, &Errors, *ProjectDir);
            }

            // Then checkout the branch
            if (bSuccess)
            {
                FString CheckoutArgs = FString::Printf(TEXT("checkout %s"), *BranchName);
                bSuccess = FPlatformProcess::ExecProcess(*GitPath, *CheckoutArgs, &ReturnCode, &Output, &Errors, *ProjectDir);

                // If checkout failed, try creating a tracking branch
                if (!bSuccess || ReturnCode != 0)
                {
                    CheckoutArgs = FString::Printf(TEXT("checkout -b %s origin/%s"), *BranchName, *BranchName);
                    bSuccess = FPlatformProcess::ExecProcess(*GitPath, *CheckoutArgs, &ReturnCode, &Output, &Errors, *ProjectDir);
                }
            }

            // Update notification on game thread
            bool bFinalSuccess = bSuccess && ReturnCode == 0;
            FString FinalBranchName = BranchName;
            FString FinalErrors = Errors;

            AsyncTask(ENamedThreads::GameThread, [ProgressNotification, bFinalSuccess, FinalBranchName, FinalErrors]()
                {
                    if (ProgressNotification.IsValid())
                    {
                        if (bFinalSuccess)
                        {
                            ProgressNotification->SetText(FText::Format(LOCTEXT("CheckoutSuccess", "Successfully checked out: {0}"), FText::FromString(FinalBranchName)));
                            ProgressNotification->SetCompletionState(SNotificationItem::CS_Success);
                        }
                        else
                        {
                            FString ErrorMsg = FinalErrors.IsEmpty() ? TEXT("Unknown error") : FinalErrors;
                            ProgressNotification->SetText(FText::Format(LOCTEXT("CheckoutError", "Failed to checkout: {0}"), FText::FromString(ErrorMsg)));
                            ProgressNotification->SetCompletionState(SNotificationItem::CS_Fail);
                        }
                        ProgressNotification->ExpireAndFadeout();
                    }
                });
        });
}

#undef LOCTEXT_NAMESPACE