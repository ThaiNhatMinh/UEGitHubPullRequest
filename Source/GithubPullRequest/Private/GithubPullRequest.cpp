// Copyright Epic Games, Inc. All Rights Reserved.

#include "GithubPullRequest.h"
#include "PullRequestEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

static const FName PanelIconName(TEXT("LevelEditor.GameSettings.Small"));

#define LOCTEXT_NAMESPACE "FGithubPullRequestModule"
static const FName MergeAssistTabId = FName(TEXT("MergeAssist1234"));

void FGithubPullRequestModule::StartupModule()
{
	const auto TabSpawner = FOnSpawnTab::CreateStatic([](const FSpawnTabArgs& Args)
		{
			// Create a dock tab and fill it with the merge assist UI
			return SNew(SDockTab)
				[
					SNew(SPullRequestsEditor, Args.GetTabId(), Args.GetOwnerWindow())
				];
		});
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FTabSpawnerEntry& TabSpawnerEntry = FGlobalTabmanager::Get()->RegisterNomadTabSpawner(MergeAssistTabId, TabSpawner);

	// Configure our tab spawner by adding a name and tooltip
	TabSpawnerEntry.SetDisplayName(LOCTEXT("TabTitle", "Github PullRequest"));
	TabSpawnerEntry.SetTooltipText(LOCTEXT("TooltipText", "Open Github PullRequest tool"));

	TabSpawnerEntry.SetGroup( WorkspaceMenu::GetMenuStructure().GetToolsCategory() );
}

void FGithubPullRequestModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MergeAssistTabId);
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGithubPullRequestModule, GithubPullRequest)