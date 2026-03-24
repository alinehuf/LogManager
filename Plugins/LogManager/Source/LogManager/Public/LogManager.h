// =============================================================================
// LogManager.h
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

/**
 * @file LogManager.h
 * @brief Declaration of the FLogManagerModule class and helper functions.
 *
 * FLogManagerModule is the core UE5 module of the LogManager plugin. It acts as
 * a bridge between Unreal Engine and a third-party C shared library (liblogmanager),
 * which handles the actual JSON file writing. The module is implemented as a singleton
 * accessible via FLogManagerModule::Get().
 *
 * Responsibilities:
 *   - Loading and unloading the shared library (DLL/SO) at module startup/shutdown
 *   - Resolving all function entry points from the shared library
 *   - Exposing a high-level API for opening/closing JSON log files, writing log entries,
 *     and appending typed key-value data fields
 *   - Providing utilities to load and query a JSON configuration file
 *
 * The module is used exclusively through the Blueprint Function Library
 * (ULogManagerBPFunctionLibrary), which provides Blueprint-accessible wrappers
 * for all logging operations.
 */

#pragma once

#include "Modules/ModuleManager.h"
#include "LogManagerConstants.h"

#include "Json.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

/** Declares the log category used throughout the LogManager plugin. */
DECLARE_LOG_CATEGORY_EXTERN(LogManager, Log, All);

// ----------------------------------------------------------------------------------------
// Helper functions
// ----------------------------------------------------------------------------------------

/**
 * Converts an EEvent enum value to its string representation.
 * Used when writing the event type field in JSON log entries.
 *
 * @param event  The EEvent value to convert.
 * @return       The name string of the enum value, or "?" if the enum is invalid.
 */
FString eventToString(EEvent event);

/**
 * Converts an ESetting enum value to its string representation.
 * Used when writing the experiment setting field in log and config files.
 *
 * @param setting  The ESetting value to convert.
 * @return         The name string of the enum value, or "?" if the enum is invalid.
 */
FString settingToString(ESetting setting);

/**
 * Converts a C-string to its corresponding ESetting enum value.
 * Used when reading the setting field back from a JSON config file.
 * Falls back to the first enum value (index 0) if the string is null or unrecognized.
 *
 * @param str  A null-terminated C-string matching an ESetting enum name.
 * @return     The matching ESetting value, or the default (index 0) if not found.
 */
ESetting stringToSetting(const char* str);

// ----------------------------------------------------------------------------------------


/**
 * FLogManagerModule
 *
 * Main module class for the LogManager plugin. Manages the lifecycle of the
 * third-party logging shared library and exposes its functionality to the rest
 * of the plugin via a clean C++ API.
 *
 * This class is a singleton: use FLogManagerModule::Get() to access it.
 * Do not instantiate it directly.
 *
 * Architecture overview:
 *   - The module loads a platform-specific shared library (liblogmanager) at startup.
 *   - All logging operations are delegated to function pointers resolved from that library.
 *   - The library handles thread safety internally via lock/unlock mechanisms.
 *   - Log data is written as JSON arrays to timestamped files in the LOGS/ directory.
 *
 * Typical write sequence for a log entry:
 *   1. OpenNewJSonFile()        — create and open a new JSON log file
 *   2. BeginLogEntry()          — lock the write buffer and start a new JSON event object
 *   3. Add*Data() calls         — append typed key-value fields to the current entry
 *   4. FinalizeConfigOrLogEntry() — finalize the entry and release the write lock
 *   5. FlushAndCloseJSonFile()  — flush and close the current file
 */
class FLogManagerModule : public IModuleInterface
{
private:
	/**
	 * Loads the platform-specific shared library (DLL on Windows, SO on Android).
	 * Sets reasonFails if the library cannot be found or loaded.
	 *
	 * @param reasonFails  Output string describing the failure reason, if any.
	 * @return             Opaque handle to the loaded library, or nullptr on failure.
	 */
    void* GetLogManagerLibraryHandle(FString& reasonFails);

