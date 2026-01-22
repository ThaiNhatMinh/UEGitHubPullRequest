// Fill out your copyright notice in the Description page of Project Settings.


#include "SGithubCommentWidget.h"
#include "Widgets/Text/SRichTextBlock.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "SlateOptMacros.h"

#define LOCTEXT_NAMESPACE "SGithubCommentWidget"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGithubCommentWidget::Construct(const FArguments& InArgs)
{
	CommentData = InArgs._CommentData;
    bShowReplyInput = InArgs._ShowReplyInput;
    PRInfo = InArgs._PullRequestInfo;

    // Border color
    FLinearColor BorderColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);
    FLinearColor ShadowColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);
    FLinearColor BackgroundColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);

    TSharedPtr<SBorder> HeaderWidget;
    TSharedPtr<SVerticalBox> BodyWidget;

    SAssignNew(HeaderWidget, SBorder)
        .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(FMargin(0, 12, 12, 12))
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
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(EVerticalAlignment::VAlign_Center)
                [
                    SNew(SBorder)
                        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .BorderBackgroundColor(BorderColor)
                        [
                            SNew(STextBlock)
                                .Text(FText::FromString(::ToString(CommentData.author_association)))
                        ]
                ]
        ];
    SAssignNew(BodyWidget, SVerticalBox)
        // Comment text area
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 12, 12, 12)
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
        .Padding(20, 0, 0, 0)
        .AutoHeight()
        [
            SNew(SBorder)
                .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                .Visibility(bShowReplyInput ? EVisibility::Visible : EVisibility::Collapsed)
                [
                    SAssignNew(RepliesWidget, SVerticalBox)
                ]
        ]
        // Reply text area
        + SVerticalBox::Slot()
        .Padding(16, 12)
        .AutoHeight()
        [
            SNew(SHorizontalBox)
                .Visibility(bShowReplyInput ? EVisibility::Visible : EVisibility::Collapsed)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(0, 0, 10, 0)
                [

                    SNew(SBorder)
                        .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
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
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(10, 0, 0, 0)
                [
                    SNew(SButton)
                        //.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
                        .Text(FText::FromString("Reply"))
                        .VAlign(EVerticalAlignment::VAlign_Center)
                        .OnClicked(this, &SGithubCommentWidget::OnReply)
                ]
        ];
    
    TSharedPtr<SExpandableArea> ExpandableCommit;

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
                                        SAssignNew(ExpandableCommit, SExpandableArea)
                                        .InitiallyCollapsed(bShowReplyInput)
                                        .Padding(0.0f)
                                        .HeaderPadding(0.0f)
                                        .HeaderContent()
                                        [
                                            HeaderWidget.ToSharedRef()
                                        ]
                                        .BodyContent()
                                        [
                                            BodyWidget.ToSharedRef()
                                        ]
                                    ]
                            ]
                    ]
            ]
	];
    //
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

FReply SGithubCommentWidget::OnReply()
{
    if (!bShowReplyInput)
        return FReply::Handled();

	auto ReplyText = ReplyInputWidget->GetText().ToString();
    if (ReplyText.IsEmpty())
    {
		FNotificationInfo Info(FText::FromString(TEXT("Reply can not empty")));
		Info.bFireAndForget = true;
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.Warning"));
		FSlateNotificationManager::Get().AddNotification(Info);
		return FReply::Handled();
    }

	UGithubApi::CreateComment(FOnCreateReviewComplete::CreateLambda([SharedThis = AsWeak()](int Code, const FString& Content)
		{
			FNotificationInfo Info(FText::FromString(TEXT("Reply comment success")));
            Info.ExpireDuration = 5.0f;
            Info.ExpireDuration = 5.0f;
            Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.Success"));
            FSlateNotificationManager::Get().AddNotification(Info);
			if (auto CommentWidget = StaticCastWeakPtr<SGithubCommentWidget>(SharedThis).Pin())
			{
				CommentWidget->ReplyInputWidget->SetText(FText::GetEmpty());
                CommentWidget->PRInfo->OnCommentShouldUpdate.ExecuteIfBound();
			}
        }), 
        PRInfo->number, ReplyText, CommentData.commit_id, CommentData.path, CommentData.id);

	return FReply::Handled();
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
			.PullRequestInfo(PRInfo)
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