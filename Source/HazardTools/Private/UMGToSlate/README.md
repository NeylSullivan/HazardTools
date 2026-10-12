# UMG to Slate

Open **Hazard Tools → UMG to Slate**, select a Widget Blueprint and click
**Generate Slate**. The **Export Slate** button in the Widget Blueprint Editor
opens the same tool with that Blueprint selected. Regenerate after designer edits;
the exporter reads the current in-memory templates, including unsaved changes.

By default, the exporter writes only values that differ from Slate defaults.
Enable **Write all values (including defaults)** to include them; changing the
checkbox immediately regenerates the code. Attribute defaults come from Slate
constructors, so differences between UMG and Slate remain explicit. Brush/style
fields are compared with their default constructors; font fields are compared
with the generated fallback font. Float literals use their shortest round-trip
representation, for example `0.0f`, `0.1f`, `24.0f`.

The result is a complete header containing an `SGenerated_<BlueprintName>` class,
required includes, `Construct`, the declarative widget hierarchy and owned style
fields. **Copy C++** copies the entire header; the read-only code view also supports
selection and normal copy shortcuts. C++ keywords, types, numbers, strings and
comments have syntax highlighting. The consuming module needs `Core`, `Slate`
and `SlateCore`, with no UMG or HazardTools runtime dependency.

## Conversion

- Panels: Canvas, Horizontal/Vertical Box, Overlay, Grid, Uniform Grid, Wrap Box,
  Scroll Box and Widget Switcher. Slot anchors, offsets, alignment, padding,
  auto/fill sizing, z-order, grid spans/fill and wrap settings are exported.
- Layout: Size Box, Scale Box, Border, Safe Zone, Spacer, Background Blur,
  Named Slot and Invalidation Box.
- Controls: Text Block, Image, Button, Check Box, Slider, Progress Bar,
  Editable Text and Editable Text Box.
- Common appearance: visibility, enabled state, opacity, clipping, pixel snapping,
  flow direction, render transform and pivot. Text includes font size, outline,
  spacing, color, shadow, wrapping and justification. Brushes and widget styles
  are stored as members so Slate never receives pointers to temporary styles.
- Inherited designer roots and named slot overrides are traversed. Nested
  User Widgets use their designer templates, including named slot content;
  recursive references and excessive hierarchy sizes are bounded.

## Boundaries

This exports a static designer snapshot. Blueprint graphs, bindings, animations,
PreConstruct, dynamic list entries, custom painting, input handlers and
localization identities are not converted. Text is exported as its current
display string. Theme colors are resolved using the current editor theme.

Asset references, including brush textures/materials, fonts and sounds, appear
only in comments. No asset loading code is generated. Brushes retain their
non-resource design properties, and fonts use the built-in Regular fallback.
Assign the commented resources manually in the consuming code to reproduce their
appearance. Use the same host size and DPI scale as the original design.

Unknown widgets become marked `SOverlay` placeholders that preserve any panel
children. Native subclasses use their supported base widget with a conversion
note. Retainer Box exports its content without the render target/effect material.
Conversion notes appear both under the code view and in the generated header.
