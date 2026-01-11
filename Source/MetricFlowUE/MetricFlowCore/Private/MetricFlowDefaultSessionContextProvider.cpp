#include "MetricFlowDefaultSessionContextProvider.h"

namespace MetricFlow
{
	static FString GetProjectVersion()
	{
		FString ProjectVersion;
		GConfig->GetString(
			TEXT("/Script/EngineSettings.GeneralProjectSettings"),
			TEXT("ProjectVersion"),
			ProjectVersion,
			GGameIni
		);
		return ProjectVersion;
	}
}

void UMetricFlowDefaultSessionContextProvider::CollectFields_Implementation(UObject* WorldContextObject, TMap<FString, FString>& OutContext) const
{
	OutContext.Add(TEXT("projectName"), FApp::GetProjectName());
	OutContext.Add(TEXT("projectVersion"), MetricFlow::GetProjectVersion());
	OutContext.Add(TEXT("platform"), FPlatformProperties::PlatformName());
	OutContext.Add(TEXT("platformVersion"), FPlatformMisc::GetOSVersion());
}
