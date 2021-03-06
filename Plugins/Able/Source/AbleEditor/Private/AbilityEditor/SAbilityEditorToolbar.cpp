// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityEditorToolbar.h"

#include "AbleEditorPrivate.h"
#include "AbilityEditor/ablAbilityEditor.h"
#include "AbilityEditor/ablAbilityEditorCommands.h"
#include "AbilityEditor/ablAbilityEditorModes.h"
#include "AbilityEditor/SAbilityGenericAssetShortcut.h"
#include "AbilityEditor/AblAbilityEditorSettings.h"

#include "GameFramework/Actor.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimBlueprint.h"
#include "Misc/MessageDialog.h"

#include "AbleStyle.h"

#include "WorkflowOrientedApp/SModeWidget.h"
#include "WorkflowOrientedApp/SContentReference.h"

#include "IDocumentation.h"

#include "ContentBrowserModule.h"
#include "EditorStyleSet.h"
#include "IContentBrowserSingleton.h"
#include "Framework/Application/SlateApplication.h"

#include "Engine/AssetManager.h"

#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "Textures/SlateIcon.h"
#include "Toolkits/AssetEditorManager.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"

#include "Styling/SlateTypes.h"

#include "Modules/ModuleManager.h"
#include "AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

class SModeSeparator : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SModeSeparator) {}
	SLATE_END_ARGS()

		void Construct(const FArguments& InArg)
	{
		SBorder::Construct(
			SBorder::FArguments()
			.BorderImage(FAbleStyle::GetBrush("AbilityEditor.ModeSeparator"))
			.Padding(0.0f)
		);
	}

	// SWidget interface
	virtual FVector2D ComputeDesiredSize(float) const override
	{
		const float Height = 24.0f;
		const float Thickness = 16.0f;
		return FVector2D(Thickness, Height);
	}
	// End of SWidget interface
};

void FAblAbilityEditorToolbar::SetupToolbar(TSharedPtr<FExtender> Extender, TSharedPtr<FAblAbilityEditor> InAbilityEditor)
{
	m_AbilityEditor = InAbilityEditor;

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		m_AbilityEditor.Pin()->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FAblAbilityEditorToolbar::FillAbilityEditorModeToolbar));

	
	m_PlayIcon = FSlateIcon(FAbleStyle::GetStyleSetName(), FName(TEXT("AblAbilityEditor.m_PlayAbility")));
	m_PauseIcon = FSlateIcon(FAbleStyle::GetStyleSetName(), FName(TEXT("AblAbilityEditor.m_PauseAbility")));

	m_PreviewThumbnailPool = MakeShareable(new FAssetThumbnailPool(16, false));
	m_TargetThumbnailPool = MakeShareable(new FAssetThumbnailPool(16, false));
}

bool FAblAbilityEditorToolbar::IsAssetValid(const FAssetData& Asset) const
{
	AActor* ActorAsset = Cast<AActor>(Asset.GetAsset());
	if (!ActorAsset)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset()))
		{
			if (Blueprint->GeneratedClass)
			{
				ActorAsset = Cast<AActor>(Blueprint->GeneratedClass->GetDefaultObject());
			}
		}
	}

	if (!ActorAsset)
	{
		return false;
	}

	return true;
}

