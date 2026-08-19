# Creation Engineer Architecture

Creation Engineer is a suite-native engineering workstation built on the shared
Creation Suite platform.

## Architectural Position

Creation Engineer should sit between pure authoring and pure simulation:

- it authors engineering models
- it owns engineering parameters and constraints
- it runs or orchestrates technical analyses
- it visualizes results directly in the workstation

This makes it neither a simple viewer nor a detached compute tool. It is an
interactive engineering environment.

## High-Level Subsystems

The app should evolve around these major subsystems:

1. Engineering model workspace
2. OpenGL technical viewport
3. Parameter and constraint system
4. Simulation and analysis system
5. Plot/result inspection system
6. CEL programming and automation surface
7. Suite project/VFS integration

## Phase 1 Workstation Spine

The first implementation phase should not try to deliver all of mechanical,
electrical, and simulation depth at once.

It should deliver the first serious workstation spine that every later domain
feature will plug into.

Phase 1 should include:

1. technical viewport shell
2. engineering model navigator
3. parameter and property inspector
4. result/status surface
5. suite project/VFS awareness for Engineer assets

The goal of this phase is to make the app feel like an engineering workstation
immediately, even before deep solver or modeling features arrive.

### Phase 1 Technical Viewport

The first viewport pass should provide:

- a dedicated central technical viewing surface
- engineering-style camera presets and navigation hooks
- visible unit/grid intent
- room for overlays such as dimensions, guides, section markers, and selection
- a reload/refresh workflow rather than aggressive always-live regeneration

This first pass can begin as a structured shell, but it must be shaped like the
real future viewport rather than a placeholder text block.

### Phase 1 Engineering Navigator

The navigator should represent the engineering workspace as structured content,
not as a loose file list.

Initial responsibilities:

- active project and Engineer domain context
- model/assembly tree placeholder
- study list placeholder
- quick visibility into which engineering assets belong to the current project
- first scene-management actions for adding, duplicating, and removing
  engineering objects
- a dedicated modifier-stack surface for non-destructive object shaping

### Phase 1 Parameter And Property Surface

This panel should be the first home for:

- named design parameters
- object properties
- units and value intent
- future constraints and exposed dynamic inputs

Even before full solving exists, this establishes the shape of engineering
editing in the workstation.

### Phase 1 Results And Status Surface

The workstation needs an area reserved for:

- study state
- validation and warnings
- quick metrics
- future plot/result summaries

That keeps analysis close to authoring from the beginning.

### Phase 1 Asset Expectations

Engineer-owned project content should ultimately live inside the shared suite
project/VFS model under the Engineer domain.

The first phase should therefore assume asset classes such as:

- engineering workspace documents
- model and assembly definitions
- parameter sets
- study definitions
- result snapshots

The shell should reinforce that Engineer stores assets into the project rather
than inventing a separate project system.

## Engineering Model Workspace

This subsystem owns the editable technical representation of the design.

Responsibilities:

- model hierarchy and assembly structure
- parametric entities and dimensions
- constraints and references
- domain properties attached to engineering objects
- revision-safe persistence of technical state

### Parametric Primitive And Freeze Workflow

The engineering model workspace should support a deliberate progression from
parameterized primitives to editable geometry.

The first modeling layer should include built-in engineering primitives such as:

- blocks
- cylinders
- tubes
- plates
- shafts
- holes
- slots
- simple structural or bracket-like forms

Those primitives should begin life as parameter-driven objects with explicit
dimensions and engineering properties.

The architecture should then allow an intentional transition where a primitive
or derived shape is frozen into explicit geometry for direct editing.

That transition should preserve:

- object identity
- assembly membership
- parameter history where useful
- compatibility with selection, inspection, and analysis systems

After the transition, editing emphasis shifts from parameter-first authoring to
geometry-first authoring, including vertex or equivalent direct geometric
manipulation where appropriate.

This gives the workstation a practical middle path between strict CAD history
and unrestricted mesh editing.

