# Scaffold helper for dialogue BindWidgets on WBP_RTSGameHUD.
#
# UE4.27 Python cannot edit a WidgetBlueprint's widget_tree, so this script:
#   1) Checks the asset exists
#   2) Opens it
#   3) Prints exact Designer steps / widget names
#
# Plugins (Edit → Plugins), then restart:
#   - Python Editor Script Plugin
#   - Editor Scripting Utilities
#
# Output Log → Cmd:
#   py "D:/UE/UEproject/BigDogBarkBarkBark/Tools/CreateRTSDialogueWidgets.py"

import unreal

HUD_PATH = "/Game/UI/WBP_RTSGameHUD"


def _has_eal():
    return hasattr(unreal, "EditorAssetLibrary")


def _asset_exists(path):
    if _has_eal():
        return unreal.EditorAssetLibrary.does_asset_exist(path)
    return unreal.load_asset(path) is not None


def _load_asset(path):
    if _has_eal():
        return unreal.EditorAssetLibrary.load_asset(path)
    asset = unreal.load_asset(path)
    if asset is None:
        short = path.rsplit("/", 1)[-1]
        asset = unreal.load_asset("{}.{}".format(path, short))
    return asset


def _open_asset(asset):
    if asset is None:
        return
    if _has_eal() and hasattr(unreal.EditorAssetLibrary, "open_editor_for_assets"):
        unreal.EditorAssetLibrary.open_editor_for_assets([asset])


def _print_manual_steps():
    lines = [
        "",
        "========== Manual dialogue setup (UE4.27) ==========",
        "Open: Content/UI/WBP_RTSGameHUD  (Designer tab)",
        "Root should be Canvas Panel.",
        "",
        "Hierarchy to create (names must match EXACTLY):",
        "",
        "  CanvasPanel (root)",
        "  └─ Border                 name: DialoguePanel     (anchors: full screen)",
        "     └─ CanvasPanel         name: DialogueInner",
        "        ├─ Button           name: BtnDialogueAdvance (anchors: full screen, almost transparent)",
        "        ├─ Border           name: PortraitBlock     (bottom-center, ~240x340)",
        "        │  └─ Image         name: DialoguePortrait  (optional art)",
        "        └─ CanvasPanel      name: DialogueBottomChrome (bottom strip)",
        "           ├─ Border        name: DialogueBox",
        "           │  └─ VerticalBox name: DialogueBoxCol",
        "           │     ├─ TextBlock name: DialogueBodyText",
        "           │     └─ TextBlock name: ContinueHint   (optional)",
        "           └─ Border        name: NamePlate",
        "              └─ TextBlock  name: DialogueNameText",
        "",
        "Required names for C++ BindWidget:",
        "  DialoguePanel, BtnDialogueAdvance, DialogueNameText, DialogueBodyText",
        "",
        "Tips:",
        "  - Temporarily set DialoguePanel Visible while editing, then Collapsed + Compile + Save.",
        "  - If DialoguePanel exists, C++ will NOT build the runtime fallback dialogue UI.",
        "====================================================",
        "",
    ]
    for line in lines:
        unreal.log(line)


def main():
    if not _has_eal():
        unreal.log_warning(
            "Enable plugin: Edit → Plugins → 'Editor Scripting Utilities' (+ Python), restart editor."
        )

    if not _asset_exists(HUD_PATH):
        unreal.log_error(
            "{} missing. In Content Browser: right-click Content/UI → User Interface → "
            "Widget Blueprint, parent class RTSGameHUD, name WBP_RTSGameHUD.".format(HUD_PATH)
        )
        _print_manual_steps()
        return

    widget_bp = _load_asset(HUD_PATH)
    if widget_bp is None:
        unreal.log_error("Failed to load {}".format(HUD_PATH))
        _print_manual_steps()
        return

    _open_asset(widget_bp)
    unreal.log("Opened {}. UE4.27 cannot auto-build UMG trees from Python — follow steps below.".format(HUD_PATH))
    _print_manual_steps()


if __name__ == "__main__":
    main()
