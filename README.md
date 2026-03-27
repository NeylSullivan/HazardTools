# HazardTools

An Unreal Engine editor plugin that provides a set of utilities for developers. It adds dedicated windows, dialogs, and UI elements for working with objects, Slate styles, and quick access to recently opened assets.

## Overview
- **Object Outliner** — an inspector for all live `UObject` instances in memory, with filtering, search, sorting, and property inspection.
- **Style Browser** — a Slate style browser with preview, highlighting, and copying of ready-to-use snippets.
- **Packages Flags** — a dialog for editing `EPackageFlags` on selected assets.
- **Recent Assets Button** — a button in the editor toolbar for quickly opening recently used assets/levels/sequences.

## Where to find it in the editor
- Main menu: **Hazard Tools** (the menu is added to `LevelEditor.MainMenu`).
  - **Packages Flags** — opens the package flags dialog for the selected assets.
  - **Object Outliner** — opens the tool tab.
  - **Style Browser** — opens the tool tab.
- Level toolbar: **Recent Assets** button next to Play (in `LevelEditor.LevelEditorToolBar.PlayToolBar`).

## Object Outliner
A tab with a table of all `UObject` instances found through `FThreadSafeObjectIterator`.

Key features:
- **Display modes**: list or tree (by `Outer`).
- **Search** by object name and class name.
- **Filters**:
  - Imperative (buttons): `CDO` (Class Default Objects).
  - Dropdown filters: `Package`, `Fields`, `Meta`, `Transient`, `Materials`, `Textures`, `Niagara`, `Subsystem`, `Actors` (enabled by default).
  - **Show only checked** option — displays only objects that match the enabled filters.
- **Sorting** by the `Name`, `Class`, and `Memory` columns.
- **Details panel**: displays details of the selected object on the right (including hidden `UPROPERTY` fields).
- **Context menu**:
  - *Open Source File* — navigate to the class source files.
  - *Copy Object Path* — copy the object path.
- **Double-click** on a row opens the asset editor (if applicable).

The state of filters, display mode, and sorting is saved in the config.

## Style Browser
A tab for browsing Slate styles registered in `FSlateStyleRegistry`.

Features:
- Select a **Style Set** from the list of all registered sets.
- **Append from parent style sets** option — extends the list with styles inherited from parent sets in the hierarchy.
- Search by name and type.
- Filtering by type: `Float`, `Vector2D`, `Color`, `SlateColor`, `Margin`, `Brush`, `WidgetStyle`, `FontStyle`, `Sound`.
- Right panel:
  - **Style name** (click copies the name to the clipboard).
  - **Code snippet** (click copies a ready-to-use `Get*` / `GetWidgetStyle` call).
  - **Preview** — visual preview (including widgets for `WidgetStyle`).
  - **Details** — textual breakdown of the `UStruct` (with clickable paths, if available).

Notes:
- For `WidgetStyle`, there are visualizers for common styles (`Button`, `CheckBox`, `ComboBox`, `Slider`, `ProgressBar`, `ToolBar`, etc.). If a visualizer is not available, this is shown explicitly.
- For `FontAwesome` fonts, a glyph table is displayed; clicking a glyph copies its name.

## Packages Flags
A dialog for managing `EPackageFlags` on assets selected in the Content Browser.

What it does:
- Scans the selected assets and determines the state of each flag.
- Displays **Checked / Unchecked / Mixed**.
- Allows flags to be enabled or disabled in bulk.
- Marks the package as dirty when a flag is changed.

## Recent Assets Button
A combo button in the editor toolbar:
- **Recent Assets** — recently opened assets (excluding levels and Level Sequences).
- **Recent Levels** — recent levels (from MRU).
- **Recent Level Sequences** — recent Level Sequences.

The button is hidden during PIE (Play In Editor / Play World mode).

## Settings and config
The plugin uses `UDeveloperSettings` and its own config files:
- Config: `Project/Config/DefaultHazardTools.ini`
- Settings classes:
  - `UHazardToolsObjectOutlinerSettings`
  - `UHazardToolsStyleBrowserSettings`

It stores active filters, display modes, and selected categories.

## Module and limitations
- The `HazardTools` module is of type **Editor**.
- The plugin does not contain content (`CanContainContent = false`) and is not intended for runtime use.
