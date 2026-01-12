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
	SessionStartedAtUTC = FDateTime::UtcNow().ToIso8601();
	SessionEndedAtUTC = TEXT("");
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

	TrySendUpsertSession();
	
	const float FlushInterval = Settings->FlushIntervalSeconds;
	World->GetTimerManager().SetTimer(
		TimerHandle,
		this,
		&UMetricFlowSubsystem::TrySendAppendEvents,
		FlushInterval,
		true
	);
}

void UMetricFlowSubsystem::Deinitialize()
{
	Super::Deinitialize();

	SessionEndedAtUTC = FDateTime::UtcNow().ToIso8601();
	TrySendUpsertSession();
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

bool UMetricFlowSubsystem::BuildUpsertSessionRequest(FMetricFlowPendingRequest& OutReq)
{
	FMetricFlowFields Session = FMetricFlowFields();
	Session.AddString(TEXT("sessionId"), SessionId);
	Session.AddString(TEXT("startedAtUTC"), SessionStartedAtUTC);
	Session.AddString(TEXT("endedAtUTC"), SessionEndedAtUTC);
	Session.AddMap(CachedSessionContext);

	const FString Json = MetricFlowJson::SerializeUpsertSessionToString(ProjectId, Session);
	if (Json.IsEmpty()) return false;

	OutReq = FMetricFlowPendingRequest();
	OutReq.Op = EMetricFlowOp::UpsertSession;
	OutReq.Json = Json;
	return true;
}

bool UMetricFlowSubsystem::BuildAppendEventsRequest(FMetricFlowPendingRequest& OutReq)
{
	if (EventQueue.Num() == 0) return false;
	
	const int32 Count = FMath::Min(BatchSize, EventQueue.Num());

	TArray<FMetricFlowEvent> Batch;
	Batch.Reserve(Count);
	Batch.Append(EventQueue.GetData(), Count);
	EventQueue.RemoveAt(0, Count, EAllowShrinking::No);

	const FString Json = MetricFlowJson::SerializeAppendEventsToString(ProjectId, SessionId, Batch);
	if (Json.IsEmpty())
	{
		EventQueue.Insert(Batch, 0);
		return false;
	}

	OutReq = FMetricFlowPendingRequest();
	OutReq.Op = EMetricFlowOp::AppendEvents;
	OutReq.Json = Json;
	OutReq.SendBatch = MoveTemp(Batch);
	
	return true;
}

void UMetricFlowSubsystem::TrySendUpsertSession()
{
	FMetricFlowPendingRequest Req;
	if (!BuildUpsertSessionRequest(Req)) return;
	SendRequest(MoveTemp(Req));
}

void UMetricFlowSubsystem::TrySendAppendEvents()
{
	FMetricFlowPendingRequest Req;
	if (!BuildAppendEventsRequest(Req)) return;
	SendRequest(MoveTemp(Req));
}

void UMetricFlowSubsystem::SendRequest(FMetricFlowPendingRequest Req)
{
	if (!bEnabledRuntime) return;
	if (ActiveEndpointURL.IsEmpty()) return;
	if (bIsFlushing) return;
	if (Req.Json.IsEmpty())
	{
		UE_LOG(LogMetricFlow, Warning, TEXT("UMetricFlowSubsystem::SendRequest: Empty JSON for %s"), ToString(Req.Op));
	}

	bIsFlushing = true;

	TWeakObjectPtr<UMetricFlowSubsystem> WeakThis(this);
	Sender.PostJson(ActiveEndpointURL, ProxyApiKey, Req.Json, TimeoutSeconds,
		[WeakThis, Req](bool bWasSuccessful, int32 ResponseCode, const FString& ResponseBody) 
		{
			if (!WeakThis.IsValid()) return;
			WeakThis->OnRequestCompleted(Req, bWasSuccessful, ResponseCode, ResponseBody);
		});
}

void UMetricFlowSubsystem::OnRequestCompleted(const FMetricFlowPendingRequest& Req, const bool bWasSuccessful, const int32 ResponseCode,
	const FString& ResponseBody)
{
	bIsFlushing = false;

	if (bWasSuccessful && ResponseCode >= 200 && ResponseCode < 300)
	{
		UE_LOG(LogMetricFlow, Log, TEXT("MetricFlow %s succeeded: code=%d body=%s"),ToString(Req.Op), ResponseCode, *ResponseBody);
		return;
	}

	UE_LOG(LogMetricFlow, Warning, TEXT("MetricFlow %s failed: code=%d body=%s"), ToString(Req.Op), ResponseCode, *ResponseBody);
	if (MetricFLow::ShouldRetry(bWasSuccessful, ResponseCode))
	{
		if (Req.Op == EMetricFlowOp::AppendEvents)
		{
			EventQueue.Insert(Req.SendBatch, 0);
		}
	}
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