The active workstation implementation should therefore track object authoring
state explicitly so the UI can distinguish:

- primitive-backed objects
- modifier-stack objects
- committed direct-geometry objects

### Modifier Stack Layer

Between raw parametric primitives and fully committed direct geometry, the
workspace should support a modifier stack layer.

This layer should allow ordered, non-destructive operations to be attached to
an engineering object and evaluated as part of its derived shape.

Architectural expectations:

- modifiers remain editable after creation
- modifiers can be enabled, disabled, and inspected individually
- modifier ordering is explicit
- objects can be committed from modifier-driven state into explicit geometry
- the same object identity continues through that transition

The early modifier family should leave room for operations such as:

- mirror
- array or pattern repetition
- boolean operations
- bevel/fillet/chamfer-like shaping
- shell/thickness
- other practical engineering shape transforms

This gives the model architecture three connected authoring layers:

1. primitive parameter layer
2. modifier stack layer
3. committed direct-geometry layer

That structure is a strong fit for Engineer because it preserves iteration
speed while still allowing eventual low-level geometric control.

The first concrete modifier implementation should be mirror because it is high
value, easy to inspect in both technical and assembly-floor views, and a clean
proof point for non-destructive object derivation before broader stack support
lands.

The next modifier-panel responsibility should be to expose stack management
directly in the workstation through add, enable or disable, reorder, and
remove actions so modifiers become a first-class engineering workflow surface.

### Early Implementation Order For Mechanical Authoring

The mechanical authoring stack should be built in a deliberate order so each
later layer sits on a stable foundation.

Recommended order:

1. shared engineering scene/model representation
2. technical viewport component
3. primitive parameter objects
4. viewport mode switching between technical and assembly-floor views
5. first modifier stack implementation
6. freeze/commit pipeline from procedural object state into explicit geometry
7. first direct-geometry editing tools

This order matters because direct-geometry editing should not arrive before the
object identity, viewport behavior, and primitive/modifier pipeline are clear.

### Direct Geometry Editing Layer

After commitment into explicit geometry, the workspace should provide a direct
geometry editing layer inspired by strong mesh-editing workflows, but adapted
for engineering precision.

The first workstation implementation should expose this through a dedicated
docked tool surface rather than leaving direct geometry as a passive state.

That early surface should cover:

- current direct-geometry tool selection
- translation nudges
- scaling adjustments
- extrusion depth control
- bevel amount control

The next direct-geometry layer should expose selectable engineering proxies for:

- vertices
- edges
- faces

That layer should not be dock-panel-only. The viewport itself should allow the
user to pick the active proxy directly and drag it as part of the direct
geometry workflow.

The next viewport step should expose visible gizmo handles so direct geometry
operations can be constrained along meaningful axes instead of relying only on
free dragging.

That gizmo layer should also expose precision controls for snapping and
step-size selection so direct geometry edits can land on predictable
engineering increments rather than only continuous mouse motion.

That lets the workstation begin element-level editing before a fuller mesh or
solid-editing system arrives.

The first direct-edit tool family should include:

- move/rotate/scale with numeric input and axis locks
- snapping modes suitable for engineering work
- extrude
- bevel/chamfer
- loop cut
- knife/cut
- merge
- dissolve

This layer should stay integrated with the broader engineering object model
instead of behaving like an isolated mesh editor.

## OpenGL Technical Viewport

This subsystem is the primary visual surface for engineered models and analysis
feedback.

Responsibilities:

- render technical geometry and engineering scenes
- provide precise inspection and navigation
- show overlays, guides, and analysis states
- reflect parameter changes and solver outputs quickly

This viewport architecture should support more than one presentation mode over
the same engineering workspace data.

### Mechanical View Families

For mechanical engineering, the workstation should support at least two
connected view families:

1. technical design view
2. immersive assembly-floor view

The technical design view is the standard CAD-style engineering surface:

