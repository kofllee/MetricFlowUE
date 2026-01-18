#include "MetricFlowDefaultEventContextProvider.h"


void UMetricFlowDefaultEventContextProvider::CollectFields_Implementation(UObject* WorldContextObject, TMap<FString, FString>& OutContext) const
{
	UWorld* World = nullptr;
	if (GEngine)
	{
		World = GEngine->GetWorldFromContextObject(
			WorldContextObject,
			EGetWorldErrorMode::ReturnNull
		);
	}
	if (!World) return;

	FString levelName = World->RemovePIEPrefix(World->GetMapName());
	OutContext.Add(TEXT("levelName"), levelName);
}
