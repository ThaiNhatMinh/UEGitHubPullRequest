
#include "GithubPRDeveloperSettings.h"

UGithubPullRequestSettings::UGithubPullRequestSettings()
{
	const FString& IniFile = USourceControlHelpers::GetSettingsIni();
	const FString& GlobalIniFile = USourceControlHelpers::GetGlobalSettingsIni();
	if (GithubToken.IsEmpty())
	{
		GConfig->GetString(TEXT("SourceControl.SourceControlSettings"), TEXT("Token"), GithubToken, IniFile);
	}
	else
	{
		GConfig->SetString(TEXT("SourceControl.SourceControlSettings"), TEXT("Token"), *GithubToken, IniFile);
	}
}

void UGithubPullRequestSettings::PostEditChangeChainProperty( struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	const FString& IniFile = USourceControlHelpers::GetSettingsIni();
	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UGithubPullRequestSettings, GithubToken))
	{
		GConfig->SetString(TEXT("SourceControl.SourceControlSettings"), TEXT("Token"), *GithubToken, IniFile);
	}
}