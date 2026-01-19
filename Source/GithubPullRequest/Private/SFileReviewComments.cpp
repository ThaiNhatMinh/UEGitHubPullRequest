// Fill out your copyright notice in the Description page of Project Settings.


#include "SFileReviewComments.h"
#include "Widgets/Text/SMultiLineEditableText.h"

#include "SlateOptMacros.h"

#define LOCTEXT_NAMESPACE "SPullRequestsEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SFileReviewComments::Construct(const FArguments& InArgs)
{
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
                        // Button text area
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(SHorizontalBox)
                        ]
                    ]
                ]
            ]
        ]
	];
	
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE