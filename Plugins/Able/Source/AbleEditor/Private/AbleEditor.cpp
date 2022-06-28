// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbleEditorPrivate.h"

#include "AbilityEditor/ablAbilityEditorAddTaskHandlers.h"
#include "AbilityEditor/AblAbilityEditorSettings.h"
#include "AbilityEditor/AblAbilityTaskDetails.h"
#include "AbilityEditor/AblAbilityThumbnailRenderer.h"
#include "AbilityEditor/AssetTypeActions_ablAbilityBlueprint.h"
#include "ablAbility.h"
#include "ablAbilityBlueprint.h"
#include "AbleEditorEventManager.h"
#include "AbleStyle.h"
#include "ablSettings.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Editor/UnrealEd/Classes/ThumbnailRendering/ThumbnailManager.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Misc/SlowTask.h"
#include "Tasks/ablCustomTask.h"
#include "Editor.h"

#include "IAbleEditor.h"

DEFINE_LOG_CATEGORY(LogAbleEditor);

#define LOCTEXT_NAMESPACE "AbleEditor"

class FAbleEditor : public IAbleEditor
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual uint32 GetAbleAssetCategory() const override { return m_AbleAssetCategory; }
	void LoadAbilities();
	void OnObjectReplaced(const UEditorEngine::ReplacementObjectMap& Replacements);
	void OnAbilityEditorInstantiated(FAblAbilityEditor& AbilityEditor);
	void OnAbilityTaskCreated(FAblAbilityEditor& AbilityEditor, UAblAbilityTask& Task);
private:
	void RegisterAssetTypes(IAssetTools& AssetTools);
	void RegisterSettings();
	void UnregisterSettings();

	EAssetTypeCategories::Type m_AbleAssetCategory;

	TArray<TSharedPtr<IAssetTypeActions>> m_CreatedAssetTypeActions;

	TSharedPtr<FAblPlayAnimationAddedHandler> m_PlayAnimationTaskHandler;

	TArray<UAblAbility*> m_LoadedAbilities;
};

void FAbleEditor::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	m_AbleAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Able")), LOCTEXT("AbleAssetCategory", "Able"));

	FAbleStyle::Initialize();

	RegisterAssetTypes(AssetTools);
	RegisterSettings();

	UThumbnailManager::Get().RegisterCustomRenderer(UAblAbility::StaticClass(), UAblAbilityThumbnailRenderer::StaticClass());
	UThumbnailManager::Get().RegisterCustomRenderer(UAblAbilityBlueprint::StaticClass(), UAblAbilityThumbnailRenderer::StaticClass());

	m_PlayAnimationTaskHandler = MakeShareable(new FAblPlayAnimationAddedHandler());
	m_PlayAnimationTaskHandler->Register();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().OnFilesLoaded().AddRaw(this, &FAbleEditor::LoadAbilities);
	FAbleEditorEventManager::Get().OnAbilityEditorInstantiated().AddRaw(this, &FAbleEditor::OnAbilityEditorInstantiated);
	FAbleEditorEventManager::Get().OnAnyAbilityTaskCreated().AddRaw(this, &FAbleEditor::OnAbilityTaskCreated);
}


void FAbleEditor::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(UAblAbilityBlueprint::StaticClass());
		UThumbnailManager::Get().UnregisterCustomRenderer(UAblAbility::StaticClass());
	}


	FAbleStyle::Shutdown();

	UnregisterSettings();

	// Unregister any customized layout objects.
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout("AblAbilityTask");
	}

	// Unregister all the asset types that we registered
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (TSharedPtr<IAssetTypeActions>& Action : m_CreatedAssetTypeActions)
		{
			AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
		}
	}
	m_CreatedAssetTypeActions.Empty();
}

void FAbleEditor::LoadAbilities()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> allAbilities;
	AssetRegistryModule.Get().GetAssetsByClass(UAblAbilityBlueprint::StaticClass()->GetFName(), allAbilities);

	{
		FSlowTask LoadingAbilities(allAbilities.Num(), LOCTEXT("LoadingAbilitiesSlowTask", "Loading Abilities..."));

		for (const FAssetData& assetData : allAbilities)
		{
			UAblAbilityBlueprint* LoadedObject = CastChecked<UAblAbilityBlueprint>(assetData.GetAsset());
			if (LoadedObject->GeneratedClass)
			{
				UAblAbility* LoadedAbility = CastChecked<UAblAbility>(LoadedObject->GeneratedClass->GetDefaultObject());
				if (!LoadedAbility)
				{
					continue;
				}
				
				// Disabling this for now.
				//if (LoadedAbility->FixUpObjectFlags())
				//{
				//	LoadedAbility->Modify();
				//}

				if (LoadedAbility->GetTasks().FindItemByClass<UAblCustomTask>()) // Only watch Abilities with Custom tasks.
				{
					LoadedAbility->AddToRoot();
					m_LoadedAbilities.Add(LoadedAbility);
				}
			}

			LoadingAbilities.CompletedWork += 1.0f;
		}
	}

	if (GEditor)
	{
		GEditor->OnObjectsReplaced().AddRaw(this, &FAbleEditor::OnObjectReplaced);
	}
}

