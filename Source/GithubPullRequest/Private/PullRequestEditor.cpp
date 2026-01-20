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

#include "GithubApi.h"

#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "PackageTools.h"

#define LOCTEXT_NAMESPACE "SPullRequestsEditor"

static const FName GithubPR_ListFile_TabId = FName(TEXT("GithubPR_Listfiles_TabId"));
static const FName GithubPR_Description_TabId = FName(TEXT("GithubPR_Description_TabId"));
static const FName GithubPR_Commits_TabId = FName(TEXT("GithubPR_Commits_TabId"));
static const FName GithubPR_List_TabId = FName(TEXT("GithubPR_List_TabId"));

void SReviewChanges::Construct(const FArguments& InArgs)
{
    PullRequestData = InArgs._PullRequestId;

    // GitHub-style colors
    FLinearColor CommentColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);      // Gray
    FLinearColor ApproveColor = FLinearColor(0.24f, 0.54f, 0.25f, 1.0f);   // Green
    FLinearColor RequestChangesColor = FLinearColor(0.82f, 0.15f, 0.15f, 1.0f); // Red

    // Border color
    FLinearColor BorderColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);
    FLinearColor ShadowColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);
    FLinearColor BackgroundColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);

    ChildSlot
    [
        // Shadow layer
        SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(ShadowColor)
        .Padding(FMargin(4, 4, 6, 6))
        [
            // Main border
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(BorderColor)
            .Padding(1)
            [
                // Background
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(BackgroundColor)
                .Padding(0)
                [
                    SNew(SBox)
                    .MinDesiredWidth(380)
                    [
                        SNew(SVerticalBox)
                        // Header
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(SBorder)
                            .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
                            .Padding(FMargin(16, 12))
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("FinishReview", "Finish your review"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                            ]
                        ]
                // Comment text area
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(16, 12)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 0, 0, 8)
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                        .Padding(8)
                        [
                            SNew(SBox)
                            .MinDesiredHeight(80)
                            [
                                SAssignNew(CommentWidget, SMultiLineEditableText)
                                .HintText(LOCTEXT("LeaveComment", "Leave a comment"))
                                .AutoWrapText(true)
                            ]
                        ]
                    ]
                ]
                // Review options
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(16, 0, 16, 12)
                [
                    SNew(SVerticalBox)
                    // Comment option
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 0, 0, 8)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Top)
                        .Padding(0, 2, 8, 0)
                        [
                            SNew(SCheckBox)
                            .Style(&FCoreStyle::Get().GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
                            .IsChecked(this, &SReviewChanges::IsReviewTypeSelected, FString(TEXT("COMMENT")))
                            .OnCheckStateChanged_Lambda([this](ECheckBoxState State)
                            {
                                if (State == ECheckBoxState::Checked)
                                    OnReviewTypeChanged(TEXT("COMMENT"));
                            })
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("CommentOption", "Comment"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("CommentDesc", "Submit general feedback without explicit approval."))
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                                .AutoWrapText(true)
                            ]
                        ]
                    ]
                    // Approve option
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 0, 0, 8)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Top)
                        .Padding(0, 2, 8, 0)
                        [
                            SNew(SCheckBox)
                            .Style(&FCoreStyle::Get().GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
                            .IsChecked(this, &SReviewChanges::IsReviewTypeSelected, FString(TEXT("APPROVE")))
                            .OnCheckStateChanged_Lambda([this](ECheckBoxState State)
                            {
                                if (State == ECheckBoxState::Checked)
                                    OnReviewTypeChanged(TEXT("APPROVE"));
                            })
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Top)
                        .Padding(0, 0, 8, 0)
                        [
                            SNew(SBox)
                            .WidthOverride(16)
                            .HeightOverride(16)
                            [
                                SNew(SBorder)
                                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                                .BorderBackgroundColor(ApproveColor)
                                .HAlign(HAlign_Center)
                                .VAlign(VAlign_Center)
                                [
                                    SNew(STextBlock)
                                    .Text(FText::FromString(TEXT("\u2713")))
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                                    .ColorAndOpacity(FLinearColor::White)
                                ]
                            ]
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("ApproveOption", "Approve"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("ApproveDesc", "Submit feedback and approve merging these changes."))
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                                .AutoWrapText(true)
                            ]
                        ]
                    ]
                    // Request changes option
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Top)
                        .Padding(0, 2, 8, 0)
                        [
                            SNew(SCheckBox)
                            .Style(&FCoreStyle::Get().GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
                            .IsChecked(this, &SReviewChanges::IsReviewTypeSelected, FString(TEXT("REQUEST_CHANGES")))
                            .OnCheckStateChanged_Lambda([this](ECheckBoxState State)
                            {
                                if (State == ECheckBoxState::Checked)
                                    OnReviewTypeChanged(TEXT("REQUEST_CHANGES"));
                            })
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Top)
                        .Padding(0, 0, 8, 0)
                        [
                            SNew(SBox)
                            .WidthOverride(16)
                            .HeightOverride(16)
                            [
                                SNew(SBorder)
                                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                                .BorderBackgroundColor(RequestChangesColor)
                                .HAlign(HAlign_Center)
                                .VAlign(VAlign_Center)
                                [
                                    SNew(STextBlock)
                                    .Text(FText::FromString(TEXT("\u2715")))
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                                    .ColorAndOpacity(FLinearColor::White)
                                ]
                            ]
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("RequestChangesOption", "Request changes"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("RequestChangesDesc", "Submit feedback that must be addressed before merging."))
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                                .AutoWrapText(true)
                            ]
                        ]
                    ]
                ]
                // Submit button
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(16, 0, 16, 16)
                .HAlign(HAlign_Right)
                [
                    SNew(SButton)
                    .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
                    .OnClicked(this, &SReviewChanges::OnSubmitReview)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("SubmitReview", "Submit review"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                    ]
                ]
            ]
        ]
    ]
    ]
    ];
}

void SReviewChanges::OnReviewTypeChanged(FString ReviewType)
{
    SelectedReviewType = ReviewType;
}

ECheckBoxState SReviewChanges::IsReviewTypeSelected(FString ReviewType) const
{
    return SelectedReviewType == ReviewType ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

FReply SReviewChanges::OnSubmitReview()
{
    if (!PullRequestData.IsValid())
        return FReply::Unhandled();

    FString CommentText = CommentWidget->GetText().ToString();

    // For REQUEST_CHANGES, comment is required
    if (SelectedReviewType == TEXT("REQUEST_CHANGES") && CommentText.IsEmpty())
    {
        FNotificationInfo Info(LOCTEXT("CommentRequired", "A comment is required when requesting changes."));
        Info.ExpireDuration = 3.0f;
        Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.Warning"));
        FSlateNotificationManager::Get().AddNotification(Info);
        return FReply::Handled();
    }

    auto OnComplete = FOnCreateReviewComplete::CreateLambda([](int Code, const FString& Content)
    {
        if (Code == 200)
        {
            FNotificationInfo Info(LOCTEXT("ReviewSubmitted", "Review submitted successfully!"));
            Info.ExpireDuration = 3.0f;
            FSlateNotificationManager::Get().AddNotification(Info);
        }
        else
        {
            FNotificationInfo Info(LOCTEXT("ReviewFailed", "Failed to submit review."));
            Info.ExpireDuration = 3.0f;
            FSlateNotificationManager::Get().AddNotification(Info);
        }
    });

    UGithubApi::CreateReview(OnComplete, PullRequestData->number, SelectedReviewType, CommentText);

    // Clear the comment after submission
    CommentWidget->SetText(FText::GetEmpty());

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
    return SNew(SReviewChanges).PullRequestId(PullRequestData);
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
                    auto ListItem = MakeShared<FCommitItem>();
                    ListItem->CommitInfo = Item;
                    ListItem->PRInfo = PullRequestData;
                    ListCommitItems.Add(ListItem);

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
                auto ListItem = MakeShared<FCommitItem>();
                ListItem->CommitInfo = Item;
                ListItem->PRInfo = PullRequestData;
                ListCommitItems.Add(ListItem);
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
            ListFileWidget.ToSharedRef()
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

void SPullRequestEditor::OnCheckoutComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
    if (InResult == ECommandResult::Succeeded)
    {
        FNotificationInfo Info(LOCTEXT("CheckoutCompleteSuccess", "Checkout completed successfully."));
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
    }
    else
    {
        FNotificationInfo Info(LOCTEXT("CheckoutCompleteFailed", "Checkout failed."));
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
    }
}

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
