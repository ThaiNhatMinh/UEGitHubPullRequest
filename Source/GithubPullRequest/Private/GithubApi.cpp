
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

	FString RemoteStr = "Remote: ";
	FString RemoteOriginStr = "Remote Origin: ";

	for (auto& Text : OutArray)
	{
		if (!Text.Contains(RemoteStr) && !Text.Contains(RemoteOriginStr))
			continue;

		if (Text.Contains(RemoteStr))
		{
			RemoteUrl = Text.RightChop(RemoteStr.Len());
		}
		else if (Text.Contains(RemoteOriginStr))
		{
			RemoteUrl = Text.RightChop(RemoteOriginStr.Len());
		}
		break;
	}
	auto Path = FGenericPlatformHttp::GetUrlPath(RemoteUrl);
	Path = Path.LeftChop(4);
	return FString::Format(
		TEXT("{0}repos{1}"),
		{ Settings->GithubUrl, FStringFormatArg(Path)});
}

bool UGithubApi::GetPullRequests(FOnPullRequestListAvailable OnPullRequestListAvailable, EPullRequestQueryState state, int Page, int NumberPage)
{
	auto State = UEnum::GetDisplayValueAsText(state).ToString();
	auto Settings = GetDefault<UGithubPullRequestSettings>();
	FHttpModule* Http = &FHttpModule::Get();
	FHttpRequestRef Request = Http->CreateRequest();
	auto BaseUrl = GetBaseUrl();
	auto Url = FString::Format(TEXT("{0}/pulls?per_page={1}&page={2}&state={3}"), {BaseUrl, NumberPage, Page, State});
	Request->SetURL(Url);
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

bool UGithubApi::GetFilesInPullRequest(int PullNumber, FOnFilesListAvailable OnFilesListAvailable, int NumFile, int Page)
{
	auto Settings = GetDefault<UGithubPullRequestSettings>();
	FHttpModule* Http = &FHttpModule::Get();
	FHttpRequestRef Request = Http->CreateRequest();
	auto BaseUrl = GetBaseUrl();
	auto Url = FString::Format(TEXT("{0}/pulls/{1}/files?per_page={2}&page={3}"), {BaseUrl, PullNumber, NumFile, Page});
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

bool UGithubApi::DownloadAvatar(FOnFileDownloadComplete OnFileDownloadComplete, const FString& Url, const FString& FileName)
{
	auto TargetDir = FPaths::ProjectSavedDir() + "Github/Avatar/";
	const FString TempFileName = TargetDir + FileName;
	IFileManager::Get().MakeDirectory(*TargetDir, true);
	if (FPaths::FileExists(TempFileName))
	{
		OnFileDownloadComplete.ExecuteIfBound(TempFileName, 200, TEXT(""));
		return true;
	}

	FHttpModule* Http = &FHttpModule::Get();
	FHttpRequestRef Request = Http->CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw"));
	Request->OnProcessRequestComplete().BindLambda([OnFileDownloadComplete, FileName, TempFileName](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
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
			if (FFileHelper::SaveArrayToFile(InResponse->GetContent(), *TempFileName))
			{
				OnFileDownloadComplete.ExecuteIfBound(TempFileName, Code, InResponse->GetContentAsString());
			}
			else
			{
				OnFileDownloadComplete.ExecuteIfBound("", 1, "Save file failed");
			}
		});
	Request->ProcessRequest();
	return true;
}

bool UGithubApi::CreateReview(FOnCreateReviewComplete OnCreateReviewComplete, int PullNumber, const FString& Event, const FString& Body, const FString& CommitId, const TArray<FReviewComment>& Comments)
{
	auto Settings = GetDefault<UGithubPullRequestSettings>();
	FHttpModule* Http = &FHttpModule::Get();
	FHttpRequestRef Request = Http->CreateRequest();
	auto BaseUrl = GetBaseUrl();
	auto Url = FString::Format(TEXT("{0}/pulls/{1}/reviews"), {BaseUrl, PullNumber});
	Request->SetURL(Url);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw+json"));
	Request->SetVerb("POST");

	FReviewBody BodyStruct;
	BodyStruct.commit_id = CommitId;
	BodyStruct.body = Body;
	BodyStruct.event = Event;
	BodyStruct.comments = Comments;
	TSharedRef<FJsonObject> OutJsonObject = MakeShareable(new FJsonObject);
	FJsonObjectConverter::UStructToJsonObject(FReviewBody::StaticStruct(), &BodyStruct, OutJsonObject, 0, 0);
	if (CommitId.IsEmpty())
		OutJsonObject->RemoveField("commit_id");

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(OutJsonObject, Writer);

	Request->SetContentAsString(OutputString);

	Request->OnProcessRequestComplete().BindLambda([OnCreateReviewComplete](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
		{
			if (!InResponse.IsValid())
			{
				OnCreateReviewComplete.ExecuteIfBound(404, TEXT("Response is not valid"));
				return;
			}
			OnCreateReviewComplete.ExecuteIfBound(InResponse->GetResponseCode(), InResponse->GetContentAsString());
		});
	Request->ProcessRequest();
	return true;
}

bool UGithubApi::ListCommit(FOnCommitListAvailable OnListCommitComplete, const FString& sha, const FString& path, const FString& author, const FString& committer, const FString& since, const FString& until, int page, int per_page)
{
	if (sha.IsEmpty())
		return false;
	auto Settings = GetDefault<UGithubPullRequestSettings>();
	FHttpModule* Http = &FHttpModule::Get();
	FHttpRequestRef Request = Http->CreateRequest();
	auto BaseUrl = GetBaseUrl();
	auto Url = FString::Format(TEXT("{0}/commits"), {BaseUrl});
	Url += "?sha=" + sha;
	if (!path.IsEmpty())
		Url += "&path=" + path;
	if (!author.IsEmpty())
		Url += "&author=" + author;
	if (!committer.IsEmpty())
		Url += "&committer=" + committer;
	if (!since.IsEmpty())
		Url += "&committer=" + since;
	if (!until.IsEmpty())
		Url += "&until=" + until;
	Url += "&page=" + FString::FromInt(page);
	Url += "&per_page=" + FString::FromInt(per_page);

	Request->SetURL(Url);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw+json"));
	Request->SetVerb("GET");
	Request->OnProcessRequestComplete().BindLambda([OnListCommitComplete](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
		{
			if (!InResponse.IsValid())
			{
				OnListCommitComplete.ExecuteIfBound({}, 404, TEXT("Response is not valid"));
				return;
			}
			auto Content = InResponse->GetContentAsString();
			TSharedRef<TJsonReader<>> const JsonReader = TJsonReaderFactory<>::Create(Content);
			TArray<TSharedPtr<FJsonValue>> JsonObject;
			const TSharedPtr<FJsonObject>* ResultObject = nullptr;

			if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || JsonObject.IsEmpty())
			{
				OnListCommitComplete.ExecuteIfBound({}, 600, TEXT("Parse string to json failed"));
				return;
			}
			TArray<FCommitFullData> Commits;
			for (auto& Element : JsonObject)
			{
				FCommitFullData Info;
				if (FJsonObjectConverter::JsonObjectToUStruct<FCommitFullData>(Element->AsObject().ToSharedRef(), &Info))
					Commits.Add(Info);
			}
			OnListCommitComplete.ExecuteIfBound(Commits, 200, TEXT(""));
		});
	Request->ProcessRequest();
	return true;
}

bool UGithubApi::ListCommitFromPull(FOnCommitListAvailable OnListCommitComplete, int PullNumber, int page, int per_page)
{
	auto Settings = GetDefault<UGithubPullRequestSettings>();
	FHttpModule* Http = &FHttpModule::Get();
	FHttpRequestRef Request = Http->CreateRequest();
	auto BaseUrl = GetBaseUrl();
	auto Url = FString::Format(TEXT("{0}/pulls/{1}/commits"), {BaseUrl, PullNumber});
	Url += "?page=" + FString::FromInt(page);
	Url += "&per_page=" + FString::FromInt(per_page);

	Request->SetURL(Url);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw+json"));
	Request->SetVerb("GET");
	Request->OnProcessRequestComplete().BindLambda([OnListCommitComplete](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
		{
			if (!InResponse.IsValid())
			{
				OnListCommitComplete.ExecuteIfBound({}, 404, TEXT("Response is not valid"));
				return;
			}
			auto Content = InResponse->GetContentAsString();
			TSharedRef<TJsonReader<>> const JsonReader = TJsonReaderFactory<>::Create(Content);
			TArray<TSharedPtr<FJsonValue>> JsonObject;
			const TSharedPtr<FJsonObject>* ResultObject = nullptr;

			if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || JsonObject.IsEmpty())
			{
				OnListCommitComplete.ExecuteIfBound({}, 600, TEXT("Parse string to json failed"));
				return;
			}
			TArray<FCommitFullData> Commits;
			for (auto& Element : JsonObject)
			{
				FCommitFullData Info;
				if (FJsonObjectConverter::JsonObjectToUStruct<FCommitFullData>(Element->AsObject().ToSharedRef(), &Info))
					Commits.Add(Info);
			}
			OnListCommitComplete.ExecuteIfBound(Commits, 200, TEXT(""));
		});
	Request->ProcessRequest();
	return true;
}