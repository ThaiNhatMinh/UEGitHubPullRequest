// Fill out your copyright notice in the Description page of Project Settings.


#include "SGithubCommentWidget.h"
#include "Widgets/Text/SRichTextBlock.h"

#include "SlateOptMacros.h"

#define LOCTEXT_NAMESPACE "SGithubCommentWidget"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGithubCommentWidget::Construct(const FArguments& InArgs)
{
	CommentData = InArgs._CommentData;
    bShowReplyInput = InArgs._ShowReplyInput;

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
                                                        // Avatar + Name + Tag
                                                        SNew(SHorizontalBox)
                                                        +SHorizontalBox::Slot()
                                                        .Padding(0, 0, 4, 0)
                                                        .AutoWidth()
                                                        [
                                                            SNew(SImage)
                                                                .Image(this, &SGithubCommentWidget::GetAvatar)
                                                        ]
                                                        + SHorizontalBox::Slot()
                                                        .FillWidth(1.0f)
                                                        .VAlign(EVerticalAlignment::VAlign_Center)
                                                        [
                                                            SNew(STextBlock)
                                                                .Text(FText::FromString(CommentData.user.login))
                                                        ]
                                                    ]
                                            ]
                                            // Comment text area
                                            + SVerticalBox::Slot()
                                            .AutoHeight()
                                            .Padding(16, 12)
                                            [
                                                SNew(SScrollBox)
                                                    + SScrollBox::Slot()
                                                    [
                                                        SNew(SRichTextBlock)
                                                            .DecoratorStyleSet(&FAppStyle::Get())
                                                            .Text(FText::FromString(CommentData.body))
                                                            //.TextStyle(&FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("Text.Large"))
                                                            .TextStyle(FAppStyle::Get(), "NormalText")
                                                    ]
                                            ]
                                            // Reply comment area
                                            + SVerticalBox::Slot()
											.AutoHeight()
                                            [
                                                SNew(SBorder)
                                                    .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                                                    [
                                                        SAssignNew(RepliesWidget, SVerticalBox)
                                                    ]
                                            ]
                                            // Reply text area
                                            + SVerticalBox::Slot()
                                            .Padding(16, 12)
                                            .AutoHeight()
                                            [
                                                SNew(SBorder)
                                                .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
												.Visibility(bShowReplyInput ? EVisibility::Visible : EVisibility::Collapsed)
                                                [
                                                    SNew(SBox)
                                                    .MinDesiredHeight(30)
                                                    [
                                                        SAssignNew(ReplyInputWidget, SMultiLineEditableText)
                                                        .HintText(LOCTEXT("LeaveComment", "Write a reply"))
                                                        .AutoWrapText(true)
                                                    ]
                                                ]
                                            ]
                                    ]
                            ]
                    ]
            ]
	];

    UGithubApi::DownloadAvatar(FOnFileDownloadComplete::CreateLambda([WeakThis = AsWeak()](const FString& GamePath, int Code, const FString& InContent)
        {
            if (GamePath.IsEmpty())
                return;
            if (auto CommentWidget = StaticCastWeakPtr<SGithubCommentWidget>(WeakThis).Pin())
            {
				CommentWidget->Avatar = MakeShared<FSlateDynamicImageBrush>(FSlateDynamicImageBrush(FName(GamePath), FVector2D(32, 32)));
            }
            
        }),
        CommentData.user.avatar_url, FString::Printf(TEXT("%d.png"), CommentData.user.id));
}

void SGithubCommentWidget::SetReply(const TArray<FComment>& ReplyComments)
{
    if (!RepliesWidget.IsValid())
        return;
    RepliesWidget->ClearChildren();
    for (auto& Reply : ReplyComments)
    {
        RepliesWidget->AddSlot()
        .AutoHeight()
        [
            SNew(SGithubCommentWidget)
            .CommentData(Reply)
            .ShowReplyInput(false)
        ];
    }
}

const FSlateBrush* SGithubCommentWidget::GetAvatar() const
{
    return Avatar.Get();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE