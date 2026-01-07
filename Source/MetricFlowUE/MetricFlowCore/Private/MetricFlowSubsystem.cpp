#include "MetricFlowSubsystem.h"
#include "MetricFlowEvent.h"
#include "MetricFlowPayload.h"

namespace MetricFLow
{
	static FString GetLevelNameSafe(const UWorld* World)
	{
		if (!World)
		{
			return FString();
		}
		
		const FString Raw = World->GetMapName();
		return UWorld::RemovePIEPrefix(Raw);
	}

}

void UMetricFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UMetricFlowSettings* Settings = UMetricFlowSettings::Get();
	if (!Settings)
	{
		UE_LOG(LogMetricFlow, Error, TEXT("UMetricFlowSubsystem::Initialize: Settings not found"));
		bEnabledRuntime = false;
		return;
	}

#if UE_BUILD_SHIPPING
	if (!Settings->bSendInShipping)
	{
		bEnabledRuntime = false;
		return;
	}
#endif

	bEnabledRuntime = Settings->bEnableMetricFlow;

	ActiveEndpointURL = Settings->GetActiveEndpointURL();
	ActiveDefaultSheet = Settings->GetActiveSheet();

	MaxQueueSize = Settings->MaxQueueSize;
	BatchSize = Settings->BatchSize;

	if (ActiveEndpointURL.IsEmpty())
	{
		UE_LOG(LogMetricFlow, Error, TEXT("UMetricFlowSubsystem::Initialize: No active endpoint"));
		bEnabledRuntime = false;
		return;
	}
	if (MaxQueueSize < 0)
	{
		UE_LOG(LogMetricFlow, Error, TEXT("UMetricFlowSubsystem::Initialize: Invalid MaxQueueSize=%d"), MaxQueueSize);
		bEnabledRuntime = false;
		return;
	}

	Context.SessionId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
	Context.UserId = FString();
	Context.BuildVersion = FApp::GetBuildVersion();
	Context.Platform = FString(FPlatformProperties::PlatformName());
	Context.PlatformVersion = FPlatformMisc::GetOSVersion();
	Context.LevelName = MetricFLow::GetLevelNameSafe(GetWorld());

	UE_LOG(
		LogMetricFlow,
		Log,
		TEXT("MetricFlow init: enabled=%d session=%s sheet=%s url=%s build=%s platform=%s level=%s"),
		bEnabledRuntime ? 1 : 0,
		*Context.SessionId,
		*ActiveDefaultSheet,
		*ActiveEndpointURL,
		*Context.BuildVersion,
		*Context.Platform,
		*Context.LevelName
	);

	if (!bEnabledRuntime)
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogMetricFlow, Warning, TEXT("UMetricFlowSubsystem::Initialize: World is null, flush timer not started"));
		return;
	}

	const float FlushInterval = Settings->FlushIntervalSeconds;
	World->GetTimerManager().SetTimer(
		TimerHandle,
		this,
		&UMetricFlowSubsystem::TickFlush,
		FlushInterval,
		true
	);
}

void UMetricFlowSubsystem::Deinitialize()
{
	Super::Deinitialize();
}


void UMetricFlowSubsystem::RecordEvent(const FName& EventName, const FMetricFlowPayload& Payload, const FString& SheetOverride)
{
	FMetricFlowEvent Event = CreateMetricFlowEvent(EventName, Payload, SheetOverride);
	Event.TimestampUTC = FDateTime::UtcNow();
	
	Context.LevelName = MetricFLow::GetLevelNameSafe(GetWorld());
	Event.Context = Context;
	
	EventQueue.Add(Event);
	const int32 Overflow = EventQueue.Num() - MaxQueueSize;
	if (Overflow > 0)
		EventQueue.RemoveAt(0, Overflow);
}

void UMetricFlowSubsystem::RecordEventMap(const FName& EventName, const TMap<FString, FString>& Map, const FString& SheetOverride)
{
	FMetricFlowPayload Payload = CreatePayloadFromMap(Map);

	RecordEvent(EventName, Payload, SheetOverride);
}


FMetricFlowEvent UMetricFlowSubsystem::CreateMetricFlowEvent(const FName& EventName, const FMetricFlowPayload& Payload,
		const FString& SheetOverride)
{
	FMetricFlowEvent Event;
	Event.EventName = EventName;
	Event.Payload = Payload;
	Event.SheetOverride = SheetOverride;
	return Event;
}

void UMetricFlowSubsystem::TickFlush()
{
	if (!bEnabledRuntime || EventQueue.Num() == 0) return;

	
}

FMetricFlowPayload UMetricFlowSubsystem::CreatePayloadFromMap(const TMap<FString, FString>& Map)
{
	FMetricFlowPayload Payload;
	for (const TPair<FString, FString>& Pair : Map)
	{
		Payload.Add(Pair.Key, Pair.Value);
	}
	return Payload;
}
