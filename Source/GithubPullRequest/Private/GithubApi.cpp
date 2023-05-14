// Copyright Ather Labs, Inc. All Rights Reserved.


#include "GithubApi.h"
#include "GithubPRDeveloperSettings.h"
#include "ISourceControlOperation.h"
#include "SourceControlOperations.h"
#include "ISourceControlProvider.h"
#include "ISourceControlModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Interfaces/IPluginManager.h"

FString UGithubApi::GetBaseUrl()
{
	auto Settings = GetDefault<UGithubPullRequestSettings>();
	ISourceControlModule& SourceControl = ISourceControlModule::Get();
	ISourceControlProvider& SourceControlProvider = SourceControl.GetProvider();
	TArray<FString> OutArray;
	SourceControlProvider.GetStatusText().ToString().ParseIntoArray(OutArray, TEXT("\n"));
	FString RemoteUrl;
	for (auto& Text : OutArray)
	{
		if (!Text.Contains("Remote: "))
			continue;
		RemoteUrl = Text.RightChop(FString("Remote: ").Len());
		break;
	}
	auto Path = FGenericPlatformHttp::GetUrlPath(RemoteUrl);
	Path = Path.LeftChop(4);
	return FString::Format(
		TEXT("{0}repos{1}"), 
		{ Settings->GithubUrl, FStringFormatArg(Path)});
}

void UGithubApi::BuildPullRequest()
{
	GetPullRequests(FOnPullRequestListAvailable::CreateLambda([](const TArray<FPullRequestInformation>& ListPullRequest, int Code, const FString& Content)
	{
			int a = 0;
		}));
	int a = 0;
}

bool UGithubApi::GetPullRequests(FOnPullRequestListAvailable OnPullRequestListAvailable)
{
	auto Settings = GetDefault<UGithubPullRequestSettings>();
	FHttpModule* Http = &FHttpModule::Get();
	FHttpRequestRef Request = Http->CreateRequest();
	auto BaseUrl = GetBaseUrl();
	Request->SetURL(BaseUrl + "/pulls");
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw+json"));
	Request->OnProcessRequestComplete().BindLambda([OnPullRequestListAvailable](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
	{
		if (!Response.IsValid())
			return;
		int Code = Response->GetResponseCode();
		if (Code != 200)
		{
			OnPullRequestListAvailable.ExecuteIfBound({}, Code, Response->GetContentAsString());
			return;
		}
		auto Content = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> const JsonReader = TJsonReaderFactory<>::Create(Content);
		TArray<TSharedPtr<FJsonValue>> JsonObject;
		const TSharedPtr<FJsonObject>* ResultObject = nullptr;

		if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || JsonObject.IsEmpty())
		{
			OnPullRequestListAvailable.ExecuteIfBound({}, 600, TEXT("Parse string to json failed"));
			return;
		}
		TArray<FPullRequestInformation> Pulls;
		for (auto& Element : JsonObject)
		{
			FPullRequestInformation Info;
			if (FJsonObjectConverter::JsonObjectToUStruct<FPullRequestInformation>(Element->AsObject().ToSharedRef(), &Info))
				Pulls.Add(Info);
		}
		OnPullRequestListAvailable.ExecuteIfBound(Pulls, 200, TEXT(""));
	});
	Request->ProcessRequest();

	return true;
}

bool UGithubApi::GetFilesInPullRequest(int PullNumber, FOnFilesListAvailable OnFilesListAvailable)
{
	auto Settings = GetDefault<UGithubPullRequestSettings>();
	FHttpModule* Http = &FHttpModule::Get();
	FHttpRequestRef Request = Http->CreateRequest();
	auto BaseUrl = GetBaseUrl();
	auto Url = FString::Format(TEXT("{0}/pulls/{1}/files"), {BaseUrl, PullNumber});
	Request->SetURL(Url);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw+json"));
	Request->OnProcessRequestComplete().BindLambda([OnFilesListAvailable](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
	{
		if (!InResponse.IsValid())
		{
			OnFilesListAvailable.ExecuteIfBound({}, 404, TEXT("Response is not valid"));
			return;
		}

		int Code = InResponse->GetResponseCode();
		if (Code != 200)
		{
			OnFilesListAvailable.ExecuteIfBound({}, Code, InResponse->GetContentAsString());
			return;
		}
		auto Content = InResponse->GetContentAsString();
		TSharedRef<TJsonReader<>> const JsonReader = TJsonReaderFactory<>::Create(Content);
		TArray<TSharedPtr<FJsonValue>> JsonObject;
		const TSharedPtr<FJsonObject>* ResultObject = nullptr;

		if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || JsonObject.IsEmpty())
		{
			OnFilesListAvailable.ExecuteIfBound({}, Code, "Parse json string failed");
			return;
		}
		TArray<FFileChangeInformation> Files;
		for (auto& Element : JsonObject)
		{
			FFileChangeInformation Info;
			if (FJsonObjectConverter::JsonObjectToUStruct<FFileChangeInformation>(Element->AsObject().ToSharedRef(), &Info))
				Files.Add(Info);
		}
		OnFilesListAvailable.ExecuteIfBound(Files, Code, TEXT(""));
	});
	Request->ProcessRequest();

	return true;
}

