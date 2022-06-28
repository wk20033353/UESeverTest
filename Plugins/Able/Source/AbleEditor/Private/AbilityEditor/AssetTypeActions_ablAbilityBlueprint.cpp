// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AssetTypeActions_ablAbilityBlueprint.h"

#include "AbleEditorPrivate.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "BlueprintEditorModule.h"
#include "AssetRegistryModule.h"

#include "AbilityEditor/ablAbilityEditor.h"
#include "ablAbilityBlueprint.h"
#include "AbilityEditor/ablAbilityBlueprintFactory.h"

#include "Editor/UnrealEd/Classes/ThumbnailRendering/SceneThumbnailInfo.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FAssetTypeActions_AblAbilityBlueprint::FAssetTypeActions_AblAbilityBlueprint(EAssetTypeCategories::Type AssetCategory)
	: m_AssetCategory(AssetCategory)
{

}

FAssetTypeActions_AblAbilityBlueprint::~FAssetTypeActions_AblAbilityBlueprint()
{

}

FText FAssetTypeActions_AblAbilityBlueprint::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AblAbilityBlueprint", "Able Ability Blueprint");
}

FColor FAssetTypeActions_AblAbilityBlueprint::GetTypeColor() const
{
	return FColor::Emerald;
}

void FAssetTypeActions_AblAbilityBlueprint::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (UObject* Object : InObjects)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			if (Blueprint->SkeletonGeneratedClass && Blueprint->GeneratedClass)
			{
				// Do we really need a 1:1 relationship with Editor to Asset?
				TSharedRef<FAblAbilityEditor> Editor(new FAblAbilityEditor());
				TArray<UBlueprint*> Blueprints;
				Blueprints.Add(Blueprint);

				Editor->InitAbilityEditor(Mode, EditWithinLevelEditor, Blueprints, ShouldUseDataOnlyEditor(Blueprint));
			}
			else
			{
				// You can only get here if someone changed our Blueprint parent class to one that no longer exists.
				checkNoEntry();
			}
		}
	}

}

bool FAssetTypeActions_AblAbilityBlueprint::ShouldUseDataOnlyEditor(const UBlueprint* Blueprint) const
{
	return FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint)
		&& !FBlueprintEditorUtils::IsLevelScriptBlueprint(Blueprint)
		&& !FBlueprintEditorUtils::IsInterfaceBlueprint(Blueprint)
		&& !Blueprint->bForceFullEditor
		&& !Blueprint->bIsNewlyCreated;
}

UClass* FAssetTypeActions_AblAbilityBlueprint::GetSupportedClass() const
{
	return UAblAbilityBlueprint::StaticClass();
}

UThumbnailInfo* FAssetTypeActions_AblAbilityBlueprint::GetThumbnailInfo(UObject* Asset) const
{
	UAblAbilityBlueprint* AbilityBlueprint = CastChecked<UAblAbilityBlueprint>(Asset);
	UThumbnailInfo* ThumbnailInfo = AbilityBlueprint->ThumbnailInfo;
	if (ThumbnailInfo == NULL)
	{
		ThumbnailInfo = NewObject<USceneThumbnailInfo>(AbilityBlueprint);
		AbilityBlueprint->ThumbnailInfo = ThumbnailInfo;
	}

	return ThumbnailInfo;
}

UFactory* FAssetTypeActions_AblAbilityBlueprint::GetFactoryForBlueprintType(UBlueprint* InBlueprint) const
{
	UAblAbilityBlueprintFactory* AblAbilityBlueprintFactory = NewObject<UAblAbilityBlueprintFactory>();
	AblAbilityBlueprintFactory->ParentClass = TSubclassOf<UAblAbility>(*InBlueprint->GeneratedClass);
	return AblAbilityBlueprintFactory;
}

#undef LOCTEXT_NAMESPACE