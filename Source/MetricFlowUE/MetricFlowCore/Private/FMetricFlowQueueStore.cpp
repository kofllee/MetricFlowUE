#include "FMetricFlowQueueStore.h"

#include "MetricFlowJson.h"
#include "MetricFlowEvent.h"

static bool WriteFile(const FString& FilePath, const FString& Content)
{
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();

	const FString Dir = FPaths::GetPath(FilePath);
	if (!PF.DirectoryExists(*Dir))
	{
		PF.CreateDirectoryTree(*Dir);
	}

	const FString TmpPath = FilePath + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(Content, *TmpPath))
	{
		return false;
	}

	if (PF.FileExists(*FilePath))
	{
		PF.DeleteFile(*FilePath);
	}

	return PF.MoveFile(*FilePath, *TmpPath);
}

static bool ReadFile(const FString& FilePath, FString& Out)
{
	return FFileHelper::LoadFileToString(Out, *FilePath);
}

FString FMetricFlowQueueStore::GetOutboxDir()
{
	return FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("MetricFlow"),
		TEXT("outbox")
	);
}

TArray<FString> FMetricFlowQueueStore::ListQueueFiles()
{
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	const FString Dir = GetOutboxDir();

	TArray<FString> Files;
	if (!PF.DirectoryExists(*Dir))
	{
		return Files;
	}

	PF.FindFilesRecursively(Files, *Dir, TEXT(".json"));

	Files.Sort([](const FString& A, const FString& B)
	{
		return A < B;
	});

	return Files;
}

FString FMetricFlowQueueStore::MakeOutboxFilePath()
{
	const FString Dir = GetOutboxDir();
	const int64 Ticks = FDateTime::UtcNow().GetTicks();
	
	const FString FileName = FString::Printf(TEXT("%lld_%s.json"), Ticks, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	return FPaths::Combine(Dir, FileName);
}


bool FMetricFlowQueueStore::Exists(const FString& FilePath)
{
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	return PF.FileExists(*FilePath);
}

bool FMetricFlowQueueStore::DeleteFile(const FString& FilePath)
{
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	return PF.FileExists(*FilePath) ? PF.DeleteFile(*FilePath) : true;
}


bool FMetricFlowQueueStore::SaveBatches(const FString& ProjectId, const FString& SessionId,
	const TArray<FMetricFlowEvent>& EventQueue, int32 MaxEventsPerBatch)
{
	MaxEventsPerBatch = FMath::Max(1, MaxEventsPerBatch);

	const int32 TotalEvents = EventQueue.Num();
	const int32 BatchesToWrite = FMath::DivideAndRoundUp(TotalEvents, MaxEventsPerBatch);
	
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	const FString Dir = GetOutboxDir();
	if (!PF.DirectoryExists(*Dir))
	{
		PF.CreateDirectoryTree(*Dir);
	}
	
	int32 Index = 0;
	
	for (int32 b = 0; b < BatchesToWrite; b++)
	{
		const int32 Remaining = TotalEvents - Index;
		if (Remaining <= 0) break;

		const int32 Count = FMath::Min(MaxEventsPerBatch, Remaining);

		TArray<FMetricFlowEvent> Batch;
		Batch.Reserve(Count);
		Batch.Append(EventQueue.GetData() + Index, Count);
		Index += Count;

		TSharedPtr<FJsonObject> AppendEventsJson =
			MetricFlowJson::SerializeAppendEventsToJson(ProjectId, SessionId, Batch);

		if (!AppendEventsJson.IsValid()) return false;

		TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetNumberField(TEXT("formatVersion"), CurrentFormatVersion);
		Root->SetStringField(TEXT("savedAtUTC"), FDateTime::UtcNow().ToIso8601());
		Root->SetObjectField(TEXT("appendEventsJson"), AppendEventsJson);

		const FString Out = MetricFlowJson::ToString(Root);
		if (Out.IsEmpty()) return false;

		const FString FilePath = MakeOutboxFilePath();
		if (!WriteFile(FilePath, Out))
		{
			return false;
		}
	}

	return true;

}

bool FMetricFlowQueueStore::LoadBatch(const FString& FilePath, FString& OutJson)
{
	OutJson.Reset();
	FString Content;
	if (!ReadFile(FilePath, Content)) return false;

	TSharedPtr<FJsonObject> Root;
	{
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
		if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
		{
			int32 Ver = 0;
			Root->TryGetNumberField(TEXT("formatVersion"), Ver);

			if (Ver != CurrentFormatVersion) return false;

			TSharedPtr<FJsonObject> AppendObj = Root->GetObjectField(TEXT("appendEventsJson"));
			if (AppendObj.IsValid())
			{
				FString Json = MetricFlowJson::ToString(AppendObj);
				OutJson = MoveTemp(Json);
				return true;
			}

			return false;
		}
	}

	return false;
}
