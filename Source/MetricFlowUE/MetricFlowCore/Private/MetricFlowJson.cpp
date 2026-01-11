#include "MetricFlowJson.h"

#include "MetricFlowEvent.h"
#include "MetricFlowFields.h"

static TSharedPtr<FJsonObject> SerializeFieldsToJson(const FMetricFlowFields& Fields)
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	for (const TPair<FString, FString>& Pair : Fields.Data)
	{
		Json->SetStringField(Pair.Key, Pair.Value);
	}
	return Json;
}

FString MetricFlowJson::SerializeBatchToString(
	const FString& ProjectId,
	const FMetricFlowFields& SessionContext,
	const TArray<FMetricFlowEvent>& Events)
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("projectId"),	ProjectId);
	Root->SetObjectField(TEXT("sessionContext"), SerializeFieldsToJson(SessionContext));

	TArray<TSharedPtr<FJsonValue>> JsonEvents;
	JsonEvents.Reserve(Events.Num());
	
	for (const FMetricFlowEvent& Event : Events)
	{
		TSharedPtr<FJsonObject> JsonEvent = MakeShared<FJsonObject>();
		JsonEvent->SetStringField(TEXT("eventName"), Event.EventName.ToString());
		JsonEvent->SetStringField(TEXT("timestampUTC"), Event.TimestampUTC.ToIso8601());
		JsonEvent->SetObjectField(TEXT("eventContext"), SerializeFieldsToJson(Event.Context));
		if (!Event.SheetOverride.IsEmpty())
		{
			JsonEvent->SetStringField(TEXT("sheet"), Event.SheetOverride);
		}
		JsonEvent->SetObjectField(TEXT("payload"), SerializeFieldsToJson(Event.Payload));

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
