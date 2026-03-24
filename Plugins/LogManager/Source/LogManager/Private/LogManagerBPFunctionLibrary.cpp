// =============================================================================
// LogManagerBPFunctionLibrary.cpp
// LogManager Plugin for Unreal Engine 5
//
// Copyright (c) 2025 AIPIC Team, Research Center of Saint-Cyr Coetquidan
// All rights reserved.
//
// This file is part of the LogManager Plugin, licensed under the
// Creative Commons Attribution-NonCommercial 4.0 International License
// (CC BY-NC 4.0) with additional terms for software usage.
//
// You are free to:
//   - Share: copy and redistribute the material in any medium or format
//   - Adapt: remix, transform, and build upon the material
//
// Under the following terms:
//   - Attribution: You must give appropriate credit to the AIPIC Team,
//     Research Center of Saint-Cyr Coetquidan, provide a link to the license,
//     and indicate if changes were made.
//   - NonCommercial: You may not use the material for commercial purposes.
//
// Additional Terms for Software Usage:
//   - This plugin may be integrated into Unreal Engine projects for research,
//     educational, and non-commercial purposes only.
//   - Any derivative work or project using this plugin must retain this header
//     and provide attribution in accompanying documentation.
//   - Redistribution of this plugin, modified or unmodified, must preserve
//     this license header and must not be sold or sublicensed.
//
// Full license text: https://creativecommons.org/licenses/by-nc/4.0/legalcode
//
// For commercial licensing inquiries, contact:
//   AIPIC Team - Research Center of Saint-Cyr Coetquidan
// =============================================================================

#include "LogManagerBPFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "LogManager.h"
#include "LogManagerConstants.h"


// ----------------------------------------------------------------------------------------
// The Log Functions
// ----------------------------------------------------------------------------------------

// This event signals the start of an XP run: a file is open and the start event is added
void ULogManagerBPFunctionLibrary::WriteLog_XPStart(float gameTime, int64 userId, ESetting currentSetting, FVector startLocation)
{
	FDateTime date = FDateTime::Now();
	FString error;
	// Build a timestamped file name that encodes the date, time, participant ID, and setting.
	// Format: YYYY-MM-DD_HHhMMmSSs_id<userId>_<setting>.json
	FString filepath = LogManagerConstants::GetBaseLogDirectory() + FString::FromInt(date.GetYear()) + "-" +
		FString::Printf(TEXT("%02d"), date.GetMonth()) + "-" +
		FString::Printf(TEXT("%02d"), date.GetDay()) + "_" +
		FString::Printf(TEXT("%02d"), date.GetHour()) + "h" +
		FString::Printf(TEXT("%02d"), date.GetMinute()) + "m" +
		FString::Printf(TEXT("%02d"), date.GetSecond()) + "s_id" +
		LexToString((uint64)userId) + "_" + settingToString(currentSetting) + ".json";
	// Create/Open the file
	if (!FLogManagerModule::Get().OpenNewJSonFile(filepath)) return; // TODO: return false and handle it in BP
	UE_LOG(LogManager, Log, TEXT("Create file %s\n"), *filepath);
	// Write the XP_START event with 3 top-level fields:
	//   - PlayerId (uint64)
	//   - CurrentSetting (string)
	//   - StartLocation (composed: x, y, z)
	if (FLogManagerModule::Get().BeginLogEntry(EEvent::XP_START, gameTime, 3)) {

		FLogManagerModule::Get().AddULongLongData("PlayerId", (uint64)userId);
		FLogManagerModule::Get().AddStringData("CurrentSetting", settingToString(currentSetting));
		// StartLocation is written as a nested JSON object with 3 sub-fields
		FLogManagerModule::Get().AddComposedData("StartLocation", 3);
			FLogManagerModule::Get().AddFloatData("x", startLocation.X);
			FLogManagerModule::Get().AddFloatData("y", startLocation.Y);
			FLogManagerModule::Get().AddFloatData("z", startLocation.Z);

		FLogManagerModule::Get().FinalizeConfigOrLogEntry();
	}
}

