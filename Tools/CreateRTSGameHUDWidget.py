# Create WBP_RTSGameHUD (parent: RTSGameHUD).
#
# Plugins (Edit → Plugins), then restart editor:
#   - Python Editor Script Plugin
#   - Editor Scripting Utilities
#
# Output Log → Cmd:
#   py "D:/UE/UEproject/BigDogBarkBarkBark/Tools/CreateRTSGameHUDWidget.py"
# Then:
#   py "D:/UE/UEproject/BigDogBarkBarkBark/Tools/CreateRTSDialogueWidgets.py"

import unreal

ASSET_PATH = "/Game/UI"
ASSET_NAME = "WBP_RTSGameHUD"
FULL_PATH = ASSET_PATH + "/" + ASSET_NAME


def _has_eal():
    return hasattr(unreal, "EditorAssetLibrary")


def _exists(path):
    if _has_eal():
        return unreal.EditorAssetLibrary.does_asset_exist(path)
    return unreal.load_asset(path) is not None


def _save(path):
    if _has_eal():
        unreal.EditorAssetLibrary.save_asset(path)
    elif hasattr(unreal, "EditorLoadingAndSavingUtils"):
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    else:
        unreal.log_warning("Please Save All (Ctrl+Shift+S).")


if not _has_eal():
    unreal.log_warning(
        "EditorAssetLibrary missing. Enable: Edit → Plugins → 'Editor Scripting Utilities', restart, re-run."
    )

if _exists(FULL_PATH):
    unreal.log_warning("{} already exists.".format(FULL_PATH))
    unreal.log('Next: py "D:/UE/UEproject/BigDogBarkBarkBark/Tools/CreateRTSDialogueWidgets.py"')
else:
    if _has_eal() and not unreal.EditorAssetLibrary.does_directory_exist(ASSET_PATH):
        unreal.EditorAssetLibrary.make_directory(ASSET_PATH)

    factory = unreal.WidgetBlueprintFactory()
    parent = unreal.load_class(None, "/Script/BigDogBarkBarkBark.RTSGameHUD")
    if parent is None:
        unreal.log_error("Could not load RTSGameHUD C++ class. Compile the project first.")
    else:
        factory.set_editor_property("parent_class", parent)
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        asset = tools.create_asset(ASSET_NAME, ASSET_PATH, unreal.WidgetBlueprint, factory)
        if asset:
            _save(FULL_PATH)
            unreal.log("Created {}. Next run CreateRTSDialogueWidgets.py".format(FULL_PATH))
        else:
            unreal.log_error("Failed to create Widget Blueprint.")