	/**
	 * Resolves a named entry point (function pointer) from the loaded shared library.
	 * Requires that logManagerLibraryHandle is valid before calling.
	 *
	 * @param EntryPointName  Null-terminated name of the exported function to resolve.
	 * @param reasonFails     Output string describing the failure reason, if any.
	 * @return                Raw function pointer, or nullptr if resolution fails.
	 */
    void* GetLogManagerLibraryEntryPoint(const char* EntryPointName, FString& reasonFails);

	/**
	 * Performs full plugin initialization:
	 *   - Ensures the log output directory exists (creates it if needed)
	 *   - Loads the shared library via GetLogManagerLibraryHandle()
	 *   - Resolves all required function entry points
	 *   - Calls initLogger() from the shared library to start the logging subsystem
	 *
	 * @param reasonFails  Output string describing the failure reason, if any.
	 * @return             True if all steps succeeded, false otherwise.
	 */
    bool Init(FString& reasonFails);

	/**
	 * Shuts down the logging subsystem and releases the shared library handle.
	 * Called automatically by ShutdownModule().
	 */
    void Shutdown();

    /** Opaque handle to the loaded liblogmanager shared library. Null if not loaded. */
    void* logManagerLibraryHandle = nullptr;

public:
	/**
	 * Returns the singleton instance of this module.
	 * Equivalent to FModuleManager::LoadModuleChecked<FLogManagerModule>("LogManager").
	 *
	 * @return Reference to the active FLogManagerModule instance.
	 */
    static FLogManagerModule& Get();

    /** Called by Unreal Engine when the module is loaded. Triggers Init(). */
    virtual void StartupModule() override;

    /** Called by Unreal Engine when the module is unloaded. Triggers Shutdown(). */
    virtual void ShutdownModule() override;

	/**
	 * Creates and opens a new JSON log file at the specified path.
	 * Any previously open file must be closed before calling this.
	 *
	 * @param fileName  Full path of the JSON file to create/open.
	 * @return          True on success, false if the file could not be opened.
	 */
    bool OpenNewJSonFile(FString& fileName);

	/**
	 * Finalizes, flushes, and closes the currently open JSON log file.
	 * Must be called after all log entries for a session have been written.
	 *
	 * @return True on success, false if the close operation failed.
	 */
    bool FlushAndCloseJSonFile();

	/**
	 * Begins a new log entry in the currently open JSON file.
	 * Acquires the internal write lock and initializes a new JSON event object.
	 * Must be followed by Add*Data() calls and then FinalizeConfigOrLogEntry().
	 *
	 * @param ename     The event type (EEvent) to associate with this log entry.
	 * @param gameTime  In-game time (in seconds) at the moment of the event.
	 * @param nbFields  Number of key-value fields that will be added to this entry.
	 * @return          True if the entry was successfully started, false otherwise.
	 */
    bool BeginLogEntry(EEvent ename, float gameTime, int nbFields);

	/**
	 * Begins a new configuration data block in the currently open JSON file.
	 * Used specifically for writing the config.json file (player ID + setting).
	 * Acquires the internal write lock; must be followed by Add*Data() calls
	 * and then FinalizeConfigOrLogEntry().
	 *
	 * @param nbFields  Number of key-value fields that will be added to this config block.
	 * @return          True if the config block was successfully started, false otherwise.
	 */
    bool BeginConfigData(int nbFields);

	/**
	 * Finalizes the current log entry or config data block and releases the write lock.
	 * Must always be called after a successful BeginLogEntry() or BeginConfigData(),
	 * even if an error occurred during the Add*Data() calls.
	 *
	 * @return True on success, false if releasing the write lock failed.
	 */
    bool FinalizeConfigOrLogEntry();

	/**
	 * Appends a string key-value pair to the current log entry.
	 * Must be called between BeginLogEntry()/BeginConfigData() and FinalizeConfigOrLogEntry().
	 *
	 * @param key    The field name.
	 * @param value  The string value to write.
	 * @return       True on success, false on write failure.
	 */
    bool AddStringData(const FString key, const FString& value);

	/**
	 * Appends a signed integer (int) key-value pair to the current log entry.
	 *
	 * @param key    The field name.
	 * @param value  The integer value to write.
	 * @return       True on success, false on write failure.
	 */
    bool AddIntData(const FString& key, int value);

	/**
	 * Appends a float key-value pair to the current log entry.
	 *
	 * @param key    The field name.
	 * @param value  The float value to write.
	 * @return       True on success, false on write failure.
	 */
    bool AddFloatData(const FString& key, float value);

