// Copyright Epic Games, Inc. All Rights Reserved.

#include "TF2HelperPlugin.h"
#include "EditorAssetLibrary.h"
#include "Templates/SharedPointer.h"
#include "Animation/AimOffsetBlendSpace.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Sound/SoundWave.h"

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

	if (SelectedAssets.Num() == 1)
	{
		if (SelectedAssets[0].AssetClass == UAnimSequence::StaticClass()->GetFName())
		{
			if (SelectedAssets[0].AssetName.ToString().Contains("Aimmatrix", ESearchCase::CaseSensitive)) {
				Extender->AddMenuExtension(
					TEXT("CommonAssetActions"),
					EExtensionHook::First,
					nullptr,
					FMenuExtensionDelegate::CreateStatic(&FTF2HelperPluginModule::CreateGenerateAimMatrixAssetMenu, SelectedAssets)
				);
			}
		}
	}

	Extender->AddMenuExtension(
		TEXT("CommonAssetActions"),
		EExtensionHook::First,
		nullptr,
		FMenuExtensionDelegate::CreateStatic(&FTF2HelperPluginModule::CreateAutoRenameAssetMenu, SelectedAssets)
	);

	return Extender;
}



void FTF2HelperPluginModule::CreateGenerateAimMatrixAssetMenu(FMenuBuilder& MenuBuilder, TArray<FAssetData>SelectedAssets)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("GenerateAimMatrix", "Generate AimMatrix"),
		LOCTEXT("GenerateAimMatrix_Tooltip", "Generate AimMatrix"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FTF2HelperPluginModule::GenerateAimMatrix, SelectedAssets)),
		NAME_None,
		EUserInterfaceActionType::Button
	);
}

void FTF2HelperPluginModule::CreateAutoRenameAssetMenu(FMenuBuilder& MenuBuilder, TArray<FAssetData>SelectedAssets)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("AutoRename", "Auto Rename"),
		LOCTEXT("AutoRename_Tooltip", "Auto Rename"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FTF2HelperPluginModule::AutoRename, SelectedAssets)),
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
		UAnimSequence* BaseAsset = Cast<UAnimSequence>(SelectedAsset.GetAsset());
		if (BaseAsset == nullptr) {
			auto Message = FText::Format(LOCTEXT("TF2GenAimMatrixSkipNotAnimSeq", "\"{AssetName}\" is not Anim Sequence.\n Skipping..."), FText::FromString(BaseAssetName));
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

		AimOffset->PreviewBasePose = AdditiveBasePose;

		for (auto Direction : Directions) {
			AimOffset->AddSample(Direction.Asset, Direction.SampleValue);
		}
	}
}

void FTF2HelperPluginModule::AutoRename(TArray<FAssetData> SelectedAssets)
{
	for (auto Asset : SelectedAssets) {
		auto Name = Asset.AssetName.ToString();
		auto Class = Asset.AssetClass.ToString();

		// Remove suffix
		{
			auto RemovingSuffix = TArray<FString>();
			RemovingSuffix.Add(TEXT("_PhysicsAsset"));
			RemovingSuffix.Add(TEXT("_Skeleton"));
			for (auto Suffix : RemovingSuffix) {
				if (Name.EndsWith(Suffix)) {
					Name = Name.LeftChop(Suffix.Len());
				}
			}
		}

		// snake_case to UpperCamelCase
		{
			FString NewName;
			NewName.AppendChar(FChar::ToUpper(Name[0]));
			NewName.ToUpperInline();
			for (int32 i = 1; i < Name.Len(); i++) {
				if (Name.RightChop(i).Left(1) != TEXT("_")) {
					NewName.AppendChar(FChar::ToLower(Name[i]));
					continue;
				}
				i++;
				NewName.AppendChar(FChar::ToUpper(Name[i]));
			}
			Name = NewName;
		}

		// Rename!
		{
			if (Class == USkeletalMesh::StaticClass()->GetName()) {
				Name = TEXT("SK_") + Name;
				UEditorAssetLibrary::RenameLoadedAsset(Asset.GetAsset(), FPaths::Combine(Asset.PackagePath.ToString(), Name));
			}
			else if (Class == UStaticMesh::StaticClass()->GetName()) {
				Name = TEXT("SM_") + Name;
				UEditorAssetLibrary::RenameLoadedAsset(Asset.GetAsset(), FPaths::Combine(Asset.PackagePath.ToString(), Name));
			}
			else if (Class == UTexture::StaticClass()->GetName()) {
				Name = TEXT("T_") + Name;
				UEditorAssetLibrary::RenameLoadedAsset(Asset.GetAsset(), FPaths::Combine(Asset.PackagePath.ToString(), Name));
			}
			else if (Class == UAnimSequence::StaticClass()->GetName()) {
				if (Name.Contains(TEXT("AnimsRoot"))) {
					Name = Name.Replace(TEXT("AnimsRoot"), TEXT(""));
				}
				if (Name.Contains(TEXT("AnimationsRoot"))) {
					Name = Name.Replace(TEXT("AnimationsRoot"), TEXT(""));
				}
				Name = TEXT("A_") + Name;
				UEditorAssetLibrary::RenameLoadedAsset(Asset.GetAsset(), FPaths::Combine(Asset.PackagePath.ToString(), Name));
			}
			else if(Class == USkeleton::StaticClass()->GetName())
			{
				Name = TEXT("SK_") + Name + TEXT("_Skeleton");
				UEditorAssetLibrary::RenameLoadedAsset(Asset.GetAsset(), FPaths::Combine(Asset.PackagePath.ToString(), Name));
			}
			else if (Class == UPhysicsAsset::StaticClass()->GetName()) {
				Name = TEXT("SK_") + Name + TEXT("_PhysicsAsset");
				UEditorAssetLibrary::RenameLoadedAsset(Asset.GetAsset(), FPaths::Combine(Asset.PackagePath.ToString(), Name));
			}
			else if (Class == USoundWave::StaticClass()->GetName()) {
				Name = TEXT("S_") + Name;
				UEditorAssetLibrary::RenameLoadedAsset(Asset.GetAsset(), FPaths::Combine(Asset.PackagePath.ToString(), Name));
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTF2HelperPluginModule, TF2HelperPlugin)