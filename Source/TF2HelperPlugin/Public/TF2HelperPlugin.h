// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Engine/World.h"
#include "LevelEditor.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "Engine/LevelScriptBlueprint.h"
#include "Toolkits/AssetEditorManager.h"

#include "TF2AimMatrixDirection.h"

class FTF2HelperPluginModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;


	static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets);
	static void CreateGenerateAimMatrixAssetMenu(FMenuBuilder& MenuBuilder, TArray<FAssetData> SelectedAssets);
	static void CreateAutoRenameAssetMenu(FMenuBuilder& MenuBuilder, TArray<FAssetData> SelectedAssets);
	static void GenerateAimMatrix(TArray<FAssetData> SelectedAssets);
	static void AutoRename(TArray<FAssetData> SelectedAssets);

};
