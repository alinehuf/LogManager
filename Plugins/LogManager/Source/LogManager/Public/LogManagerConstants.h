// =============================================================================
// LogManagerConstants.h
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

/*
 * This file contains constants and enums used by the Log Manager plugin.
 * It is included in both the plugin module and the Blueprint Function Library, 
 * so it should not contain any code that could cause static linkage issues
 * (like static variables or non-inline functions).
 */
#pragma once
#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "LogManagerConstants.generated.h"


/* Constants used in the blueprint library
 * Use inline functions to avoid static linkage issues
 */
namespace LogManagerConstants
{
	/**
	 * Returns the base directory path where log files are stored.
	 * Resolves to <ProjectPersistentDownloadDir>/LOGS/
	 * Using an inline function avoids static linkage issues across translation units.
	 *
	 * @return Absolute path to the log output directory (with trailing slash).
	 */
	inline FString GetBaseLogDirectory() {
		// log files directory
		return FPaths::Combine(FPaths::ProjectPersistentDownloadDir(), "LOGS/");
	}
}

/* 
 * Enum for different XP Conditions: don't change the enum name!
 * The values of the enum are not important, but the name and type of the enum must not be changed, 
 * as they are used in the Blueprint Function Library.
 *
 * ESetting represents the physical configuration of the experiment setup,
 * specifically the position of an optional table relative to the participant.
 */ 
UENUM(BlueprintType)
enum class ESetting : uint8 {
	TableLeft		UMETA(DisplayName = "Table On The Left"),   // Table placed to the left of the participant
	NoTable			UMETA(DisplayName = "No Table"),            // No table present during the XP
	TableRight		UMETA(DisplayName = "Table On The Right"),  // Table placed to the right of the participant
	Count			UMETA(Hidden)	                            // Sentinel value: used to cycle through settings, do not use directly
};

/* Enum for the different events: don't change the enum name !
 * The values of the enum are not important, but the name and type of the enum must not be changed, 
 * as they are used in the Blueprint Function Library.
 *
 * EEvent identifies the type of a log entry written to a JSON log file.
 * Each value corresponds to a specific stage or action in an XP (experiment) session.
 */
UENUM(BlueprintType)
enum class EEvent : uint8 {
	// For the start survey or an XP
	START_SURVEY,
	// Start of XP stage: records user id, xp settings...
	XP_START,
	// Records a specific action of the user
	BOX_EVENT,
	// Records the interaction of the user with a cube
	CUBE_INTERACTION,
	// End of XP stage: allow you to check is log file is properly completed
	XP_STOP,
	// For the final survey or each XP run
	FINAL_SURVEY
};