void FAblAbilityEditorToolbar::FillAbilityEditorModeToolbar(FToolBarBuilder& ToolbarBuilder)
{
	TSharedPtr<FAblAbilityEditor> AbilityEditor = m_AbilityEditor.Pin();
	UBlueprint* BlueprintObj = AbilityEditor->GetBlueprintObj();

	const float ContentRefWidth = 80.0f;

	FToolBarBuilder Builder(ToolbarBuilder.GetTopCommandList(), ToolbarBuilder.GetCustomization());

	{
		TAttribute<FName> GetActiveMode(AbilityEditor.ToSharedRef(), &FAblAbilityEditor::GetCurrentMode);
		FOnModeChangeRequested SetActiveMode = FOnModeChangeRequested::CreateSP(AbilityEditor.ToSharedRef(), &FAblAbilityEditor::SetCurrentMode);

		// Left side padding
		AbilityEditor->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));

		AbilityEditor->AddToolbarWidget(
			SNew(SModeWidget, FAblAbilityEditorModes::GetLocalizedMode(FAblAbilityEditorModes::AbilityTimelineMode), FAblAbilityEditorModes::AbilityTimelineMode)
			.OnGetActiveMode(GetActiveMode)
			.OnSetActiveMode(SetActiveMode)
			.IconImage(FAbleStyle::GetBrush("AblAbilityEditor.TimelineMode"))
			.SmallIconImage(FAbleStyle::GetBrush("AblAbilityEditor.TimelineMode.Small"))
			.DirtyMarkerBrush(AbilityEditor.Get(), &FAblAbilityEditor::GetDirtyImageForMode, FAblAbilityEditorModes::AbilityTimelineMode)
			.ToolTip(IDocumentation::Get()->CreateToolTip(
				LOCTEXT("AbilityTimelineButtonTooltip", "Switch to timeline editing mode"),
				NULL,
				TEXT("Shared/Plugin/Able"),
				TEXT("TimelineMode")))
			.AddMetaData<FTagMetaData>(TEXT("Able.Timeline"))
			.ShortContents()
			[
				SNew(SContentReference)
				.AssetReference(AbilityEditor.ToSharedRef(), &FAblAbilityEditor::GetAbilityBlueprintAsObject)
			.AllowSelectingNewAsset(false)
			.AllowClearingReference(false)
			.WidthOverride(ContentRefWidth)
			]
		);

		AbilityEditor->AddToolbarWidget(SNew(SModeSeparator));

		AbilityEditor->AddToolbarWidget(
			SNew(SModeWidget, FAblAbilityEditorModes::GetLocalizedMode(FAblAbilityEditorModes::AbilityBlueprintMode), FAblAbilityEditorModes::AbilityBlueprintMode)
			.OnGetActiveMode(GetActiveMode)
			.OnSetActiveMode(SetActiveMode)
			.IconImage(FAbleStyle::GetBrush("AblAbilityEditor.GraphMode"))
			.SmallIconImage(FAbleStyle::GetBrush("AblAbilityEditor.GraphMode.Small"))
			.DirtyMarkerBrush(AbilityEditor.Get(), &FAblAbilityEditor::GetDirtyImageForMode, FAblAbilityEditorModes::AbilityBlueprintMode)
			.ToolTip(IDocumentation::Get()->CreateToolTip(
				LOCTEXT("AbilityBlueprintButtonTooltip", "Switch to blueprint editing mode"),
				NULL,
				TEXT("Shared/Plugin/Able"),
				TEXT("BlueprintMode")))
			.AddMetaData<FTagMetaData>(TEXT("Able.Blueprint"))
			.ShortContents()
			[
				SNew(SContentReference)
				.AssetReference(AbilityEditor.ToSharedRef(), &FAblAbilityEditor::GetAbilityBlueprintAsObject)
			.AllowSelectingNewAsset(false)
			.AllowClearingReference(false)
			.WidthOverride(ContentRefWidth)
			]
		);

		// Right side padding
		AbilityEditor->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));
	}
}

void FAblAbilityEditorToolbar::AddTimelineToolbar(TSharedPtr<FExtender> Extender, TSharedPtr<FAblAbilityEditor> InAbilityEditor)
{
	m_AbilityEditor = InAbilityEditor;

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		m_AbilityEditor.Pin()->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FAblAbilityEditorToolbar::FillTimelineModeToolbar));
}

void FAblAbilityEditorToolbar::OnPreviewAssetSelected(const FAssetData& Asset)
{
	if (!IsAssetValid(Asset))
	{
		FMessageDialog::Open(EAppMsgType::Ok, EAppReturnType::Ok, LOCTEXT("InvalidAssetErrorMessage", "The Asset choosen isn't an Actor. Please choose an appropriate actor."));
	}

	m_AbilityEditor.Pin()->SetPreviewAsset(Asset);
}