// This event signal the end of an XP run: the event is added and log file is closed. 
void ULogManagerBPFunctionLibrary::WriteLog_XPStop(float gameTime)
{
	// Write the XP_STOP event with no additional fields.
	// Its presence in the file confirms that the session ended normally.
	if (FLogManagerModule::Get().BeginLogEntry(EEvent::XP_STOP, gameTime, 0)) {

		FLogManagerModule::Get().FinalizeConfigOrLogEntry();
	}
	// Close the log file
	FLogManagerModule::Get().FlushAndCloseJSonFile();
}

void ULogManagerBPFunctionLibrary::CreateNewConfigFile(int64 previousUserId, ESetting previousSetting, int64& nextId, ESetting& nextSetting) {
	FDateTime date = FDateTime::Now();
	FString error;
	// Derive the next participant ID from the previous one (via the shared library)
	nextId = FLogManagerModule::Get().GetNextID(previousUserId);
	// Cycle to the next setting in the ESetting enum sequence (wraps around using Count)
	nextSetting = static_cast<ESetting>((static_cast<uint8>(previousSetting) + 1) % static_cast<uint8>(ESetting::Count));
	// The config file is always written at a fixed path and overrides the previous one
	FString filepath = LogManagerConstants::GetBaseLogDirectory() + "config.json";
	// Create/Open the file
	if (!FLogManagerModule::Get().OpenNewJSonFile(filepath)) return;
	UE_LOG(LogManager, Log, TEXT("Create file %s\n"), *filepath);
	// Write the config block with 2 fields: PlayerId and Setting
	if (FLogManagerModule::Get().BeginConfigData(2)) {

		FLogManagerModule::Get().AddULongLongData("PlayerId", (uint64)nextId);
		FLogManagerModule::Get().AddStringData("Setting", settingToString(nextSetting));
		FLogManagerModule::Get().FinalizeConfigOrLogEntry();
	}
	// Close the log file
	FLogManagerModule::Get().FlushAndCloseJSonFile();
}

bool ULogManagerBPFunctionLibrary::GetConfigFileInfos(int64& userId, ESetting& currentSetting) {
	FString filepath = LogManagerConstants::GetBaseLogDirectory() + "config.json";
	// Attempt to load and deserialize the config file (with built-in retry logic)
	TSharedPtr<FJsonObject> jsonObject = FLogManagerModule::Get().LoadConfigJson(filepath);
	if (!jsonObject) {
		UE_LOG(LogManager, Error, TEXT("Failed to load config file %s\n"), *filepath);
		return false;
	}
	// Extract the participant ID (stored as uint64, read back as int64)
	if (!FLogManagerModule::Get().GetJsonField(jsonObject, "PlayerId", userId)) {
		UE_LOG(LogManager, Error, TEXT("Failed to get user id in %s\n"), *filepath);
		return false;
	}
	// Extract the setting name and convert it back to the ESetting enum value
	FString settingText;
	if (!FLogManagerModule::Get().GetJsonField(jsonObject, "Setting", settingText)) {
		UE_LOG(LogManager, Error, TEXT("Failed to get setting in %s\n"), *filepath);
		return false;
	}
	else {
		currentSetting = stringToSetting(TCHAR_TO_ANSI(*settingText));
	}
	return true;
}

