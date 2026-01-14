// Fill out your copyright notice in the Description page of Project Settings.


#include "SGitCommitWidget.h"
#include "SGitListFileChangedWidget.h"

#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGitCommitWidget::Construct(const FArguments& InArgs)
{
	CommitData = InArgs._CommitData.Get();

    // Get short SHA (first 7 characters)
    FString ShortSha = CommitData.sha.Left(7);

    // Get author name - prefer login, fallback to commit author name
    FString AuthorName = CommitData.author.login.IsEmpty() ? CommitData.commit.author.name : CommitData.author.login;

    // Get first line of commit message as title
    FString CommitTitle = CommitData.commit.message;
    int32 NewlineIndex;
    if (CommitTitle.FindChar('\n', NewlineIndex))
    {
        CommitTitle = CommitTitle.Left(NewlineIndex);
    }

    // Parse and format date
    FString DateStr = CommitData.commit.author.date;
    FDateTime DateTime;
    FString FormattedDate = DateStr;
    if (FDateTime::ParseIso8601(*DateStr, DateTime))
    {
        FormattedDate = DateTime.ToString(TEXT("%b %d, %Y"));
    }

    // Get author initials for avatar placeholder
    FString Initials;
    if (!AuthorName.IsEmpty())
    {
        TArray<FString> NameParts;
        AuthorName.ParseIntoArray(NameParts, TEXT(" "), true);
        if (NameParts.Num() > 0)
        {
            Initials = NameParts[0].Left(1).ToUpper();
            if (NameParts.Num() > 1)
            {
                Initials += NameParts.Last().Left(1).ToUpper();
            }
        }
    }

    // GitHub-style green color for commit indicator
    FLinearColor CommitColor = FLinearColor(0.24f, 0.54f, 0.25f, 1.0f); // Green similar to GitHub

    SAssignNew(CommitHeader, SBorder)
        .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
        .BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 1.0f))
        .Padding(0)
        [
            SNew(SHorizontalBox)
                // Left color bar (GitHub-style commit indicator)
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SBox)
                        .WidthOverride(4)
                        [
                            SNew(SBorder)
                                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                                .BorderBackgroundColor(CommitColor)
                        ]
                ]
                // Main content
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(FMargin(12, 8))
                [
                    SNew(SHorizontalBox)
                        // Avatar placeholder (circle with initials)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Top)
                        .Padding(0, 0, 12, 0)
                        [
                            SNew(SBox)
                                .WidthOverride(32)
                                .HeightOverride(32)
                                [
                                    SNew(SBorder)
                                        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                                        .BorderBackgroundColor(CommitColor)
                                        .HAlign(HAlign_Center)
                                        .VAlign(VAlign_Center)
                                        [
                                            SNew(STextBlock)
                                                .Text(FText::FromString(Initials))
                                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                                                .ColorAndOpacity(FLinearColor::White)
                                        ]
                                ]
                        ]
                        // Commit info
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .VAlign(VAlign_Center)
                        [
                            SNew(SVerticalBox)
                                // Commit title (first line of message)
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromString(CommitTitle))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                                        .AutoWrapText(true)
                                ]
                                // Author and date
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0, 4, 0, 0)
                                [
                                    SNew(SHorizontalBox)
                                        + SHorizontalBox::Slot()
                                        .AutoWidth()
                                        [
                                            SNew(STextBlock)
                                                .Text(FText::FromString(AuthorName))
                                                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                                        ]
                                        + SHorizontalBox::Slot()
                                        .AutoWidth()
                                        .Padding(8, 0, 0, 0)
                                        [
                                            SNew(STextBlock)
                                                .Text(FText::FromString(FormattedDate))
                                                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                                        ]
                                ]
                        ]
                        // SHA
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(12, 0, 0, 0)
                        [
                            SNew(SBorder)
                                .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
                                .Padding(FMargin(8, 4))
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromString(ShortSha))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
                                        .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                                ]
                        ]
                ]
        ];

        auto ParentSha = CommitData.parents.IsEmpty() ? "" : CommitData.parents[0].sha;

        SAssignNew(FilesContainer ,SGitListFileChangedWidget)
            .Head(CommitData.sha)
            .Parent(ParentSha)
            .ShowSearchBox(false);

        SAssignNew(CommitDetail, SVerticalBox)
        +SVerticalBox::Slot()
        .AutoHeight()
        [
            FilesContainer.ToSharedRef()
        ];

        if (CommitData.files.IsEmpty())
        {
            CommitDetail->AddSlot()
            .AutoHeight()
            .Padding(20, 2)
            [
                SAssignNew(EmptyFileText, STextBlock)
                .Text(FText::FromString(TEXT("No files or failed to load")))
                .ColorAndOpacity(FSlateColor(FLinearColor::Gray))
            ];
            UGithubApi::GetCommitDetails(FOnCommitDetailsAvailable::CreateRaw(this, &SGitCommitWidget::OnCommitDetailsAvailable), CommitData.sha);
        }
    FilesContainer->SetListFiles(CommitData.files);
    TSharedPtr<SExpandableArea> ExpandableCommit;
    SAssignNew(ExpandableCommit, SExpandableArea)
        .InitiallyCollapsed(true)
        .BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
        .BodyBorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
        .BodyBorderBackgroundColor(FLinearColor::White)
        .Padding(0.0f)
        .HeaderContent()
        [
            CommitHeader.ToSharedRef()
        ]
        .BodyContent()
        [
            CommitDetail.ToSharedRef()
        ];

	ChildSlot
	[
        ExpandableCommit.ToSharedRef()
	];
	
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SGitCommitWidget::OnCommitDetailsAvailable(const FCommitFullData& Commit, int Code, const FString& Content)
{
    if (Code != 200)
        return;
    CommitData = Commit;
    EmptyFileText->SetVisibility(CommitData.files.IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed);

    FilesContainer->SetListFiles(CommitData.files);
}