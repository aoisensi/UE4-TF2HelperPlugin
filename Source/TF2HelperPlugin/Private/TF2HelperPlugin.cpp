// Copyright Epic Games, Inc. All Rights Reserved.

#include "TF2HelperPlugin.h"
#include "EditorAssetLibrary.h"
#include "Templates/SharedPointer.h"
#include "Animation/AimOffsetBlendSpace.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"

#define LOCTEXT_NAMESPACE "FTF2HelperPluginModule"

namespace
{
	FContentBrowserMenuExtender_SelectedAssets ContentBrowserExtenderDelegate;
	FDelegateHandle ContentBrowserExtenderDelegateHandle;
}

void FTF2HelperPluginModule::StartupModule()
{
	if (IsRunningCommandlet()) { return; }
	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	ContentBrowserExtenderDelegate =
		FContentBrowserMenuExtender_SelectedAssets::CreateStatic(
			&FTF2HelperPluginModule::OnExtendContentBrowserAssetSelectionMenu
		);
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates =
		ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	CBMenuExtenderDelegates.Add(ContentBrowserExtenderDelegate);
	ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last().GetHandle();
}

void FTF2HelperPluginModule::ShutdownModule()
{
	FContentBrowserModule* ContentBrowserModule =
		FModuleManager::GetModulePtr<FContentBrowserModule> (TEXT("ContentBrowser"));
	if (nullptr != ContentBrowserModule)
	{
		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates =
			ContentBrowserModule->GetAllAssetViewContextMenuExtenders();
		CBMenuExtenderDelegates.RemoveAll([](const FContentBrowserMenuExtender_SelectedAssets& Delegate)
			{ return Delegate.GetHandle() == ContentBrowserExtenderDelegateHandle; });
	}
}


TSharedRef<FExtender>FTF2HelperPluginModule::OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender>Extender(new FExtender());

	for (auto ItAsset = SelectedAssets.CreateConstIterator(); ItAsset; ++ItAsset)
	{
		if ((*ItAsset).AssetClass == UAnimSequence::StaticClass() -> GetFName())
		{
			Extender -> AddMenuExtension(
				TEXT("CommonAssetActions"),
				EExtensionHook::First,
				nullptr,
				FMenuExtensionDelegate::CreateStatic(& FTF2HelperPluginModule::CreateAssetMenu, SelectedAssets)
			);
			return Extender;
		}
	}

	return Extender;
}

void FTF2HelperPluginModule::CreateAssetMenu(FMenuBuilder& MenuBuilder, TArray<FAssetData>SelectedAssets)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("GenerateAimMatrix", "Generate AimMatrix"),
		LOCTEXT("GenerateAimMatrix_Tooltip", "Generate AimMatrix"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(& FTF2HelperPluginModule::GenerateAimMatrix, SelectedAssets)),
		NAME_None,
		EUserInterfaceActionType::Button
	);
}

