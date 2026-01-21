// Fill out your copyright notice in the Description page of Project Settings.


#include "SAddFileComment.h"
#include "Widgets/Text/SMultiLineEditableText.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "SlateOptMacros.h"

#define LOCTEXT_NAMESPACE "SPullRequestsEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SAddFileComment::Construct(const FArguments& InArgs)
{
	CommitFile = InArgs._CommitFile.Get();
    PRInfo = InArgs._PullRequestInfo.Get();
    CommitSha = InArgs._CommitSha.Get();

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
                            // Button text area
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                .Padding(0, 0, 10, 0)
                                [
                                    SNew(SButton)
                                    .Text(FText::FromString("Add Single Comment"))
                                    .OnClicked(this, &SAddFileComment::OnAddSingleComment)
                                ]
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                .Padding(10, 0, 0, 0)
                                [
                                    SNew(SButton)
                                    .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
                                    .Text(FText::FromString("Start A Review"))
                                    .OnClicked(this, &SAddFileComment::OnStartReview)
                                ]

                            ]
                        ]
                    ]
                ]
            ]
        ]
	];
	
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


FReply SAddFileComment::OnAddSingleComment()
{
	FString CommentText = CommentWidget->GetText().ToString();
    if (CommentText.IsEmpty())
    {
        FNotificationInfo Info(FText::FromString(TEXT("Commment can not empty")));
        Info.bFireAndForget = true;
        Info.ExpireDuration = 5.0f;
        Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.Warning"));
        FSlateNotificationManager::Get().AddNotification(Info);
		return FReply::Handled();
    }
	if (!PRInfo.IsValid() || !CommitFile.IsValid())
	{
		FNotificationInfo Info(FText::FromString(TEXT("Pull Request info is not valid")));
		Info.bFireAndForget = true;
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.Warning"));
		FSlateNotificationManager::Get().AddNotification(Info);
        return FReply::Handled();
    }

    UGithubApi::CreateComment(FOnCreateReviewComplete::CreateLambda([SharedThis = AsWeak()](int Code, const FString& Content)
        {
			FNotificationInfo Info(FText::FromString(TEXT("Add comment success")));
			Info.bFireAndForget = true;
			Info.ExpireDuration = 5.0f;
			Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.Success"));
			FSlateNotificationManager::Get().AddNotification(Info);

            if (auto Widget = StaticCastWeakPtr<SAddFileComment>(SharedThis).Pin())
            {
                Widget->CommentWidget->SetText(FText::GetEmpty());
            }
        }), PRInfo->number, CommentText, CommitSha, CommitFile->filename);
	return FReply::Handled();
}

FReply SAddFileComment::OnStartReview()
{
    FString CommentText = CommentWidget->GetText().ToString();
    if (CommentText.IsEmpty())
    {
        FNotificationInfo Info(FText::FromString(TEXT("Commment can not empty")));
        Info.bFireAndForget = true;
        Info.ExpireDuration = 5.0f;
        Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.Warning"));
        FSlateNotificationManager::Get().AddNotification(Info);
        return FReply::Handled();
    }
    if (!PRInfo.IsValid())
    {
        FNotificationInfo Info(FText::FromString(TEXT("Pull Request info is not valid")));
        Info.bFireAndForget = true;
        Info.ExpireDuration = 5.0f;
        Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.Warning"));
        FSlateNotificationManager::Get().AddNotification(Info);
        return FReply::Handled();
    }

    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE