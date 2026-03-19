# STYLE.md - Victorian-born Digital (Qt Quick / QML Styling Guide)

## Purpose

This document defines a **"Victorian-born digital"** styling system for vcstream's authored UI. It is written for AI agents and human developers applying styling and behaviour primarily through **Qt Quick / QML** shared components, theme tokens, and small amounts of C++ support where required.

The aim is **not** "aged antique" or parody. The target look is **newly made, carefully composed, and richly presented** - as if Victorian design culture had developed native digital interfaces.

## Scope and non-goals

### In scope
- Styling authored controls (buttons, links, inputs, cards, tabs, nav, overlays, notices)
- Spatial arrangement of elements (grouping, hierarchy, framing, sequencing)
- Motion and interactive behaviours (hover, focus, expand, reveal)
- Theme variants and design tokens
- Progressive enhancement via shared QML wrapper components and theme objects

### Not in scope
- Rebuilding the application information architecture from scratch
- Heavy layout redesign mandates when the existing structure is already sound
- Fake ageing/distress effects (rust, stains, torn parchment, grime)
- Steampunk cliches (gears everywhere, brass UI props)

---

## Design premise (historical rationale translated into UI rules)

The visual system should synthesise these Victorian design ideas into digital styling:

1. **Ornament is disciplined by structure**
   Use ornament to **decorate construction**, not replace it. In practice: build a clear grid/panel system first, then apply borders, corner details, dividers, and patterned bands.

2. **Pattern and nature are stylised, not literal**
   Use abstracted floral/foliate/geometric motifs (linework, repeat patterns, borders), not photoreal leaves, faux materials, or scenic wallpaper textures.

3. **Beauty serves utility, and utility is visible**
   Controls must remain legible, predictable, accessible, and keyboard-usable. Decorative treatment should make hierarchy clearer, not harder to use.

4. **The interface is a presented object**
   A screen should feel composed like a title page, periodical spread, cabinet display, or shopfront: framed regions, visible hierarchy, section transitions, distinct content classes.

5. **Colour and hierarchy are expressive**
   Victorian-influenced design can be richly coloured, but palette choices should be coherent and controlled by a small token set.

### Short historical anchors (optional quotes to guide decisions)

- Owen Jones: **"decorate construction, never to construct decoration"**. Translate this into UI as: structure first, ornament second.
- Christopher Dresser: **"Utility must precede beauty"**. Translate this into UI as: usable controls first, then refinement.

---

## Aesthetic target in one sentence

**Architectural layout + stylised ornament + expressive typography + restrained ceremonial motion, all in service of clear modern usability.**

---

## Core styling principles (implementation rules)

## 1) Start from a structural grid, not decoration

Before adding any ornamental styling, establish:
- A consistent **content width** and **column grid**
- Reusable **panel containers**
- Spacing scale (tight / normal / ceremonial)
- Heading hierarchy
- Section separators

### QML expectations
- Use shared theme tokens for spacing, borders, radii, and colours.
- Prefer `RowLayout`, `ColumnLayout`, `GridLayout`, and deliberate panel structure over ad-hoc anchors and margins.
- Put structural dimensions in the shared theme layer rather than scattering literals through pages.

### Visual result
The interface should already feel orderly in monochrome wireframe mode. Ornament then amplifies hierarchy.

---

## 2) Treat sections as rooms, panels, and compartments

Victorian-born digital interfaces should read as a sequence of **framed compartments**, not a flat stream of floating content.

### Apply this to existing elements
If the UI contains generic blocks, style them into one of these presentation classes:
- **Panel** - standard content container
- **Notice panel** - alerts, updates, summaries
- **Feature panel** - hero or spotlight area
- **Index panel** - lists, menus, navigation groups
- **Reading panel** - article text, documentation, explanatory copy

### Arrangement guidance
- Group related controls into panels with a visible boundary.
- Prefer 2-3 levels of nesting max.
- Use divider bands / rule lines between sections instead of excessive empty voids.
- Avoid "everything boxed" at equal weight: vary emphasis through border weight, fill, and title treatment.

---

## 3) Ornament must attach to seams and edges

Ornament is most convincing when it appears where construction logically happens:
- Corners
- Header bands
- Footer bands
- Section dividers
- Panel title plaques
- Active tab edges
- Focus rings (subtle decorative echo)

### Do this
- Use layered borders, corner markers, accent rails, and divider components.
- Use decorative SVGs or generated motifs as repeatable separators where appropriate.
- Confine pattern fills to headers, side rails, or hero bands.

### Do not do this
- Scatter decorative motifs in empty areas with no structural purpose.
- Place heavy ornament behind body text.
- Use full-screen busy wallpaper under interactive controls.

---

## 4) Use stylised motifs, not faux materials

### Preferred motif families
- Foliate scrolls, ivy/acanthus/laurel linework
- Rosettes and medallions
- Geometric repeats (trellis, diaper, lattice, star, strapwork)
- Heraldic/shield/crest framing (for logos or emblems)
- Printer's ornaments (fleurets, rule ends, cartouche shapes)

### Surface treatment
Use:
- Flat colour fields
- Fine linework
- Subtle hatch/stipple overlays
- Clean vector borders
- Optional enamel/lacquer-like colour blocks

Avoid:
- Rust textures
- Torn parchment edges
- Fake dust/grime
- Heavy leather/brass skeuomorphism

---

## 5) Typography carries hierarchy (without sacrificing readability)

Victorian-flavoured interfaces are often won or lost on typography.

### Functional typographic roles
Define and style these roles:
- **Masthead / title** (display)
- **Section heading** (display-lite)
- **Panel heading** (labelled plaque)
- **Body** (high legibility)
- **Meta** (small caps / letterspaced labels)
- **Notice / badge** (compact label)

### Styling behaviour
- Make headings visibly composed (rules, plaque, ornament caps, banner treatment).
- Keep body text calm and readable (line length, line-height, contrast).
- Use decorative styles only for display roles, never for dense form labels or body paragraphs.
- Use small caps / letterspaced labels for metadata and UI categories.

### Layout rhythm
- Pair ornate headings with simple body areas.
- Keep vertical rhythm consistent via spacing tokens.
- Use drop caps only in long-form reading panels, never in forms or dashboards.

---

## 6) Use colour as a controlled system (not random richness)

Victorian design can be vivid, but digital implementation needs a disciplined token system.

### Palette design rule
Choose **one** principal family and **one** accent family. Do not mix all possible Victorian colours at once.

### Recommended theme variants (pick one as default)

#### A. Scholarly Gothic (serious / archival)
- Base: warm ivory / stone
- Ink: near-black
- Primary accent: oxblood or deep navy
- Secondary accent: restrained gold or brass-like flat ochre
- Use case: articles, archives, institutional pages

#### B. Aesthetic Peacock (artistic / refined)
- Base: cream
- Ink: deep brown-black
- Primary accent: peacock blue-green / teal
- Secondary accent: dusky rose or olive
- Use case: portfolios, cultural sites, community landing pages

#### C. Arts-and-Crafts Workshop (human / crafted)
- Base: warm ivory
- Ink: charcoal
- Primary accent: indigo or leaf green
- Secondary accent: madder red / ochre
- Use case: community, writing, studio software

#### D. High Victorian Commercial (bold / promotional)
- Base: pale cream
- Ink: black
- Primary accent: claret / bottle green / ultramarine (choose one)
- Secondary accent: gold highlight
- Use case: announcements, events, storefront-like pages

### Colour usage constraints
- Body text contrast must meet modern accessibility standards.
- Reserve gold/bright accent for headings, active states, ornaments, or key CTAs.
- Use muted fills behind dense UI areas; keep high-chroma colour for bands, plaques, emphasis.
- Avoid simultaneous use of multiple saturated hues at equal prominence.

### Accent roles
The theme system should expose semantic accent tiers:
- **Primary accent** - user-selected accent family or custom accent
- **Secondary accent** - companion accent for framing, rails, and supporting emphasis
- **Tertiary accent** - quieter accent for ornament, insets, and active-state support

User preferences should adjust these tokens coherently rather than allowing arbitrary per-widget colour edits.

---

## 7) Images and icons should look printed, engraved, or emblematic

### Preferred image treatment
- Framed images with margins/padding like plates
- Line illustrations, silhouettes, emblem marks, crests
- Monochrome or limited-palette illustrations integrated with theme colours
- Pattern-backed hero graphics

### UI icons
Prefer icons that feel:
- outline-based
- heraldic / engraved in character
- consistent stroke width
- geometrically disciplined

Do not mix:
- thin modern outline set + chunky pseudo-vintage pictograms + photoreal badges

### Image framing patterns
- Plate frame (simple line + outer rule)
- Gilt frame analogue (flat accent line, no realistic metallic gradients)
- Oval/cameo mask for portraits/logomarks (use sparingly)

---

## 8) Motion should be ceremonial, not playful

If animation is used, it should feel deliberate and composed.

### Preferred motion language
- Panel reveal / fade-up
- Accordion unfold (cabinet-drawer feel)
- Tab underline/plaque shift
- Ornament line drawing or rule extension
- Section divider appearing on state change or reveal

### Avoid
- Bouncy spring physics for all interactions
- Wobble/jelly effects
- Excessive parallax
- Decorative animation that impedes reading/input

### Accessibility requirement
Respect the user's reduce-motion preference. Motion must degrade to instant state changes.

---

## Applying the style to existing controls (QML emphasis)

## Buttons and CTAs

### Visual treatment
Style buttons as **plaques** or **framed labels**, not glossy pills.

#### Primary button
- Rectangular or gently chamfered shape
- Outer border + inner rule
- Solid fill in primary accent
- High-contrast label
- Optional tiny corner marks or rule caps

#### Secondary button
- Transparent or pale fill
- Strong border and title-case/small-caps label
- Accent on hover via border/fill inversion

#### Text button / link-button
- Underline or rule-based emphasis, not heavy background pill
- Optional ornamental prefix/suffix on hover (small and consistent)

### States
- **Hover**: subtle fill shift, inner rule highlight, ornament appears
- **Active**: slight inset shadow/border compression
- **Focus-visible**: strong modern focus ring first, decorative echo second
- **Disabled**: reduced contrast and ornament removal (do not rely on colour only)

---

## Links

Links are a major part of the style. Treat them as typographic and editorial elements.

### Styling rules
- Maintain clear underline behaviour in body text.
- For navigation/metadata links, use rule separators and hover banners rather than vague colour changes.
- Distinguish visited links in long-form reading contexts.
- Use small ornaments only for special links, not every link.

---

## Forms (inputs, selects, textareas, checkboxes, radios)

Focus on making form controls feel integrated into the panel system.

### Text inputs / textareas / selects
- Framed field with clear border and inner padding
- Optional inset background tint (very subtle)
- Label above field (or left in wide layouts), with small-caps/meta styling
- Validation/help text styled as a note line beneath

### Focus treatment
- Strong focus ring with accessible contrast
- Optional secondary ornamental corner markers on focus
- Do not animate focus in ways that delay input feedback

### Checkboxes / radios
- Prefer clean geometric boxes/circles with crisp strokes
- Add Victorian feel via label typography and grouping panels, not over-designed controls
- Use accent colour for checked state; keep mark shape simple

### Field grouping
Wrap logically related controls visually with:
- legend/plaque heading
- thin divider rules
- consistent spacing

---

## Tabs, segmented controls, and accordions

These components naturally suit a Victorian compartment aesthetic.

### Tabs
- Render as labelled plaques or book-tabs
- Active tab appears integrated with content panel (shared border seam)
- Inactive tabs lighter and simpler
- Use underline/rule extension to connect active tab to panel

### Accordions
- Header row styled as notice or catalogue entry
- Chevron/icon may be replaced or supplemented by rule/corner markers
- Expansion motion should be measured and brief

### Segmented controls
- Treat as a mini panel with internal dividers
- Active segment gets plaque fill or border emphasis

---

## Cards, tiles, and list items

Cards are the easiest way to achieve the visual language without structural overhauls.

### Card anatomy
- Header band / title strip
- Content body
- Meta/footer line
- Optional ornament anchor (one corner, one footer mark, or divider cap)

### Styling guidance
- Vary weight by role (standard / featured / notice)
- Keep internal spacing consistent
- Avoid stacking many equally ornate cards in a dense grid (fatigue)
- In lists, use repeated separators and subtle alternating fills instead of heavy borders on every item

---

## Navigation bars, sidebars, and menus

Treat navigation as **architectural moulding**, not floating text.

### Top navigation
- Framed bar with inner rule
- Logo/crest/monogram anchored left
- Visible separators between top-level items
- Active item highlighted by plaque, underline rule, or banner band

### Sidebar navigation
- Group items into labelled sections
- Use nested indentation sparingly
- Show current page strongly via border/fill, not only bold text

### Dropdowns / popovers
- Panel styling with border + subtle shadow
- Section labels and separators
- Optional ornament in header strip only

---

## Modals, dialogs, toasts, and notices

### Modal/dialog styling
- Treat as a formal notice panel or cabinet drawer
- Strong title band and clear close affordance
- Comfortable padding and visible button hierarchy
- Decorative accents limited to title band / corners

### Toasts/notifications
- Small framed notice slips
- Colour-coded border accent for severity/state
- Minimal ornament (tiny rule cap or icon)

