# HazardTools

An Unreal Engine editor plugin with tools for inspecting objects, browsing Slate styles, editing package flags, and reopening recently used assets. The tools are available through dedicated editor tabs, dialogs, and toolbar controls.

## Features

- **Object Outliner** — Inspect live `UObject` instances in memory, with filtering, search, sorting, and property inspection.
- **Style Browser** — Browse registered Slate styles, preview their appearance, and copy style names or C++ snippets.
- **Packages Flags** — Edit `EPackageFlags` for assets selected in the Content Browser.
- **Recent Assets Button** — Quickly reopen recently used assets, levels, and Level Sequences from the editor toolbar.

## Accessing the Tools

- **Hazard Tools** in the main menu (registered under `LevelEditor.MainMenu`):
  - **Packages Flags** — Opens the package flags dialog for the selected assets.
  - **Object Outliner** — Opens the Object Outliner tab.
  - **Style Browser** — Opens the Style Browser tab.
- **Recent Assets** in the level editor toolbar, next to Play (under `LevelEditor.LevelEditorToolBar.PlayToolBar`).

## Object Outliner

An editor tab that lists `UObject` instances found by `FThreadSafeObjectIterator`.

Features:
- **View modes:** Flat list or tree grouped by `Outer`.
- **Search:** Filter by object or class name.
- **Filters:**
  - Toggle button: `CDO` (Class Default Objects).
  - Dropdown filters: `Package`, `Fields`, `Meta`, `Transient`, `Materials`, `Textures`, `Niagara`, `Subsystem`, and `Actors` (enabled by default).
  - **Show only checked:** Show only objects matching the enabled filters.
- **Sorting:** Sort by `Name`, `Class`, or `Memory`.
- **Details panel:** Inspect the selected object's properties, including hidden `UPROPERTY` fields.
- **Context menu:**
  - *Open Source File* — Navigate to the object's class source code.
  - *Copy Object Path* — Copy the object path to the clipboard.
- **Double-click:** Open the asset editor when supported by the selected object.

Filter settings, view mode, and sort order are saved to the configuration file.

## Style Browser

An editor tab for inspecting Slate styles registered with `FSlateStyleRegistry`.

Features:
- Select a **Style Set** from the registered style sets.
- Enable **Append from parent style sets** to include styles inherited from parent style sets.
- Search by style name or type.
- Filter by type: `Float`, `Vector2D`, `Color`, `SlateColor`, `Margin`, `Brush`, `WidgetStyle`, `FontStyle`, or `Sound`.
- Inspect styles in the right-hand panel:
  - **Style name:** Click to copy the name.
  - **Code snippet:** Click to copy the corresponding `Get*` or `GetWidgetStyle` call.
  - **Preview:** Visual preview, including widgets for `WidgetStyle` entries.
  - **Details:** Text representation of the `UStruct`, with clickable paths where available.

Additional functionality:
- `WidgetStyle` previews support common widget styles, including `Button`, `CheckBox`, `ComboBox`, `Slider`, `ProgressBar`, and `ToolBar`. Styles without a supported preview are identified as such.
- `FontAwesome` fonts have a glyph table; clicking a glyph copies its name.

## Packages Flags

A dialog for editing `EPackageFlags` on assets selected in the Content Browser.

- Scans the selected assets and determines the state of each flag.
- Displays **Checked**, **Unchecked**, or **Mixed** for each flag.
- Supports enabling or disabling flags across multiple assets.
- Marks a package as dirty when its flags change.

## Recent Assets Button

A combo button in the editor toolbar with three lists:

- **Recent Assets** — Recently opened assets, excluding levels and Level Sequences.
- **Recent Levels** — Recently opened levels from the MRU list.
- **Recent Level Sequences** — Recently opened Level Sequences.

The button is hidden during PIE (Play In Editor).

## Font Glyph Browser

Open **Hazard Tools > Font Glyph Browser** to inspect a runtime **Font** or **Font Face** asset. For a composite Font, select a face from the default, sub-font, or fallback typefaces. The browser displays the selected face's Unicode character map independently of composite ranges and system fallback fonts. Offline bitmap fonts are not supported.

