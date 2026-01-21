// Fill out your copyright notice in the Description page of Project Settings.


#include "SGitFileDetail.h"
#include "SAddFileComment.h"
#include "SGithubCommentWidget.h"

#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGitFileDetail::Construct(const FArguments& InArgs)
{
    FileItem = InArgs._FileItem;
    PRInfo = InArgs._PullRequestInfo;
    CommitSha = InArgs._CommitSha;
    static const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 12);

    const FSlateBrush* IconSamples = FAppStyle::Get().GetBrush(TEXT("Graph.StateNode.Icon"));

    if (FileItem->status == "added")
    {
        IconSamples = FAppStyle::Get().GetBrush(TEXT("SourceControl.Add"));
    }
    else if (FileItem->status == "modified")
    {
        IconSamples = FAppStyle::Get().GetBrush(TEXT("SourceControl.Edit"));
    }
    else if (FileItem->status == "renamed")
    {
        IconSamples = FAppStyle::Get().GetBrush(TEXT("SourceControl.Integrate"));
    }


    TSharedPtr<SBox> FileHeader;
    SAssignNew(FileHeader, SBox)
    .Padding(10)
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
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                    .Text(FText::FromString(FileItem->filename))
                    .Font(Font)
            ]
            +SHorizontalBox::Slot()
            .VAlign(VAlign_Center)
            .AutoWidth()
            .Padding(5, 0, 0, 0)
            [
                SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("+%d -%d"), FileItem->additions, FileItem->deletions)))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    .ColorAndOpacity(FSlateColor(FLinearColor::Gray))
            ]
            +SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0, 0, 0)
            [
                SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                    [
                        SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            [
                                SNew(STextBlock)
                                    .Text(FText::FromString("Viewed"))
                                    .Font(Font)
                            ]
                            + SHorizontalBox::Slot()
                            [
                                SNew(SCheckBox)
                            ]
                    ]

            ]
            +SHorizontalBox::Slot()
            .MinWidth(100)
            .AutoWidth()
            .Padding(5, 0, 0, 0)
            [
                SNew(SComboButton)
                    .MenuPlacement(MenuPlacement_BelowAnchor)
                    .HasDownArrow(false)
                    .ButtonContent()
                    [
                        SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            [
                                SNew(STextBlock)
                                    .Text(FText::FromString("Comment:"))
                            ]
                            + SHorizontalBox::Slot()
                            .Padding(5, 0, 0, 0)
                            .AutoWidth()
                            [
                                SNew(STextBlock)
                                    .Text_Raw(this, &SGitFileDetail::GetFileNumCommentText)
                            ]
                    ]
                    .MenuContent()
                    [
                        SNew(SAddFileComment)
                            .CommitSha(CommitSha)
                            .CommitFile(FileItem)
                            .PullRequestInfo(PRInfo)
                    ]
            ]
    ];


    SAssignNew(FileDetail, SVerticalBox);


    TSharedPtr<SExpandableArea> ExpandableCommit;
    SAssignNew(ExpandableCommit, SExpandableArea)
        .InitiallyCollapsed(true)
        .BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
        .BodyBorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
        .BodyBorderBackgroundColor(FLinearColor::White)
        .Padding(0.0f)
        .HeaderContent()
        [
            FileHeader.ToSharedRef()
        ]
        .BodyContent()
        [
            FileDetail.ToSharedRef()
        ];

    ChildSlot
        [
            ExpandableCommit.ToSharedRef()
        ];



    UpdateCommentForFile();

    if (FileItem->AllComments.IsEmpty())
    {
        PRInfo->OnCommentAvailable.AddLambda([WeakThis = AsWeak()]()
            {
                if (auto FileDetail = StaticCastWeakPtr<SGitFileDetail>(WeakThis).Pin())
                {
                    FileDetail->UpdateCommentForFile();
                }
            });
    }
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


FText SGitFileDetail::GetFileNumCommentText() const
{
    if (!FileItem)
        return {};

    if (FileItem->AllComments.IsEmpty())
        return {};
    return FText::AsNumber(FileItem->AllComments.Num());
}


void SGitFileDetail::UpdateCommentForFile()
{
    if (!PRInfo || PRInfo->AllComments.IsEmpty())
        return;
    TArray<FComment> Results;

    for (auto& Comment : PRInfo->AllComments)
    {
        if (Comment.path != FileItem->filename)
            continue;
        Results.Add(Comment);
    }
    FileItem->AllComments = Results;
    if (!FileDetail)
        return;
    TMap<int64, TArray<FComment>> ReplyComments;
	TMap<int64, TSharedPtr<SGithubCommentWidget>> CommentWidgets;

    for (auto& Comment : FileItem->AllComments)
    {
		if (Comment.in_reply_to_id != 0)
		{
			ReplyComments.FindOrAdd(Comment.in_reply_to_id).Add(Comment);
            continue;
		}
		TSharedPtr<SGithubCommentWidget> CommentWidget;
        FileDetail->AddSlot()
		.AutoHeight()
        [
			SAssignNew(CommentWidget, SGithubCommentWidget)
				.CommentData(Comment)
        ];
		CommentWidgets.Add(Comment.id, CommentWidget);
    }
    for (auto& [Id, Widget] : CommentWidgets)
    {
		if (ReplyComments.Contains(Id))
		{
			Widget->SetReply(ReplyComments[Id]);
		}
    }
}