bool UGithubApi::DownloadFile(FOnFileDownloadComplete OnFileDownloadComplete, const FString& FilePath, const FString& Sha)
{
	IFileManager::Get().MakeDirectory(*FPaths::DiffDir(), true);
	const FString TempFileName = FString::Printf(TEXT("%s%s-%s"), *FPaths::DiffDir(), *Sha, *FPaths::GetCleanFilename(FilePath));
	if (FPaths::FileExists(TempFileName))
	{
		OnFileDownloadComplete.ExecuteIfBound(TempFileName, 200, TEXT(""));
		return true;
	}

	auto Settings = GetDefault<UGithubPullRequestSettings>();
	FHttpModule* Http = &FHttpModule::Get();
	auto BaseUrl = GetBaseUrl();
	FString Url;
	if (Sha.IsEmpty())
		Url = FString::Format(TEXT("{0}/contents/{1}"), {BaseUrl, FilePath});
	else
		Url = FString::Format(TEXT("{0}/contents/{1}?ref={2}"), {BaseUrl, FilePath, Sha});
	FHttpRequestRef Request = Http->CreateRequest();
	Request->SetURL(Url);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw"));
	Request->SetVerb(TEXT("GET"));
	Request->OnProcessRequestComplete().BindLambda([OnFileDownloadComplete, Settings, Sha, FilePath](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
		{
			if (!InResponse.IsValid())
			{
				OnFileDownloadComplete.ExecuteIfBound("", 404, TEXT("Response is not valid"));
				return;
			}

			int Code = InResponse->GetResponseCode();
			if (Code != 200)
			{
				OnFileDownloadComplete.ExecuteIfBound("", Code, InResponse->GetContentAsString());
				return;
			}
			FContentInfo ContentInfo;
			if (!FJsonObjectConverter::JsonObjectStringToUStruct<FContentInfo>(InResponse->GetContentAsString(), &ContentInfo))
			{
				OnFileDownloadComplete.ExecuteIfBound("", Code, "Parse json string failed");
				return;
			}
			FHttpModule* Http = &FHttpModule::Get();
			FHttpRequestRef NewRequest = Http->CreateRequest();
			NewRequest->SetURL(ContentInfo.download_url);
			NewRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
			NewRequest->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
			NewRequest->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw"));
			NewRequest->SetVerb(TEXT("GET"));
			NewRequest->OnProcessRequestComplete().BindLambda([OnFileDownloadComplete, Sha, FilePath](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
				{
					if (!InResponse.IsValid())
					{
						OnFileDownloadComplete.ExecuteIfBound("", 404, TEXT("Response is not valid"));
						return;
					}

					int Code = InResponse->GetResponseCode();
					if (Code != 200)
					{
						OnFileDownloadComplete.ExecuteIfBound("", Code, InResponse->GetContentAsString());
						return;
					}

					IFileManager::Get().MakeDirectory(*FPaths::DiffDir(), true);
					const FString TempFileName = FString::Printf(TEXT("%s%s-%s"), *FPaths::DiffDir(), *Sha, *FPaths::GetCleanFilename(FilePath));
					if (FFileHelper::SaveArrayToFile(InResponse->GetContent(), *TempFileName))
					{
						OnFileDownloadComplete.ExecuteIfBound(TempFileName, Code, InResponse->GetContentAsString());
					}
					else
					{
						OnFileDownloadComplete.ExecuteIfBound("", 1, "Save file failed");
					}
				});
			NewRequest->ProcessRequest();
		});
	Request->ProcessRequest();
	return true;
}