void FTF2HelperPluginModule::GenerateAimMatrix(TArray<FAssetData> SelectedAssets)
{

	for (auto SelectedAsset : SelectedAssets) {
		TArray<FTF2AimMatrixDirection> Directions;
		Directions.Add(FTF2AimMatrixDirection{ TEXT("down_right"), 0, FVector(-45, -45, 0) });
		Directions.Add(FTF2AimMatrixDirection{ TEXT("down"), 1, FVector(0, -45, 0) });
		Directions.Add(FTF2AimMatrixDirection{ TEXT("down_left"), 2, FVector(45, -45, 0) });
		Directions.Add(FTF2AimMatrixDirection{ TEXT("left"), 3, FVector(-45, 0, 0) });
		Directions.Add(FTF2AimMatrixDirection{ TEXT("center"), 4, FVector(0, 0, 0) });
		Directions.Add(FTF2AimMatrixDirection{ TEXT("right"), 5, FVector(45, 0, 0) });
		Directions.Add(FTF2AimMatrixDirection{ TEXT("up_right"), 6, FVector(-45, 45, 0) });
		Directions.Add(FTF2AimMatrixDirection{ TEXT("up"), 7, FVector(0, 45, 0) });
		Directions.Add(FTF2AimMatrixDirection{ TEXT("up_left"), 8, FVector(45, 45, 0) });
		Directions.Add(FTF2AimMatrixDirection{ TEXT("above"), 9, FVector(0, 90, 0) });

		FString BaseAssetName = SelectedAsset.AssetName.ToString();
		if (!BaseAssetName.Contains("aimmatrix")) {
			auto Message = FText::Format(LOCTEXT("TF2GenAimMatrixSkipNotAimMatrix", "\"{AssetName}\" is not contains \"aimmatrix\".\n Skipping..."), FText::FromString(BaseAssetName));
			FMessageDialog::Open(EAppMsgType::Ok, Message);
			continue;
		}
		UAnimSequence* BaseAsset = Cast<UAnimSequence>(SelectedAsset.GetAsset());
		if (BaseAsset == nullptr) {
			auto Message = FText::Format(LOCTEXT("TF2GenAimMatrixSkipNotAnimSeq", "\"{AssetName}\" is not contains Anim Sequence.\n Skipping..."), FText::FromString(BaseAssetName));
			FMessageDialog::Open(EAppMsgType::Ok, Message);
			continue;
		}
		auto AdditiveAnimType = BaseAsset->GetAdditiveAnimType();
		if (AdditiveAnimType != EAdditiveAnimationType::AAT_RotationOffsetMeshSpace) {
			auto Message = FText::Format(LOCTEXT("TF2GenAimMatrixSkipNotMeshSpace", "\"{AssetName}\"'s Additive Anim Type is not Mesh Space.\n Skipping..."), FText::FromString(BaseAssetName));
			FMessageDialog::Open(EAppMsgType::Ok, Message);
			continue;
		}
		auto AdditiveBasePose = BaseAsset->GetAdditiveBasePose();
		if (!AdditiveBasePose) {
			auto Message = FText::Format(LOCTEXT("TF2GenAimMatrixSkipNoAddBasePose", "\"{AssetName}\" is no set Additive Base Pose.\n Skipping..."), FText::FromString(BaseAssetName));
			FMessageDialog::Open(EAppMsgType::Ok, Message);
			continue;
		}

		for (auto & Direction : Directions) {
			auto PartAssetPath = FString::Format(TEXT("{0}_{1}"), { BaseAsset->GetPathName(), Direction.Name.ToString() });
			PartAssetPath = PartAssetPath.Replace(TEXT("."), *FString::Format(TEXT("_{0}."), { Direction.Name.ToString() }));
			UE_LOG(LogTemp, Log, TEXT("%s"), *PartAssetPath);
			UEditorAssetLibrary::DuplicateLoadedAsset(BaseAsset, PartAssetPath);
			UAnimSequence* PartAsset = Cast<UAnimSequence>(UEditorAssetLibrary::LoadAsset(PartAssetPath));
			Direction.Asset = PartAsset;
			auto Frames = (float)PartAsset->GetRawNumberOfFrames();
			auto Length = PartAsset->SequenceLength;
			auto AliveFrame = Direction.AliveFrame;

			PartAsset->CropRawAnimData(((AliveFrame + 1) / Frames) * Length, false);
			if (AliveFrame == 0) continue;
			PartAsset->CropRawAnimData(((AliveFrame + 0.5) / Frames) * Length, true);
		}
		UPackage* CollectionPackage = BaseAsset->GetOutermost();
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		FString PackageName;
		FString AssetName;
		AssetToolsModule.Get().CreateUniqueAssetName(CollectionPackage->GetName(), TEXT("_AimOffset"), PackageName, AssetName);
		auto AimOffset = CastChecked<UAimOffsetBlendSpace>(AssetToolsModule.Get().CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), UAimOffsetBlendSpace::StaticClass(), nullptr));
		
		AimOffset->SetSkeleton(BaseAsset->GetSkeleton());

		AimOffset->Modify();

		UAimOffsetBlendSpace::StaticClass();
		auto Property = AimOffset->GetClass()->FindPropertyByName(TEXT("BlendParameters"));
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property)) {
			auto YawBlendParameter = StructProperty->ContainerPtrToValuePtr<FBlendParameter>(AimOffset, 0);
			YawBlendParameter->DisplayName = TEXT("Yaw");
			YawBlendParameter->Min = -45.0;
			YawBlendParameter->Max = 45.0;
			YawBlendParameter->GridNum = 2;
			auto PitchBlendParameter = StructProperty->ContainerPtrToValuePtr<FBlendParameter>(AimOffset, 1);
			PitchBlendParameter->DisplayName = TEXT("Pitch");
			PitchBlendParameter->Min = -45.0;
			PitchBlendParameter->Max = 90.0;
			PitchBlendParameter->GridNum = 3;
		}


		for (auto Direction : Directions) {
			AimOffset->AddSample(Direction.Asset, Direction.SampleValue);
		}

		AimOffset->PreviewBasePose = AdditiveBasePose;
		AimOffset->MarkPackageDirty();
		/*
		FPropertyChangedEvent EmptyPropertyChangedEvent(nullptr);
		FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(AimOffset, EmptyPropertyChangedEvent);
		*/
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTF2HelperPluginModule, TF2HelperPlugin)