void ULogManagerBPFunctionLibrary::WriteLog_StartSurvey(float gameTime, int64 userId, int evaluation, bool usefullness, FString comment)
{
	FDateTime date = FDateTime::Now();
	FString error;
	// Build the file path with a "_startSurvey" suffix to distinguish it from XP log files.
	// Format: YYYY-MM-DD_HHhMMmSSs_id<userId>_startSurvey.json
	FString filepath = LogManagerConstants::GetBaseLogDirectory() + FString::FromInt(date.GetYear()) + "-" +
		FString::Printf(TEXT("%02d"), date.GetMonth()) + "-" +
		FString::Printf(TEXT("%02d"), date.GetDay()) + "_" +
		FString::Printf(TEXT("%02d"), date.GetHour()) + "h" +
		FString::Printf(TEXT("%02d"), date.GetMinute()) + "m" +
		FString::Printf(TEXT("%02d"), date.GetSecond()) + "s_id" +
		LexToString((uint64)userId) + "_startSurvey.json";
	// Create/Open the file
	if (!FLogManagerModule::Get().OpenNewJSonFile(filepath)) return; // TODO: return false and handle it in BP
	UE_LOG(LogManager, Log, TEXT("Create file %s\n"), *filepath);
	// Write the START_SURVEY event with 3 fields: Evaluation, Usefulness, Comment
	if (FLogManagerModule::Get().BeginLogEntry(EEvent::START_SURVEY, gameTime, 3)) {
		FLogManagerModule::Get().AddIntData("Evaluation", evaluation);
		FLogManagerModule::Get().AddBoolData("Usefullness", usefullness);
		FLogManagerModule::Get().AddStringData("Comment", comment);

		FLogManagerModule::Get().FinalizeConfigOrLogEntry();
	}
	// Close the log file
	FLogManagerModule::Get().FlushAndCloseJSonFile();
}

void ULogManagerBPFunctionLibrary::WriteLog_FinalSurvey(float gameTime, TMap<FString, FString> questionsAndAnswers)
{
	// Write the FINAL_SURVEY event with a single composed field "Answers"
	// containing one sub-field per question-answer pair.
	if (FLogManagerModule::Get().BeginLogEntry(EEvent::FINAL_SURVEY, gameTime, 1)) {
		FLogManagerModule::Get().AddComposedData("Answers", questionsAndAnswers.Num());
		for (const auto& Elem : questionsAndAnswers) {
			FLogManagerModule::Get().AddStringData(Elem.Key, Elem.Value);
		}
		FLogManagerModule::Get().FinalizeConfigOrLogEntry();
	}
}

void ULogManagerBPFunctionLibrary::WriteLog_CubeInteraction(float gameTime, bool isGrabbed, float timer, int64 distance, FName name)
{
	// Write the CUBE_INTERACTION event with 4 fields:
	//   - IsGrabbed (bool): true = grab, false = release
	//   - Timer (float): seconds elapsed since last interaction with this cube
	//   - Distance (int64): distance in Unreal units between participant and cube
	//   - Name (string): identifier of the cube involved
	if (FLogManagerModule::Get().BeginLogEntry(EEvent::CUBE_INTERACTION, gameTime, 4)) {
		FLogManagerModule::Get().AddBoolData("IsGrabbed", isGrabbed);
		FLogManagerModule::Get().AddFloatData("Timer", timer);
		FLogManagerModule::Get().AddLongLongData("Distance", distance);
		FLogManagerModule::Get().AddStringData("Name", name.ToString());

		FLogManagerModule::Get().FinalizeConfigOrLogEntry();
	}
}

void ULogManagerBPFunctionLibrary::WriteLog_BoxEvent(float gameTime, int32 number, FVector location, bool inside)
{
	// Write the BOX_EVENT event with 3 fields:
	//   - Number (uint32): index of the box/zone that triggered the event
	//   - Location (composed: x, y, z): world-space position of the zone
	//   - Inside (bool): true = participant entered the zone, false = exited
	if (FLogManagerModule::Get().BeginLogEntry(EEvent::BOX_EVENT, gameTime, 3)) {
		FLogManagerModule::Get().AddUIntData("Number", static_cast<uint32>(number));
		// Location is written as a nested JSON object with 3 sub-fields
		FLogManagerModule::Get().AddComposedData("Location", 3);
			FLogManagerModule::Get().AddFloatData("x", location.X);
			FLogManagerModule::Get().AddFloatData("y", location.Y);
			FLogManagerModule::Get().AddFloatData("z", location.Z);
		FLogManagerModule::Get().AddBoolData("Inside", inside);

		FLogManagerModule::Get().FinalizeConfigOrLogEntry();
	}
}