- precise technical rendering
- orthographic and engineering camera behavior
- dimensions, guides, references, sectioning, and constraint overlays
- selection and editing of engineering model structure

This viewport should favor contextual interaction over persistent button banks.
Primary view commands should live in a right-click context menu so the working
surface remains clear for modeling while still exposing fast access to camera
presets, view-family switching, primitive insertion, and direct-geometry tool
selection.

The first navigation baseline should support:

- middle-mouse orbit
- shift plus middle-mouse pan
- mouse-wheel zoom
- camera reset from the context menu

Selection semantics should also be explicit:

- left click selects without immediately moving the part
- non-geometry objects expose a visible move handle for whole-object
  repositioning
- direct-geometry movement remains anchored to dedicated gizmos and element
  proxies

The immersive assembly-floor view is a rendered spatial mode using the same
assembly/model data:

- game-like walkaround or flyaround inspection
- rendered presentation of parts in assembled context
- evaluation of space claims, access, reach, and fit
- practical visual inspection of how a design occupies real space

These modes should not fork the model into separate representations.

They should share:

- the same engineering object graph
- the same assembly relationships
- the same parameter state
- the same visibility and selection concepts where practical

This makes the immersive mode an engineering inspection tool rather than a
separate visualization export pipeline.

## Parameter And Constraint System

This subsystem makes the workstation genuinely engineering-oriented instead of
just geometry-oriented.

Responsibilities:

- named parameters
- dependency relationships
- constraints
- value propagation
- design variation inputs

This system should also be scriptable through CEL.

## Simulation And Analysis System

This subsystem manages analyses without turning the app into one giant solver.

Responsibilities:

- define studies
- run domain-specific analyses
- store inputs and outputs
- support repeatable sweeps and comparisons
- allow additional solver domains over time

The architecture should favor pluggable analysis domains over one oversized,
hard-coded engine.

## Electrical Modeling Direction

Electrical engineering should be represented at more than one level.

The architecture should support:

- system-level electrical chain modeling
- circuit-level analysis
- future lower-level SPICE-like electronics modeling

System-level modeling should cover practical device chains such as:

- power source
- regulation
- control unit
- actuator or motor loads

That layer should make it possible to evaluate:

- battery life
- current flow
- power draw
- component ratings
- system operating margins

## Circuit-Level Analysis Direction

The analysis layer should also support practical circuit behavior such as:

- resistive analysis
- capacitive behavior
- inductive behavior
- AC response

This should integrate with the higher-level engineering model rather than live
as a disconnected specialist island.

## Mechanical And FEA Direction

One of the first serious domain targets should be practical mechanical
engineering.

That means the architecture should leave room for:

- part and assembly representations suitable for engineering constraints
- motion/clearance-aware mechanism study
- practical finite element analysis
- GPU-assisted analysis acceleration where useful

The intended scale is product, mechanism, enclosure, fixture, and robotics
engineering rather than massive aerospace-class simulation as an initial goal.

## Plot And Result Inspection

This subsystem presents engineering outputs in usable technical forms.

Responsibilities:

- plots
- tables
- scalar and vector result summaries
- comparisons between runs
- inspection of generated datasets

## CEL Integration

CEL should act as the programmable layer across the app.

Responsibilities:

- model-generation scripting
- parameter logic
- repeatable studies
- custom technical calculations
- derived metrics
- future automation agents

Creation Engineer should expose engineering-specific CEL domains rather than
forcing everything through generic suite-only scripting.

## Suite Platform Responsibilities

The shared suite platform remains responsible for:

- project/VFS storage
- shared project identity
- shared auth/session/profile behavior
- shared AI/provider routing
- shared infrastructure libraries

Creation Engineer should consume those systems, not reimplement them.

## Design Principle

The core architectural principle is:

"Bring the modeling, analysis, visualization, and programmable workflow close
together, but keep solver domains modular."

That preserves room for serious engineering capability without collapsing the
app into an unmaintainable monolith.