void FAblAbilityEditorToolbar::OnTargetAssetSelected(const FAssetData& Asset)
{
	if (!IsAssetValid(Asset))
	{
		FMessageDialog::Open(EAppMsgType::Ok, EAppReturnType::Ok, LOCTEXT("InvalidAssetErrorMessage", "The Asset choosen isn't an Actor. Please choose an appropriate actor."));
	}

	m_AbilityEditor.Pin()->SetTargetAsset(Asset);
}

TSharedRef<SWidget> FAblAbilityEditorToolbar::OnAddPreviewAssetWidgets()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "Toolbar.Button")
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked(this, &FAblAbilityEditorToolbar::OnResetPreviewAsset)
			.ToolTipText(LOCTEXT("PreviewResetTooltip", "Reset the Preview actor to it's last position before an Ability was executed."))
			[
				SNew(SImage).Image(FAbleStyle::Get()->GetBrush("AblAbilityEditor.ResetButton"))
			]
		];
}

TSharedRef<SWidget> FAblAbilityEditorToolbar::OnAddTargetAssetWidgets()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "Toolbar.Button")
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked(this, &FAblAbilityEditorToolbar::OnResetTargetAsset)
			.ToolTipText(LOCTEXT("TargetResetTooltip", "Reset the Target actor(s) to their last position before an Ability was executed."))
			[
				SNew(SImage).Image(FAbleStyle::Get()->GetBrush("AblAbilityEditor.ResetButton"))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "Toolbar.Button")
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked(this, &FAblAbilityEditorToolbar::OnAddTargetAsset)
			.ToolTipText(LOCTEXT("TargetAddTooltip", "Add a Target actor to the scene."))
			[
				SNew(SImage).Image(FAbleStyle::Get()->GetBrush("AblAbilityEditor.AddButton"))
			]
		];
}

TSharedRef<SWidget> FAblAbilityEditorToolbar::GenerateForceTargetComboBox()
{
	check(m_AbilityEditor.IsValid());
	const TArray<TWeakObjectPtr<AActor>>& allTargets = m_AbilityEditor.Pin()->GetAbilityPreviewTargets();

	FMenuBuilder TargetMenu(true, nullptr);
	TargetMenu.BeginSection(NAME_None, LOCTEXT("TargetMenuLabel", "Force Target"));

	TArray<FText> entryLabels;
	TArray<FText> entryTooltips;

	entryLabels.Add(LOCTEXT("TargetSelectNoneLabel", "None"));
	entryTooltips.Add(LOCTEXT("TargetSelectNoneTooltip", "Does not override the Ability Target. (Default)"));

	FText genericToolTip = LOCTEXT("TargetSelectooltip", "Forces the Ability to set {0} as the Target.");
	for (const TWeakObjectPtr<AActor>& Target : allTargets)
	{
		if (Target.IsValid())
		{
			FText targetName = FText::FromString(Target->GetName());
			entryLabels.Add(targetName);
			entryTooltips.Add(FText::FormatOrdered(genericToolTip, targetName));
		}
	}

	for (int i = 0; i < entryLabels.Num(); ++i)
	{
		TargetMenu.AddMenuEntry(entryLabels[i], entryTooltips[i], FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &FAblAbilityEditorToolbar::SetForceTarget, i)));
	}

	return TargetMenu.MakeWidget();
}

void FAblAbilityEditorToolbar::SetForceTarget(int index)
{
	if (m_AbilityEditor.IsValid())
	{
		if (index == 0)
		{
			m_AbilityEditor.Pin()->ClearForceTarget();
		}
		else
		{
			m_AbilityEditor.Pin()->SetForceTarget(index - 1);
		}
	}
}

