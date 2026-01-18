#pragma once
#include "CoreMinimal.h"

struct FMetricFlowEvent;

struct FMetricFlowQueueStore
{
	static constexpr int32 CurrentFormatVersion = 1;

	
	static FString GetOutboxDir();
	static TArray<FString> ListQueueFiles();
	static FString MakeOutboxFilePath();
	static bool Exists(const FString& FilePath);
	static bool DeleteFile(const FString& FilePath);

	static bool SaveBatches(
		const FString& ProjectId,
		const FString& SessionId,
		const TArray<FMetricFlowEvent>& EventQueue,
		int32 MaxEventsPerBatch);

	static bool LoadBatch(const FString& FilePath, FString& OutJson);
};