- Browse a virtualized glyph grid and resize the selected glyph's preview.
- Search by partial name, alias, hexadecimal code (`E5C4`, `U+E5C4`, `0xE5C4`, `\uE5C4`), decimal code, or the glyph itself. Searches ignore case; spaces combine terms with AND. OR, NOT, and quoted phrases are also supported.
- Double-click a glyph to copy `U+XXXX`, or use the preview buttons to copy a Unicode code, hexadecimal literal, C++ Unicode escape, or the glyph itself. Supplementary characters use `\UXXXXXXXX` escapes.
- **Material Symbols names** uses the bundled Google codepoint mapping, including aliases. It is enabled automatically for Material Symbols fonts and can be toggled for renamed or unrelated fonts. Other fonts use their embedded glyph names when available. Only codepoints actually present in the selected face are shown.
- Use **Refresh** after reimporting or editing a font.

The last selected asset and face are saved in the current user's `EditorPerProjectUserSettings.ini`, under `[HazardTools.FontGlyphBrowser]` (`LastFont`, `LastFace`). No project asset path is hard-coded. The browser uses only engine modules and the engine's FreeType library, with no dependency on HazardUX or other project plugins. It works without a network connection; source, license, and update instructions for the bundled names are in `Resources/FontGlyphBrowser/SOURCE.md`.

## Settings and Configuration

The other tools use `UDeveloperSettings` and the plugin's configuration file:

- Configuration file: `Project/Config/DefaultHazardTools.ini`
- Settings classes:
  - `UHazardToolsObjectOutlinerSettings`
  - `UHazardToolsStyleBrowserSettings`

Active filters, view modes, and selected categories are persisted.

## Module and Limitations

- `HazardTools` is an **Editor** module.
- The plugin contains no content (`CanContainContent = false`) and is not intended for runtime use.

## Primary Asset Types Manager

**Primary Asset Types** is an editor-only Slate tool for managing
`UAssetManagerSettings::PrimaryAssetTypesToScan` in a table-based interface.

Open it from:

```text
Tools -> Hazard Tools -> Primary Asset Types
```

The tool opens in a Nomad tab and belongs to the `HazardTools` editor module. It
has no effect on runtime modules or packaged builds.

### Main Features

- Displays all `PrimaryAssetTypesToScan` entries in configuration order.
- Shows a compact table with columns for index, primary asset type, asset base
  class, Blueprint class scanning, editor-only status, scan path counts, cook
  rules, and validation status.
- Supports adding, duplicating, deleting, and reordering entries.
- Uses Unreal Engine's standard Details panel to edit the selected
  `FPrimaryAssetTypeInfo`, including nested `FPrimaryAssetRules`.
- Provides a toolbar search field that filters by type name, base class, rules,
  validation messages, directories, and specific asset paths.
- Highlights matching text in the table.
- Shows the full entry summary in each row's tooltip.
- Saves changes through `UAssetManagerSettings` and Unreal Engine's configuration APIs.
- Provides a shortcut to the Asset Manager page in Project Settings for comparison.

### Validation

Each entry is validated, with the result shown in its table row. Checks include
empty or duplicate `PrimaryAssetType` values, missing `AssetBaseClass` values,
missing scan paths for standard asset types, invalid paths, directories shared
by multiple types, potentially incorrect Blueprint class scanning settings,
and editor-only entries with cook rules that imply cooking.

`GameFeatureData` entries may omit explicit directories and specific assets in
`PrimaryAssetTypesToScan`, since Game Feature assets are commonly discovered
through the Game Features system rather than those scan paths.

### Save and Revert

Changes remain in an in-memory working copy until **Save** is clicked.
**Revert** reloads the current values from `UAssetManagerSettings`.

The tool does not edit `DefaultEngine.ini` as raw text.

## UMG to Slate

**Hazard Tools → UMG to Slate** exports a Widget Blueprint's Designer hierarchy
to a standalone Slate C++ header. The Widget Blueprint Editor also provides an
**Export Slate** toolbar button. The code view supports syntax highlighting,
copying, and a **Write all values (including defaults)** option. Compact output
is enabled by default. Asset references are included only as comments.

See [conversion coverage and usage](Source/HazardTools/Private/UMGToSlate/README.md).