FReply FAblAbilityEditorToolbar::OnResetPreviewAsset()
{
	if (m_AbilityEditor.IsValid())
	{
		m_AbilityEditor.Pin()->ResetPreviewActor();

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply FAblAbilityEditorToolbar::OnResetTargetAsset()
{
	if (m_AbilityEditor.IsValid())
	{
		m_AbilityEditor.Pin()->ResetTargetActors();

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply FAblAbilityEditorToolbar::OnAddTargetAsset()
{
	if (m_AbilityEditor.IsValid())
	{
		m_AbilityEditor.Pin()->AddAbilityPreviewTarget();

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void FAblAbilityEditorToolbar::FillTimelineModeToolbar(FToolBarBuilder& ToolbarBuilder)
{
		const FAblAbilityEditorCommands& Commands = FAblAbilityEditorCommands::Get();
		
		ToolbarBuilder.BeginSection("Thumbnail");
		ToolbarBuilder.AddToolBarButton(Commands.m_CaptureThumbnail);
		ToolbarBuilder.EndSection();

		ToolbarBuilder.BeginSection("Timeline");
		ToolbarBuilder.AddToolBarButton(Commands.m_AddTask);
		ToolbarBuilder.AddToolBarButton(Commands.m_StepAbilityBackwards);
		ToolbarBuilder.AddToolBarButton(Commands.m_PlayAbility,
			NAME_None,
			TAttribute<FText>(this, &FAblAbilityEditorToolbar::GetPlayText),
			TAttribute<FText>(this, &FAblAbilityEditorToolbar::GetPlayToolTip),
			TAttribute<FSlateIcon>(this, &FAblAbilityEditorToolbar::GetPlayIcon));
		ToolbarBuilder.AddToolBarButton(Commands.m_StepAbility);
		ToolbarBuilder.AddToolBarButton(Commands.m_StopAbility);
		ToolbarBuilder.AddToolBarButton(Commands.m_Resize);
		//ToolbarBuilder.AddToolBarButton(Commands.m_SetPreviewAsset);
		//ToolbarBuilder.AddToolBarButton(Commands.m_ResetPreviewAsset);
		ToolbarBuilder.AddToolBarButton(Commands.m_Validate);
		ToolbarBuilder.AddToolBarButton(Commands.m_ToggleCost);

		TWeakPtr<FAblAbilityEditor> WeakAbilityEditor = m_AbilityEditor.Pin();
		UAssetManager& AssetManager = UAssetManager::Get();
		UAblAbilityEditorSettings& EditorSettings = WeakAbilityEditor.Pin()->GetEditorSettings();

		// Preview Asset Generic Asset Widget
		TArray<FName> ClassFilter;
		EditorSettings.m_PreviewAllowedClasses.RemoveAll([](UClass* lhs) { return lhs == nullptr; });
		for (UClass* PreviewClass : EditorSettings.m_PreviewAllowedClasses)
		{
			ClassFilter.Add(PreviewClass->GetFName());
		}
		ClassFilter.Add(UBlueprint::StaticClass()->GetFName());

		if (ClassFilter.Num() == 0)
		{
			FMessageDialog::Open(EAppMsgType::Ok, EAppReturnType::Ok, LOCTEXT("NoClassErrorMessage", "Your Class list is empty for this actor. You will not be able to find any assets in the drop down.\nAdd the class you are looking for to the Ability Editor Settings and restart the Ability Editor."));
		}

		FAssetData PreviewAssetData;
		AssetManager.GetAssetDataForPath(EditorSettings.m_PreviewAsset, PreviewAssetData);
		SAblGenericAssetShortcut::FOnAddAdditionalWidgets PreviewCallback;
		PreviewCallback.BindSP(this, &FAblAbilityEditorToolbar::OnAddPreviewAssetWidgets);
		FText PreviewAssetLabel = LOCTEXT("PreviewAssetButton", "Preview Asset");
		TSharedRef<SAblGenericAssetShortcut> PreviewAsset = SNew(SAblGenericAssetShortcut, PreviewAssetData, m_PreviewThumbnailPool.ToSharedRef(), PreviewAssetLabel, PreviewCallback);
		PreviewAsset->SetHostingApp(WeakAbilityEditor);
		PreviewAsset->SetFilterData(ClassFilter);
		PreviewAsset->SetClassFilter(EditorSettings.m_PreviewAllowedClasses);
		PreviewAsset->OnAssetSelectedCallback().BindSP(this, &FAblAbilityEditorToolbar::OnPreviewAssetSelected);
		ToolbarBuilder.AddWidget(PreviewAsset);

		// Target Asset Generic Asset Widget
		FAssetData TargetAssetData;
		ClassFilter.Empty();
		EditorSettings.m_TargetAllowedClasses.RemoveAll([](UClass* lhs) { return lhs == nullptr; });
		for (UClass* TargetClass : EditorSettings.m_TargetAllowedClasses)
		{
			ClassFilter.Add(TargetClass->GetFName());
		}
		ClassFilter.Add(UBlueprint::StaticClass()->GetFName());

		if (ClassFilter.Num() == 0)
		{
			FMessageDialog::Open(EAppMsgType::Ok, EAppReturnType::Ok, LOCTEXT("NoClassErrorMessage", "Your Class list is empty for this actor. You will not be able to find any assets in the drop down.\nAdd the class you are looking for to the Ability Editor Settings and restart the Ability Editor."));
		}

		AssetManager.GetAssetDataForPath(EditorSettings.m_TargetAsset, TargetAssetData);
		SAblGenericAssetShortcut::FOnAddAdditionalWidgets TargetCallback;
		TargetCallback.BindSP(this, &FAblAbilityEditorToolbar::OnAddTargetAssetWidgets);
		FText TargetAssetLabel = LOCTEXT("TargetAssetButton", "Target Asset");
		TSharedRef<SAblGenericAssetShortcut> TargetAsset = SNew(SAblGenericAssetShortcut, TargetAssetData, m_TargetThumbnailPool.ToSharedRef(), TargetAssetLabel, TargetCallback);
		TargetAsset->SetHostingApp(WeakAbilityEditor);
		TargetAsset->SetFilterData(ClassFilter);
		TargetAsset->SetClassFilter(EditorSettings.m_TargetAllowedClasses);
		TargetAsset->OnAssetSelectedCallback().BindSP(this, &FAblAbilityEditorToolbar::OnTargetAssetSelected);
		ToolbarBuilder.AddWidget(TargetAsset);
		ToolbarBuilder.AddComboButton(FUIAction(),
			FOnGetContent::CreateSP(this, &FAblAbilityEditorToolbar::GenerateForceTargetComboBox),
			LOCTEXT("ForceTargetLabel", "Override Target"),
			LOCTEXT("ForceTargetTooltip", "Forces the Ability to set the provided actor as the target."));
		ToolbarBuilder.EndSection();

}

FSlateIcon FAblAbilityEditorToolbar::GetPlayIcon() const
{
	if (m_AbilityEditor.Pin()->IsPlayingAbility() && !m_AbilityEditor.Pin()->IsPaused())
	{
		return m_PauseIcon;
	}

	return m_PlayIcon;
}

FText FAblAbilityEditorToolbar::GetPlayText() const
{
	if (m_AbilityEditor.Pin()->IsPlayingAbility() && !m_AbilityEditor.Pin()->IsPaused())
	{
		return LOCTEXT("PauseCommand", "Pause");
	}

	return LOCTEXT("PlayCommand", "Play");
}

FText FAblAbilityEditorToolbar::GetPlayToolTip() const
{
	if (m_AbilityEditor.Pin()->IsPlayingAbility() && !m_AbilityEditor.Pin()->IsPaused())
	{
		return LOCTEXT("PauseCommandToolTip", "Pause the currently playing Ability.");
	}

	return LOCTEXT("PlayCommandToolTip", "Play the current Ability.");
}

#undef LOCTEXT_NAMESPACE