#include "MetricFlowJson.h"

#include "MetricFlowContext.h"
#include "MetricFlowEvent.h"
#include "MetricFlowPayload.h"

static TSharedPtr<FJsonObject> SerializeContextToJson(const FMetricFlowContext& Context)
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("sessionId"), Context.SessionId);
	Json->SetStringField(TEXT("userId"), Context.UserId);
	Json->SetStringField(TEXT("buildVersion"), Context.BuildVersion);
	Json->SetStringField(TEXT("platform"), Context.Platform);
	Json->SetStringField(TEXT("platformVersion"), Context.PlatformVersion);
	Json->SetStringField(TEXT("levelName"),	Context.LevelName);
	return Json;
}

static TSharedPtr<FJsonObject> SerializePayloadToJson(const FMetricFlowPayload& Payload)
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	for (const TPair<FString, FString>& Pair : Payload.Data)
	{
		Json->SetStringField(Pair.Key, Pair.Value);
	}
	return Json;
}

FString MetricFlowJson::SerializeBatchToString(
	const FString& ProjectToken,
	const FString& DefaultSheet,
	const TArray<FMetricFlowEvent>& Events)
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("token"),	ProjectToken);
	Root->SetStringField(TEXT("defaultSheet"), DefaultSheet);

	TArray<TSharedPtr<FJsonValue>> JsonEvents;
	JsonEvents.Reserve(Events.Num());
	
	for (const FMetricFlowEvent& Event : Events)
	{
		TSharedPtr<FJsonObject> JsonEvent = MakeShared<FJsonObject>();
		JsonEvent->SetStringField(TEXT("eventName"), Event.EventName.ToString());
		JsonEvent->SetStringField(TEXT("timestampUTC"), Event.TimestampUTC.ToIso8601());
		JsonEvent->SetObjectField(TEXT("context"), SerializeContextToJson(Event.Context));
		if (!Event.SheetOverride.IsEmpty())
		{
			JsonEvent->SetStringField(TEXT("sheet"), Event.SheetOverride);
		}
		JsonEvent->SetObjectField(TEXT("payload"), SerializePayloadToJson(Event.Payload));

		JsonEvents.Add(MakeShared<FJsonValueObject>(JsonEvent));
	}

	Root->SetArrayField(TEXT("events"), JsonEvents);

	FString out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&out);
	if (FJsonSerializer::Serialize(Root.ToSharedRef(), Writer))
	{
		Writer->Close();
		return out;
	}
	
	UE_LOG(LogMetricFlow, Error, TEXT("MetricFlowJson::SerializeBatchToString: Failed to serialize JSON"));
	return FString();
}
