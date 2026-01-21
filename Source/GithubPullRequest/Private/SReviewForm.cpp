// Fill out your copyright notice in the Description page of Project Settings.


#include "SReviewForm.h"

#include "SlateOptMacros.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SReviewForm"

void SReviewForm::Construct(const FArguments& InArgs)
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
                                                                        .IsChecked(this, &SReviewForm::IsReviewTypeSelected, FString(TEXT("COMMENT")))
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
                                                                        .IsChecked(this, &SReviewForm::IsReviewTypeSelected, FString(TEXT("APPROVE")))
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
                                                                        .IsChecked(this, &SReviewForm::IsReviewTypeSelected, FString(TEXT("REQUEST_CHANGES")))
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
                                                        .OnClicked(this, &SReviewForm::OnSubmitReview)
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

void SReviewForm::OnReviewTypeChanged(FString ReviewType)
{
    SelectedReviewType = ReviewType;
}

ECheckBoxState SReviewForm::IsReviewTypeSelected(FString ReviewType) const
{
    return SelectedReviewType == ReviewType ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

FReply SReviewForm::OnSubmitReview()
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

#undef LOCTEXT_NAMESPACE