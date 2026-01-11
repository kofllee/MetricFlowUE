#include "MetricFlowSubsystem.h"

#include "MetricFlowDefaultEventContextProvider.h"
#include "MetricFlowDefaultSessionContextProvider.h"
#include "MetricFlowEvent.h"
#include "MetricFlowFields.h"
#include "MetricFlowJson.h"

namespace MetricFLow
{
	static bool ShouldRetry(bool bWasSuccessful, int32 ResponseCode)
	{
		if (!bWasSuccessful)
		{
			return true;
		}

		if (ResponseCode >= 500 && ResponseCode < 600)
		{
			return true;
		}
		
		return false;
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

	ProjectId = Settings->ProjectId;
	ProxyApiKey = Settings->ProxyApiKey;
	
	ActiveEndpointURL = Settings->GetActiveEndpointURL();

	MaxQueueSize = Settings->MaxQueueSize;
	BatchSize = FMath::Max(1, Settings->BatchSize);

	TimeoutSeconds = Settings->RequestTimeoutSeconds;

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
	if (ProjectId.IsEmpty())
	{
		UE_LOG(LogMetricFlow, Error, TEXT("UMetricFlowSubsystem::Initialize: No ProjectId"));
		bEnabledRuntime = false;
		return;
	}

	if (!bEnabledRuntime)
	{
		return;
	}

	SessionId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
	LoadProvidersFromSettings(Settings);
	RebuildSessionContextCache();

	UE_LOG(
		LogMetricFlow,
		Log,
		TEXT("MetricFlow init: enabled=%d session=%s"),
		bEnabledRuntime ? 1 : 0,
		*SessionId
	);
	
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
		&UMetricFlowSubsystem::Flush,
		FlushInterval,
		true
	);
}

void UMetricFlowSubsystem::Deinitialize()
{
	Super::Deinitialize();
}


void UMetricFlowSubsystem::RecordEvent(const FName& EventName, const FMetricFlowFields& ExtraContext, const FMetricFlowFields& Payload, const FString& SheetOverride)
{
	FMetricFlowFields EventContext;
	for (const UMetricFlowContextProviderBase* Provider : EventContextProviders)
	{
		TMap<FString, FString> ProviderFields;
		Provider->CollectFields(this, ProviderFields);
		UE_LOG(LogMetricFlow, Verbose, TEXT("EventContext Provider %s fields:"), *Provider->GetName());
		EventContext = EventContext.AddMap(ProviderFields);
	}
	
	const FMetricFlowEvent Event = CreateMetricFlowEvent(EventName, EventContext + ExtraContext, Payload, SheetOverride);
	
	EventQueue.Add(Event);
	const int32 Overflow = EventQueue.Num() - MaxQueueSize;
	if (Overflow > 0)
		EventQueue.RemoveAt(0, Overflow, EAllowShrinking::No);
}

void UMetricFlowSubsystem::RecordEventMap(const FName& EventName, const TMap<FString, FString>& ExtraContextMap, const TMap<FString, FString>& PayloadMap, const FString& SheetOverride)
{
	
	const FMetricFlowFields ExtraContext = FMetricFlowFields().AddMap(ExtraContextMap);
	const FMetricFlowFields Payload = FMetricFlowFields().AddMap(PayloadMap);

	RecordEvent(EventName, ExtraContext, Payload, SheetOverride);
}


FMetricFlowEvent UMetricFlowSubsystem::CreateMetricFlowEvent(const FName& EventName, const FMetricFlowFields& Context,
	const FMetricFlowFields& Payload, const FString& SheetOverride)
{
	FMetricFlowEvent Event = FMetricFlowEvent();
	
	Event.EventName = EventName;
	Event.Context = Context;
	Event.TimestampUTC = FDateTime::UtcNow();
	Event.Payload = Payload;
	Event.SheetOverride = SheetOverride;
	return Event;
}

void UMetricFlowSubsystem::Flush()
{
	if (!bEnabledRuntime || EventQueue.Num() == 0) return;
	if (ActiveEndpointURL.IsEmpty()) return;
	if (bIsFlushing) return;
	
	const int32 Count = FMath::Min(BatchSize, EventQueue.Num());

	BatchEvents.Reset();
	BatchEvents.Append(EventQueue.GetData(), Count);
	EventQueue.RemoveAt(0, Count, EAllowShrinking::No);

	FMetricFlowFields SessionContext = FMetricFlowFields().AddMap(CachedSessionContext);
	const FString Json = MetricFlowJson::SerializeBatchToString(ProjectId, SessionContext, BatchEvents);
	if (Json.IsEmpty())
	{
		EventQueue.Insert(BatchEvents, 0);
		return;
	}

	bIsFlushing = true;

	TWeakObjectPtr<UMetricFlowSubsystem> WeakThis(this);
	Sender.PostJson(ActiveEndpointURL, ProxyApiKey, Json, TimeoutSeconds,
		[WeakThis](bool bWasSuccessful, int32 ResponseCode, const FString& ResponseBody) 
		{
			if (!WeakThis.IsValid()) return;
			
			WeakThis->bIsFlushing = false;
			
			if (bWasSuccessful && ResponseCode >= 200 && ResponseCode < 300)
			{
				UE_LOG(LogMetricFlow, Log, TEXT("MetricFlow flush succeeded: code=%d body=%s"), ResponseCode, *ResponseBody);
				return;
			}

			UE_LOG(LogMetricFlow, Warning, TEXT("MetricFlow flush failed: code=%d body=%s"), ResponseCode, *ResponseBody);
			if (MetricFLow::ShouldRetry(bWasSuccessful, ResponseCode))
				WeakThis->EventQueue.Insert(WeakThis->BatchEvents, 0);
		});
}

void UMetricFlowSubsystem::LoadProvidersFromSettings(const UMetricFlowSettings* Settings)
{
	for (TSubclassOf<UMetricFlowContextProviderBase> ProviderClass : Settings->SessionContextProviders)
	{
		if (!ProviderClass) continue;
		if (UMetricFlowContextProviderBase* Provider = NewObject<UMetricFlowContextProviderBase>(this, ProviderClass))
		{
			SessionContextProviders.Add(Provider);
		}
	}
	if (SessionContextProviders.Num() == 0)
	{
		if (UMetricFlowContextProviderBase* Provider =
			NewObject<UMetricFlowContextProviderBase>(this, UMetricFlowDefaultSessionContextProvider::StaticClass()))
		{
			SessionContextProviders.Add(Provider);
		}
	}

	for (TSubclassOf<UMetricFlowContextProviderBase> ProviderClass : Settings->EventContextProviders)
	{
		if (!ProviderClass) continue;
		if (UMetricFlowContextProviderBase* Provider = NewObject<UMetricFlowContextProviderBase>(this, ProviderClass))
		{
			EventContextProviders.Add(Provider);
		}
	}
	if (EventContextProviders.Num() == 0)
	{
		if (UMetricFlowContextProviderBase* Provider =
			NewObject<UMetricFlowContextProviderBase>(this, UMetricFlowDefaultEventContextProvider::StaticClass()))
		{
			EventContextProviders.Add(Provider);
		}
	}
}

void UMetricFlowSubsystem::RebuildSessionContextCache()
{
	CachedSessionContext.Reset();

	for (const UMetricFlowContextProviderBase* Provider : SessionContextProviders)
	{
		if (!Provider) continue;

		TMap<FString, FString> ProviderFields;
		Provider->CollectFields(this, ProviderFields);

		for (const TPair<FString, FString>& Pair : ProviderFields)
		{
			CachedSessionContext.Add(Pair.Key, Pair.Value);
		}
	}
}
