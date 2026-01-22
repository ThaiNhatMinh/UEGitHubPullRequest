
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
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

namespace
{
	bool CheckGithubToken()
	{
		const UGithubPullRequestSettings* Settings = GetDefault<UGithubPullRequestSettings>();
		if (Settings->GithubToken.IsEmpty())
		{
			FNotificationInfo Info(FText::FromString(TEXT("GitHub Token is missing. Please configure it in Project Settings > Plugins > Github Pull Request.")));
			Info.bFireAndForget = true;
			Info.ExpireDuration = 5.0f;
			Info.bUseThrobber = false;
			Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.Warning"));
			FSlateNotificationManager::Get().AddNotification(Info);
		}
		return true;
	}

	void OnError(FString Message)
	{
		FNotificationInfo Info(FText::FromString(Message));
		Info.bFireAndForget = true;
		Info.ExpireDuration = 5.0f;
		Info.bUseThrobber = false;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.Error"));
		FSlateNotificationManager::Get().AddNotification(Info);
	}

	void OnError(FHttpResponsePtr Response)
	{
		if (Response.IsValid())
		{

			FQueryResultError Error;
			if (!FJsonObjectConverter::JsonObjectStringToUStruct<FQueryResultError>(Response->GetContentAsString(), &Error))
			{
				return;
			}
			FNotificationInfo Info(FText::FromString(Error.message));
			FString Message;
			for (auto& error : Error.errors)
			{
				Message += "\n" + error;
			}
			Message += "\n" + Error.documentation_url;
			Info.SubText = FText::FromString(Message);
			Info.bFireAndForget = true;
			Info.ExpireDuration = 5.0f;
			Info.bUseThrobber = false;
			Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.Error"));
			FSlateNotificationManager::Get().AddNotification(Info);
		}
		else
		{
			FNotificationInfo Info(FText::FromString("Response is not valid"));
			Info.bFireAndForget = true;
			Info.ExpireDuration = 5.0f;
			Info.bUseThrobber = false;
			Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.Error"));
			FSlateNotificationManager::Get().AddNotification(Info);
		}
	}
}

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
	if (!CheckGithubToken())
	{
		OnPullRequestListAvailable.ExecuteIfBound({}, 401, TEXT("GitHub token is not configured"));
		return false;
	}

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
	Request->SetHeader(TEXT("X-GitHub-Api-Version"), TEXT("2022-11-28"));
	Request->OnProcessRequestComplete().BindLambda([OnPullRequestListAvailable](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
	{
		if (!Response.IsValid())
			return;
		int Code = Response->GetResponseCode();
		if (Code != 200)
		{
			OnError(Response);
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

bool UGithubApi::GetPullRequest(FOnPullRequestAvailable OnPullRequestAvailable, int PullNumber)
{
	if (!CheckGithubToken())
	{
		OnPullRequestAvailable.ExecuteIfBound({}, 401, TEXT("GitHub token is not configured"));
		return false;
	}
	auto Settings = GetDefault<UGithubPullRequestSettings>();
	FHttpModule* Http = &FHttpModule::Get();
	FHttpRequestRef Request = Http->CreateRequest();
	auto BaseUrl = GetBaseUrl();
	auto Url = FString::Format(TEXT("{0}/pull/{1}"), {BaseUrl, PullNumber});
	Request->SetURL(Url);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw+json"));
	Request->SetHeader(TEXT("X-GitHub-Api-Version"), TEXT("2022-11-28"));
	Request->OnProcessRequestComplete().BindLambda([OnPullRequestAvailable](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			if (!Response.IsValid())
				return;
			int Code = Response->GetResponseCode();
			if (Code != 200)
			{
				OnError(Response);
				return;
			}
			auto Content = Response->GetContentAsString();
			TSharedRef<TJsonReader<>> const JsonReader = TJsonReaderFactory<>::Create(Content);
			TArray<TSharedPtr<FJsonValue>> JsonObject;
			const TSharedPtr<FJsonObject>* ResultObject = nullptr;

			if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || JsonObject.IsEmpty())
			{
				OnPullRequestAvailable.ExecuteIfBound({}, 600, TEXT("Parse string to json failed"));
				return;
			}
			FPullRequestInformation Result;
			if (!FJsonObjectConverter::JsonObjectStringToUStruct<FPullRequestInformation>(Response->GetContentAsString(), &Result))
			{
				return;
			}
			OnPullRequestAvailable.ExecuteIfBound(Result, Code, Response->GetContentAsString());
		});
	Request->ProcessRequest();

	return true;
}

bool UGithubApi::GetFilesInPullRequest(int PullNumber, FOnFilesListAvailable OnFilesListAvailable, int NumFile, int Page)
{
	if (!CheckGithubToken())
	{
		OnFilesListAvailable.ExecuteIfBound({}, 401, TEXT("GitHub token is not configured"));
		return false;
	}

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
			OnError(InResponse);
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
		TArray<FCommitFile> Files;
		for (auto& Element : JsonObject)
		{
			FCommitFile Info;
			if (FJsonObjectConverter::JsonObjectToUStruct<FCommitFile>(Element->AsObject().ToSharedRef(), &Info))
				Files.Add(Info);
		}
		OnFilesListAvailable.ExecuteIfBound(Files, Code, TEXT(""));
	});
	Request->ProcessRequest();

	return true;
}

bool UGithubApi::DownloadFile(FOnFileDownloadComplete OnFileDownloadComplete, const FString& FilePath, const FString& Sha)
{
	if (!CheckGithubToken())
	{
		OnFileDownloadComplete.ExecuteIfBound(TEXT(""), 401, TEXT("GitHub token is not configured"));
		return false;
	}

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
	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github+json"));
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
				OnError(InResponse);
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
			FString content;
			FBase64::Decode(ContentInfo.content, content);
			NewRequest->SetURL(ContentInfo.download_url);
			//NewRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
			//NewRequest->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw+json"));
			NewRequest->SetHeader(TEXT("X-GitHub-Api-Version"), TEXT("2022-11-28"));
			//NewRequest->SetHeader(TEXT("Accept-Encoding"), TEXT("gzip, deflate"));
			auto Token = NewRequest->GetURLParameter("token");
			if (!Token.IsEmpty())
			{
				NewRequest->SetHeader(TEXT("token"), *Token);
			}
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
						OnError(InResponse);
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
//
//bool UGithubApi::DownloadFile(FOnFileDownloadComplete OnFileDownloadComplete, const FCommitFile& CommitFile)
//{
//	if (!CheckGithubToken() || CommitFile.raw_url.IsEmpty())
//	{
//		return false;
//	}
//
//
//	IFileManager::Get().MakeDirectory(*FPaths::DiffDir(), true);
//	const FString TempFileName = FString::Printf(TEXT("%s%s-%s"), *FPaths::DiffDir(), *CommitFile.Sha, *FPaths::GetCleanFilename(FilePath));
//	if (FPaths::FileExists(TempFileName))
//	{
//		OnFileDownloadComplete.ExecuteIfBound(TempFileName, 200, TEXT(""));
//		return true;
//	}
//
//	auto Settings = GetDefault<UGithubPullRequestSettings>();
//	FHttpModule* Http = &FHttpModule::Get();
//	auto BaseUrl = GetBaseUrl();
//	FString Url;
//	if (Sha.IsEmpty())
//		Url = FString::Format(TEXT("{0}/contents/{1}"), {BaseUrl, FilePath});
//	else
//		Url = FString::Format(TEXT("{0}/contents/{1}?ref={2}"), {BaseUrl, FilePath, Sha});
//	FHttpRequestRef Request = Http->CreateRequest();
//	Request->SetURL(Url);
//	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
//	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
//	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw"));
//	Request->SetVerb(TEXT("GET"));
//	Request->OnProcessRequestComplete().BindLambda([OnFileDownloadComplete, Settings, Sha, FilePath](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
//		{
//			if (!InResponse.IsValid())
//			{
//				OnFileDownloadComplete.ExecuteIfBound("", 404, TEXT("Response is not valid"));
//				return;
//			}
//
//			int Code = InResponse->GetResponseCode();
//			if (Code != 200)
//			{
//				OnFileDownloadComplete.ExecuteIfBound("", Code, InResponse->GetContentAsString());
//				return;
//			}
//			FContentInfo ContentInfo;
//			if (!FJsonObjectConverter::JsonObjectStringToUStruct<FContentInfo>(InResponse->GetContentAsString(), &ContentInfo))
//			{
//				OnFileDownloadComplete.ExecuteIfBound("", Code, "Parse json string failed");
//				return;
//			}
//			FHttpModule* Http = &FHttpModule::Get();
//			FHttpRequestRef NewRequest = Http->CreateRequest();
//			FString content;
//			FBase64::Decode(ContentInfo.content, content);
//			NewRequest->SetURL(ContentInfo.download_url);
//			NewRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
//			NewRequest->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw"));
//			NewRequest->SetHeader(TEXT("X-GitHub-Api-Version"), TEXT("2022-11-28"));
//			NewRequest->SetHeader(TEXT("Accept-Encoding"), TEXT("gzip, deflate"));
//			auto Token = NewRequest->GetURLParameter("token");
//			if (!Token.IsEmpty())
//			{
//				NewRequest->SetHeader(TEXT("token"), *Token);
//			}
//			NewRequest->SetVerb(TEXT("GET"));
//			NewRequest->OnProcessRequestComplete().BindLambda([OnFileDownloadComplete, Sha, FilePath](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
//				{
//					if (!InResponse.IsValid())
//					{
//						OnFileDownloadComplete.ExecuteIfBound("", 404, TEXT("Response is not valid"));
//						return;
//					}
//
//					int Code = InResponse->GetResponseCode();
//					if (Code != 200)
//					{
//						OnFileDownloadComplete.ExecuteIfBound("", Code, InResponse->GetContentAsString());
//						return;
//					}
//
//					IFileManager::Get().MakeDirectory(*FPaths::DiffDir(), true);
//					const FString TempFileName = FString::Printf(TEXT("%s%s-%s"), *FPaths::DiffDir(), *Sha, *FPaths::GetCleanFilename(FilePath));
//					if (FFileHelper::SaveArrayToFile(InResponse->GetContent(), *TempFileName))
//					{
//						OnFileDownloadComplete.ExecuteIfBound(TempFileName, Code, InResponse->GetContentAsString());
//					}
//					else
//					{
//						OnFileDownloadComplete.ExecuteIfBound("", 1, "Save file failed");
//					}
//				});
//			NewRequest->ProcessRequest();
//		});
//	Request->ProcessRequest();
//	return true;
//}

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
				OnError(InResponse);
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
	if (!CheckGithubToken())
	{
		OnCreateReviewComplete.ExecuteIfBound(401, TEXT("GitHub token is not configured"));
		return false;
	}

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
		OutJsonObject->RemoveField(TEXT("commit_id"));

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(OutJsonObject, Writer);

	Request->SetContentAsString(OutputString);

	Request->OnProcessRequestComplete().BindLambda([OnCreateReviewComplete](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
		{
			if (!InResponse.IsValid())
			{
				OnError(InResponse);
				return;
			}
			auto Code = InResponse->GetResponseCode();
			if (Code != 200)
			{
				OnError(InResponse);
				return;
			}
			OnCreateReviewComplete.ExecuteIfBound(InResponse->GetResponseCode(), InResponse->GetContentAsString());
		});
	Request->ProcessRequest();
	return true;
}

bool UGithubApi::CreateComment(FOnCreateReviewComplete OnCreateReviewComplete, int PullNumber, const FString& Body, const FString& CommitId, const FString& FilePath)
{
	if (!CheckGithubToken())
	{
		return false;
	}


	auto Settings = GetDefault<UGithubPullRequestSettings>();
	FHttpModule* Http = &FHttpModule::Get();
	FHttpRequestRef Request = Http->CreateRequest();
	auto BaseUrl = GetBaseUrl();
	auto Url = FString::Format(TEXT("{0}/pulls/{1}/comments"), {BaseUrl, PullNumber});
	Request->SetURL(Url);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw+json"));
	Request->SetHeader(TEXT("X-GitHub-Api-Version"), TEXT("2022-11-28"));
	Request->SetVerb("POST");

	FCommentBody BodyStruct;
	BodyStruct.commit_id = CommitId;
	BodyStruct.body = Body;
	BodyStruct.path = FilePath;

	auto OutJsonObject = FJsonObjectConverter::UStructToJsonObject(BodyStruct, 0, 0);
	//BodyStruct.subject_type = "line";
	OutJsonObject->RemoveField(TEXT("line"));
	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(OutJsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(OutputString);

	Request->OnProcessRequestComplete().BindLambda([OnCreateReviewComplete](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
		{
			if (!InResponse.IsValid())
			{
				OnError(InResponse);
				return;
			}
			auto Code = InResponse->GetResponseCode();
			if (Code != 201)
			{
				OnError(InResponse);
				return;
			}
			OnCreateReviewComplete.ExecuteIfBound(InResponse->GetResponseCode(), InResponse->GetContentAsString());
		});
	Request->ProcessRequest();
	return true;
}

bool UGithubApi::ListComment(FOnListCommentAvailable OnListCommentAvailable, int PullNumber, int Page, int NumberPage)
{
	if (!CheckGithubToken())
	{
		return false;
	}

	auto Settings = GetDefault<UGithubPullRequestSettings>();
	FHttpModule* Http = &FHttpModule::Get();
	FHttpRequestRef Request = Http->CreateRequest();
	auto BaseUrl = GetBaseUrl();
	auto Url = FString::Format(TEXT("{0}/pulls/{1}/comments"), {BaseUrl, PullNumber});
	Url += "?page=" + FString::FromInt(Page);
	Url += "&per_page=" + FString::FromInt(NumberPage);

	Request->SetURL(Url);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw+json"));
	Request->SetVerb("GET");
	Request->OnProcessRequestComplete().BindLambda([OnListCommentAvailable](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
		{
			if (!InResponse.IsValid() || InResponse->GetResponseCode() != 200)
			{
				OnError(InResponse);
				return;
			}
			auto Content = InResponse->GetContentAsString();
			TSharedRef<TJsonReader<>> const JsonReader = TJsonReaderFactory<>::Create(Content);
			TArray<TSharedPtr<FJsonValue>> JsonObject;
			const TSharedPtr<FJsonObject>* ResultObject = nullptr;

			if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || JsonObject.IsEmpty())
			{
				return;
			}
			TArray<FComment> Comments;
			for (auto& Element : JsonObject)
			{
				FComment Info;
				if (FJsonObjectConverter::JsonObjectToUStruct<FComment>(Element->AsObject().ToSharedRef(), &Info))
					Comments.Add(Info);
			}
			OnListCommentAvailable.ExecuteIfBound(Comments, 200, TEXT(""));
		});
	Request->ProcessRequest();
	return true;
}

bool UGithubApi::ListIssueComment(FOnListIssueCommentAvailable OnListCommentAvailable, int PullNumber, int Page, int NumberPage)
{
	if (!CheckGithubToken())
	{
		return false;
	}

	auto Settings = GetDefault<UGithubPullRequestSettings>();
	FHttpModule* Http = &FHttpModule::Get();
	FHttpRequestRef Request = Http->CreateRequest();
	auto BaseUrl = GetBaseUrl();
	auto Url = FString::Format(TEXT("{0}/issue/{1}/comments"), {BaseUrl, PullNumber});
	Url += "?page=" + FString::FromInt(Page);
	Url += "&per_page=" + FString::FromInt(NumberPage);

	Request->SetURL(Url);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw+json"));
	Request->SetVerb("GET");
	Request->OnProcessRequestComplete().BindLambda([OnListCommentAvailable](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
		{
			if (!InResponse.IsValid() || InResponse->GetResponseCode() != 200)
			{
				OnError(InResponse);
				return;
			}
			auto Content = InResponse->GetContentAsString();
			TSharedRef<TJsonReader<>> const JsonReader = TJsonReaderFactory<>::Create(Content);
			TArray<TSharedPtr<FJsonValue>> JsonObject;
			const TSharedPtr<FJsonObject>* ResultObject = nullptr;

			if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || JsonObject.IsEmpty())
			{
				OnError("Parse string to json failed");
				return;
			}
			TArray<FIssueComment> Comments;
			for (auto& Element : JsonObject)
			{
				FIssueComment Info;
				if (FJsonObjectConverter::JsonObjectToUStruct<FIssueComment>(Element->AsObject().ToSharedRef(), &Info))
					Comments.Add(Info);
			}
			OnListCommentAvailable.ExecuteIfBound(Comments, 200, TEXT(""));
		});
	Request->ProcessRequest();
	return true;
}

bool UGithubApi::ListCommit(FOnCommitListAvailable OnListCommitComplete, const FString& sha, const FString& path, const FString& author, const FString& committer, const FString& since, const FString& until, int page, int per_page)
{
	if (!CheckGithubToken())
	{
		return false;
	}

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
			if (!InResponse.IsValid() || InResponse->GetResponseCode() != 200)
			{
				OnError(InResponse);
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
	if (!CheckGithubToken())
	{
		OnListCommitComplete.ExecuteIfBound({}, 401, TEXT("GitHub token is not configured"));
		return false;
	}

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
			if (!InResponse.IsValid() || InResponse->GetResponseCode() != 200)
			{
				OnError(InResponse);
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

bool UGithubApi::GetCommitDetails(FOnCommitDetailsAvailable OnCommitDetailsAvailable, const FString& CommitSha)
{
	if (!CheckGithubToken())
	{
		OnCommitDetailsAvailable.ExecuteIfBound({}, 401, TEXT("GitHub token is not configured"));
		return false;
	}

	if (CommitSha.IsEmpty())
	{
		OnCommitDetailsAvailable.ExecuteIfBound({}, 400, TEXT("Commit SHA is empty"));
		return false;
	}

	auto Settings = GetDefault<UGithubPullRequestSettings>();
	FHttpModule* Http = &FHttpModule::Get();
	FHttpRequestRef Request = Http->CreateRequest();
	auto BaseUrl = GetBaseUrl();
	auto Url = FString::Format(TEXT("{0}/commits/{1}"), {BaseUrl, CommitSha});

	Request->SetURL(Url);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->GithubToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/vnd.github.raw+json"));
	Request->SetHeader(TEXT("X-GitHub-Api-Version"), TEXT("2022-11-28"));
	Request->SetVerb("GET");
	Request->OnProcessRequestComplete().BindLambda([OnCommitDetailsAvailable](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
		{
			if (!InResponse.IsValid())
			{
				OnCommitDetailsAvailable.ExecuteIfBound({}, 404, TEXT("Response is not valid"));
				return;
			}

			int Code = InResponse->GetResponseCode();
			if (Code != 200)
			{
				OnError(InResponse);
				return;
			}

			FCommitFullData CommitData;
			if (!FJsonObjectConverter::JsonObjectStringToUStruct<FCommitFullData>(InResponse->GetContentAsString(), &CommitData))
			{
				OnCommitDetailsAvailable.ExecuteIfBound({}, 600, TEXT("Parse json string failed"));
				return;
			}
			OnCommitDetailsAvailable.ExecuteIfBound(CommitData, 200, TEXT(""));
		});
	Request->ProcessRequest();
	return true;
}

FString ToString(EGitAuthorAssociation AuthorAssociation)
{
	switch (AuthorAssociation)
	{
	case EGitAuthorAssociation::COLLABORATOR:
		return "Collaborator";
	case EGitAuthorAssociation::CONTRIBUTOR:
		return "Contributor";
	case EGitAuthorAssociation::FIRST_TIMER:
		return "First Timer";
	case EGitAuthorAssociation::FIRST_TIME_CONTRIBUTOR:
		return "First Time Contributor";
	case EGitAuthorAssociation::MANNEQUIN:
		return "Mannequin";
	case EGitAuthorAssociation::MEMBER:
		return "Member";
	case EGitAuthorAssociation::NONE:
		return "";
	case EGitAuthorAssociation::OWNER:
		return "Author";
	default:
		break;
	}
	return "";
}
