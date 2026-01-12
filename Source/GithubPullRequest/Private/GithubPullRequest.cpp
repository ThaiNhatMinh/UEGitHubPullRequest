// Copyright Epic Games, Inc. All Rights Reserved.

#include "GithubPullRequest.h"
#include "PullRequestEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"

static const FName PanelIconName(TEXT("LevelEditor.GameSettings.Small"));

#define LOCTEXT_NAMESPACE "FGithubPullRequestModule"
static const FName GithubPullRequestTagId = FName(TEXT("GithubPullRequestTagId"));

void FGithubPullRequestModule::StartupModule()
{
	const auto TabSpawner = FOnSpawnTab::CreateStatic([](const FSpawnTabArgs& Args)
		{
			// Check if source control is enabled
			ISourceControlModule& SourceControlModule = ISourceControlModule::Get();
			ISourceControlProvider& SourceControlProvider = SourceControlModule.GetProvider();

			if (!SourceControlModule.IsEnabled() || !SourceControlProvider.IsAvailable())
			{
				// Create a container that can be updated after login
				TSharedPtr<SBox> ContentContainer;

				TSharedRef<SDockTab> DockTab = SNew(SDockTab)
					[
						SAssignNew(ContentContainer, SBox)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("EnableSourceControl", "Please enable Source Control to use this tool.\nConnecting..."))
							.Justification(ETextJustify::Center)
						]
					];

				// Capture what we need for the callback
				FTabId TabId = Args.GetTabId();
				TSharedPtr<SWindow> OwnerWindow = Args.GetOwnerWindow();

				// Show the source control login dialog with callback
				FSourceControlLoginClosed OnLoginClosed = FSourceControlLoginClosed::CreateLambda(
					[ContentContainer, TabId, OwnerWindow](bool bSuccess)
					{
						if (bSuccess && ContentContainer.IsValid())
						{
							// Source control connected successfully, switch to the full UI
							ContentContainer->SetHAlign(HAlign_Fill);
							ContentContainer->SetVAlign(VAlign_Fill);
							ContentContainer->SetContent(
								SNew(SPullRequestsEditor, TabId, OwnerWindow)
							);
						}
						else if (ContentContainer.IsValid())
						{
							// Login failed or cancelled, show message
							ContentContainer->SetContent(
								SNew(STextBlock)
								.Text(LOCTEXT("SourceControlFailed", "Source Control connection failed or was cancelled.\nPlease close this tab and try again."))
								.Justification(ETextJustify::Center)
							);
						}
					});

				SourceControlModule.ShowLoginDialog(OnLoginClosed, ELoginWindowMode::Modeless);

				return DockTab;
			}

			// Create a dock tab and fill it with the merge assist UI
			return SNew(SDockTab)
				[
					SNew(SPullRequestsEditor, Args.GetTabId(), Args.GetOwnerWindow())
				];
		});
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FTabSpawnerEntry& TabSpawnerEntry = FGlobalTabmanager::Get()->RegisterNomadTabSpawner(GithubPullRequestTagId, TabSpawner);

	// Configure our tab spawner by adding a name and tooltip
	TabSpawnerEntry.SetDisplayName(LOCTEXT("TabTitle", "Github PullRequest"));
	TabSpawnerEntry.SetTooltipText(LOCTEXT("TooltipText", "Open Github PullRequest tool"));

	TabSpawnerEntry.SetGroup( WorkspaceMenu::GetMenuStructure().GetToolsCategory() );
}

void FGithubPullRequestModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GithubPullRequestTagId);
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGithubPullRequestModule, GithubPullRequest)