### Error and warning states
Keep modern clarity first:
- icon + label + message
- strong contrast
- accessible colour pairings
- avoid making warnings ornate to the point of ambiguity

---

## Page-level arrangement patterns

## Pattern 1: Framed processional page
Use for homepages and feature pages.
- Hero/title panel (largest framing)
- Intro/notice band
- Main content panels in sequence
- Side rail or inset panels for notices/metadata
- Formal footer with separators

## Pattern 2: Reading room page
Use for docs and explanatory views.
- Calm reading panel with clear line length
- Decorative treatment concentrated at header, section breaks, and footnotes/meta
- Sidebar/TOC panel optional
- Distinguish editorial notes from main text via bordered callouts

## Pattern 3: Catalogue/dashboard page
Use for settings, collections, resources.
- Structured header panel
- Filter/search panel
- Card/table grid with role-based emphasis
- Repeated separators and labels
- Ornaments only at panel/title level

---

## Theme token system

Use a token-first approach so authored components can be styled consistently.

The shared QML theme object should expose at least:

- typography roles
- spacing scale
- border widths and radii
- surface colours
- ink and muted text colours
- primary/secondary/tertiary accent roles
- focus colour
- success / warning / danger colours
- panel and popup shadows
- motion durations and easing

User preferences should drive token changes through coherent theme options such as:
- mode (`system`, `light`, `dark`)
- accent family or custom accent
- reduced motion
- future density and style-family settings

---

## Component implementation strategy

Use shared QML wrappers to classify and decorate authored UI:
- `VcPanel`
- `VcButton`
- `VcToolButton`
- `VcTextField`
- `VcComboBox`
- `VcCheckBox`
- additional wrappers as needed

Rules:
- prefer adding one shared wrapper rather than restyling many pages independently
- do not break existing semantics or interaction flow for decoration
- do not move focus unexpectedly during restyling
- do not reorder content purely for ornament

---

## Accessibility and usability constraints (hard requirements)

These override aesthetics when in conflict.

1. **Keyboard navigation remains clear**
   - Strong `focus-visible` styling
   - No focus traps introduced by decorative wrappers

2. **Contrast remains modern and legible**
   - Body text on surfaces must meet WCAG guidance
   - Decorative linework must not substitute for text contrast

3. **Readable content density**
   - Avoid cramped Victorian "print nostalgia" for forms and UI controls
   - Use generous padding on touch targets

4. **Decorative fonts are optional and limited**
   - Highly readable body typography is acceptable
   - Decorative display styling can be achieved via borders, case, spacing, and layout even without period fonts

5. **Reduced motion support**
   - All animations optional
   - No essential information hidden behind animation timing

---

## Do / Don't checklist for the AI agent

### Do
- Build a clean structural layout first
- Use framed panels and visible hierarchy
- Apply ornament at seams, edges, and title areas
- Keep body text and forms highly usable
- Choose one palette family and stick to it
- Make interactions feel deliberate and polished
- Treat cards, lists, tabs, and overlays as designed compartments

### Don't
- Fake age with stains, tears, rust, or grime
- Scatter decorative motifs randomly
- Over-pattern large backgrounds behind text
- Use ornate type for body copy or dense UI labels
- Make every component equally decorative
- Sacrifice focus visibility or contrast for mood
- Turn the interface into steampunk theatre props

---

## Quick implementation plan (for an AI styling agent)

1. **Audit existing UI roles**
   - Identify panels, headings, actions, forms, nav, overlays, lists

2. **Install token layer**
   - Shared QML theme object + preference-driven variants

3. **Apply structural wrappers**
   - `VcPanel`, `VcButton`, `VcToolButton`, and follow-on wrappers

4. **Style states and accessibility**
   - hover, focus-visible, active, disabled, validation states

5. **Add controlled ornament**
   - corners, header bands, dividers, rails (not everywhere)

6. **Add ceremonial motion (optional)**
   - panel reveals, accordion/tab transitions, rule animations

7. **Tune density and hierarchy**
   - reduce ornament in dense screens; increase in hero, landing, and reading surfaces

---

## Success criteria (what "Victorian-born digital" should feel like)

A user should feel that the interface is:
- **crafted** (not generic)
- **structured** (not chaotic)
- **ornamented with discipline** (not cluttered)
- **expressive in colour and typography** (not flat)
- **fully modern in usability** (not cosplay)

If the interface still works well when all ornament is removed, and becomes more ceremonial and distinctive when ornament is restored, the styling system is working correctly.