	/**
	 * Appends a boolean key-value pair to the current log entry.
	 *
	 * @param key    The field name.
	 * @param value  The boolean value to write.
	 * @return       True on success, false on write failure.
	 */
    bool AddBoolData(const FString& key, bool value);

	/**
	 * Appends an unsigned integer (uint32) key-value pair to the current log entry.
	 *
	 * @param key    The field name.
	 * @param value  The unsigned integer value to write.
	 * @return       True on success, false on write failure.
	 */
    bool AddUIntData(const FString& key, unsigned int value);

	/**
	 * Appends a signed 64-bit integer key-value pair to the current log entry.
	 *
	 * @param key    The field name.
	 * @param value  The signed long long value to write.
	 * @return       True on success, false on write failure.
	 */
    bool AddLongLongData(const FString& key, signed long long value);

	/**
	 * Appends an unsigned 64-bit integer key-value pair to the current log entry.
	 * Typically used for player IDs to avoid sign issues with large values.
	 *
	 * @param key    The field name.
	 * @param value  The unsigned long long value to write.
	 * @return       True on success, false on write failure.
	 */
    bool AddULongLongData(const FString& key, unsigned long long value);

	/**
	 * Begins a nested JSON object within the current log entry.
	 * The following nbSubPairs calls to Add*Data() will be written as children of this object.
	 * Typically used for structured data like 3D vectors (x, y, z).
	 *
	 * @param key         The field name for the nested object.
	 * @param nbSubPairs  Number of child key-value fields that will follow.
	 * @return            True on success, false on write failure.
	 */
    bool AddComposedData(const FString& key, unsigned int nbSubPairs);

	/**
	 * Generates the next unique player/session ID based on the previous one.
	 * Delegates to the shared library's getNextID function.
	 *
	 * @param OriginalId  The previous player ID.
	 * @return            A new unique ID derived from OriginalId.
	 */
    long long GetNextID(long long OriginalId);

	/**
	 * Loads and deserializes a JSON config file from the specified path.
	 * Retries up to 10 times with 100ms delays in case of transient read failures
	 * (e.g., file still being written by another process).
	 * The config file is expected to contain a JSON array with a single object element.
	 *
	 * @param FilePath  Full path to the JSON config file to read.
	 * @return          A shared pointer to the parsed JSON object, or nullptr on failure.
	 */
	TSharedPtr<FJsonObject> LoadConfigJson(const FString& FilePath);

	/**
	 * Retrieves a typed value from a JSON object by field name.
	 * Supported types: FString, int32, uint32, int64, uint64, float.
	 * Returns false if the JSON object is invalid, the field does not exist,
	 * or the template type T is not one of the supported types.
	 *
	 * Template specialization is resolved at compile time via if constexpr.
	 * This function is defined in the header because Unreal Engine does not support
	 * template function definitions in .cpp files across module boundaries.
	 *
	 * @param JsonObject  The JSON object to query.
	 * @param FieldName   The name of the field to retrieve.
	 * @param OutValue    Output variable that receives the field value on success.
	 * @return            True if the field was found and successfully cast to T, false otherwise.
	 */
	template<typename T>
	bool GetJsonField(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, T& OutValue)
	{
		if (!JsonObject.IsValid() || !JsonObject->HasField(FieldName))
			return false;

		const TSharedPtr<FJsonValue> Value = JsonObject->TryGetField(FieldName);

		if constexpr (std::is_same_v<T, FString>)
		{
			OutValue = Value->AsString();
			return true;
		}
		else if constexpr (std::is_same_v<T, int32>)
		{
			OutValue = (int32)Value->AsNumber();
			return true;
		}
		else if constexpr (std::is_same_v<T, uint32>)
		{
			OutValue = (uint32)Value->AsNumber();
			return true;
		}
		else if constexpr (std::is_same_v<T, int64>)
		{
			OutValue = (int64)Value->AsNumber();
			return true;
		}
		else if constexpr (std::is_same_v<T, uint64>)
		{
			OutValue = (uint64)Value->AsNumber();
			return true;
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			OutValue = (float)Value->AsNumber();
			return true;
		}
		return false;
	}
};