void FAbleEditor::OnObjectReplaced(const UEditorEngine::ReplacementObjectMap& Replacements)
{
	for (const TPair<UObject*, UObject*>& kvp : Replacements)
	{
		if (kvp.Key->IsA<UAblAbility>())
		{
			UAblAbility* NewAbility = CastChecked<UAblAbility>(kvp.Key);
			int index = 0;

			// Are we already watching this ability?
			if (m_LoadedAbilities.Find(NewAbility, index))
			{
				m_LoadedAbilities[index]->RemoveFromRoot();
				m_LoadedAbilities[index] = CastChecked<UAblAbility>(kvp.Value);
				m_LoadedAbilities[index]->AddToRoot();
			}
			else if (NewAbility->GetTasks().FindItemByClass<UAblCustomTask>()) // Should we be?
			{
				NewAbility = CastChecked<UAblAbility>(kvp.Value);
				NewAbility->AddToRoot();
				m_LoadedAbilities.Add(NewAbility);
			}
		}
		else if (kvp.Key->IsA<UBlueprintGeneratedClass>())
		{
			UBlueprintGeneratedClass* BPC = CastChecked<UBlueprintGeneratedClass>(kvp.Key);
			if (BPC->GetName().StartsWith(TEXT("REINST_")))
			{
				UBlueprintGeneratedClass* newBPC = Cast<UBlueprintGeneratedClass>(kvp.Value);
				if (newBPC &&
					newBPC->ClassDefaultObject &&
					newBPC->ClassDefaultObject->IsA<UAblCustomTask>())
				{
					UAblCustomTask* BP = CastChecked<UAblCustomTask>(newBPC->ClassDefaultObject);

					for (UAblAbility* Ability : m_LoadedAbilities)
					{
						TArray<UAblAbilityTask*>& allTasks = Ability->GetMutableTasks();
						for (int i = 0; i < allTasks.Num(); ++i)
						{
							if (!allTasks[i]->IsA<UAblCustomTask>())
							{
								continue;
							}

							UBlueprintGeneratedClass* TaskBGC = Cast<UBlueprintGeneratedClass>(allTasks[i]->GetClass());
							if (TaskBGC && TaskBGC->GetFName() == BPC->GetFName())
							{
								UAblAbilityTask* NewTask = NewObject<UAblAbilityTask>(Ability, BP->GetClass(), NAME_None, Ability->GetMaskedFlags(RF_PropagateToSubObjects) | RF_Transactional);

								NewTask->CopyProperties(*allTasks[i]);
								//NewTask->FixUpObjectFlags();

								allTasks[i] = NewTask;
								Ability->ValidateDependencies();
								Ability->MarkPackageDirty();
								Ability->EditorRefreshBroadcast();
							}
						}
					}
				}
			}
		}

	}
}

void FAbleEditor::OnAbilityEditorInstantiated(FAblAbilityEditor& AbilityEditor)
{
	// See if we need to watch this Ability.
	if (UAblAbility* Ability = AbilityEditor.GetAbility())
	{
		if (Ability->NeedsCompactData())
		{
			if (m_LoadedAbilities.Find(Ability) == INDEX_NONE)
			{
				m_LoadedAbilities.Add(Ability);
			}
		}
	}
}

void FAbleEditor::OnAbilityTaskCreated(FAblAbilityEditor& AbilityEditor, UAblAbilityTask& Task)
{
	// Do we need to watch this Task/Ability?
	if (Task.HasCompactData())
	{
		if (UAblAbility* Ability = AbilityEditor.GetAbility())
		{
			if (m_LoadedAbilities.Find(Ability) == INDEX_NONE)
			{
				m_LoadedAbilities.Add(Ability);
			}
		}
	}
}

void FAbleEditor::RegisterAssetTypes(IAssetTools& AssetTools)
{
	// Register any asset types
	
	// Ability Blueprint
	TSharedRef<IAssetTypeActions> AbilityBlueprint = MakeShareable(new FAssetTypeActions_AblAbilityBlueprint(m_AbleAssetCategory));
	AssetTools.RegisterAssetTypeActions(AbilityBlueprint);
	m_CreatedAssetTypeActions.Add(AbilityBlueprint);


}

void FAbleEditor::RegisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "Able",
			LOCTEXT("RuntimeSettingsName", "Able"),
			LOCTEXT("RuntimeSettingsDescription", "Configure the Able plugin"),
			GetMutableDefault<UAbleSettings>());

		SettingsModule->RegisterSettings("Editor", "ContentEditors", "AbilityEditor",
			LOCTEXT("AbilityEditorSettingsName", "Ability Editor"),
			LOCTEXT("AbilityEditorSettingsDescription", "Configure the Ability Editor"),
			GetMutableDefault<UAblAbilityEditorSettings>());
	}
}

void FAbleEditor::UnregisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Editor", "ContentEditors", "AbilityEditor");
		SettingsModule->UnregisterSettings("Project", "Plugins", "Able");
	}
}


IMPLEMENT_MODULE(FAbleEditor, AbleEditor)