# Creation Engineer Capabilities

Creation Engineer is an engineering workstation inside the Creation Suite.

The product vision is not "clone SPICE, Pro/ENGINEER, and MatLab in full."
The product vision is to distill the parts real engineers actually use most
often into one programmable, suite-native tool.

## Product Direction

Creation Engineer should feel like:

- focused engineering modeling and technical design
- simulation and numeric analysis that serve design decisions directly
- programmable workflows through CEL
- real-time visual feedback through an OpenGL-based viewport

The comparison points are:

- `SPICE` for circuit-oriented modeling and simulation
- `Pro/ENGINEER` for disciplined engineering geometry and assemblies
- `MatLab` for numeric analysis, parameter exploration, and technical scripting

But the target is not feature parity with any of them. The target is a leaner,
more integrated engineering workstation shaped around actual engineer workflows.

## Core User Workflows

Creation Engineer must support these primary workflows:

1. Build technical models of engineered systems.
2. Define parameters, constraints, and relationships between system elements.
3. Run simulations or analyses against those models.
4. Visualize results immediately in 2D, 3D, charts, tables, or derived views.
5. Iterate quickly by changing parameters rather than rebuilding models by hand.
6. Automate repeated calculations, studies, and design-space exploration with CEL.

## First Implementation Slice

The first implementation slice should establish the engineering workstation
shape before deeper solver domains arrive.

That slice should include:

- a central technical viewport surface
- a structured engineering navigator
- a parameter/property inspector
- a results and study status panel
- explicit project/VFS awareness for Engineer-owned assets

This first slice is important because it sets the user workflow:

- open a suite project
- see Engineer assets in context
- create and manage engineering objects directly inside the workstation
- inspect a technical model in the viewport
- change parameters and properties
- review status, metrics, and later analysis outputs nearby

The first release of this slice does not need full solver depth yet, but it
must already look and behave like the skeleton of a serious engineering tool.

## Engineering Modeling

Creation Engineer should support structured engineering models rather than
freeform artistic content creation.

Required modeling direction:

- parametric geometry and dimension-driven construction
- assemblies and subassemblies
- technical coordinate systems, references, and constraints
- engineering objects with meaningful properties, not just meshes
- editable model hierarchies suitable for design revision

This is closer to engineering authoring than general DCC sculpting.

## Parametric Primitive To Editable Geometry Workflow

Creation Engineer should include a practical workflow based on built-in
parameterized engineering shapes.

These should include things like:

- blocks
- cylinders
- tubes
- plates
- shafts
- holes
- slots
- simple framed or bracket-like primitives

The expected workflow is:

1. create a built-in primitive
2. drive it through engineering parameters and dimensions
3. iterate until the shape and fit are right
4. optionally freeze or lock it into explicit geometry
5. continue editing it as geometry/vertices when that is the right next step

This is important because not every engineering object should stay purely
parametric forever, but the parametric phase should still get the user most of
the way there quickly.

After freezing, the object should:

- preserve its engineering identity and project membership
- remain editable
- switch from parameter-first editing to geometry-first editing
- still participate in assemblies, inspection, and analysis where appropriate

The intent is not to force one modeling religion.
The intent is to let engineers start with disciplined parameterized forms and
then deliberately commit to direct geometry when needed.

The current workstation implementation should explicitly surface that workflow
in the UI so users can see which objects are still primitive-backed, which
ones are carrying live modifiers, and which ones have been committed into
direct geometry.

The navigator should also act as the first scene-management surface by letting
the user:

- add new primitive objects
- duplicate selected engineering objects
- remove selected objects without leaving the workstation context

The workstation should also expose a dedicated modifier-stack surface instead
of hiding modifiers entirely inside general object properties.

The first live modifier-stack panel should support:

- adding high-value early modifiers such as mirror and shell
- enabling or disabling modifiers
- reordering modifiers in the stack
- removing modifiers cleanly
- keeping the selected object's authoring state synchronized with stack changes

## Direct Geometry Tool Surface

Once an engineering object is committed into direct geometry, the workstation
should expose a dedicated editing surface for that state instead of forcing the
user back through primitive or modifier controls.

The first live direct-geometry tool panel should support:

- selecting a current geometry tool mode
- translation nudges for committed parts
- direct scaling adjustments
- extrusion depth changes
- bevel amount changes

This is not the end-state geometry editor yet, but it establishes that
committed parts remain actively editable inside Engineer.

## Geometry Element Proxies

Direct geometry should also expose selectable element-level proxies so editing
can move beyond whole-part transforms.

The first live pass should support:

- vertex proxy selection
- edge proxy selection
- face proxy selection
- stepping between available elements
- nudging the selected element directly from a docked control surface
- selecting the active element directly in the viewport
- dragging the selected viewport proxy to manipulate the committed shape

These proxies give Engineer an engineering-friendly bridge toward fuller
vertex/edge/face editing without pretending the full mesh editor is already
finished.

The first viewport gizmo pass should also support:

- axis-constrained translation handles
- axis-constrained scaling handles
- dedicated extrusion and bevel handles
- planar center-handle dragging for whole-part direct-geometry movement
- snapping enable/disable control
- selectable numeric snap increments for geometry edits

The viewport interaction model should avoid permanent banks of buttons across
the render surface. Core viewport commands should be available from a
right-click context menu so the modeling area stays visually clear while still
exposing fast access to mode, camera, primitive, and direct-geometry actions.

The baseline camera interaction should feel closer to CAD and Blender-style
navigation:

- middle-drag orbiting around the working view
- shift plus middle-drag panning
- mouse-wheel zoom
- quick camera preset switching and navigation reset from the viewport menu

Selection and movement should also stay deliberate rather than collapsing into
accidental drag behavior:

- plain left click selects a part or geometry target without moving it
- whole-object movement for non-geometry parts should come from an explicit
  viewport move handle
- direct-geometry movement should stay tied to visible gizmos and element
  proxies instead of implicit drag-anywhere behavior

## Modifier Stack Workflow

Creation Engineer should also support a modifier-style workflow similar in
spirit to Blender, but adapted for engineering use.

That means objects should be able to carry ordered, non-destructive operations
that can be edited, reordered where safe, enabled or disabled, and eventually
committed when the user chooses.

Important modifier directions include:

- mirror
- array or patterned repetition
- boolean combine/cut/intersect
- bevel or fillet/chamfer-style shaping
- shell or thickness
- simple deformation where engineering use cases justify it
- cleanup or topology-preparation steps where useful

The expected workflow is:

1. start from a parameterized primitive or existing engineering object
2. apply one or more modifiers non-destructively
3. inspect the result in both technical and rendered views
4. continue editing the underlying parameters, modifier settings, or both
5. optionally freeze/commit the result into explicit geometry

This gives Engineer three useful modeling layers:

- parameter-driven engineering primitives
- modifier-driven non-destructive shaping
- direct geometry editing after commitment

The first live modifier pass should start with a high-value operation:

- mirror across a major axis

That is enough to validate the authoring-state model before the broader stack
arrives.

That combination is much more powerful than forcing every edit into either
strict CAD history or immediate vertex editing.

## First Modeling Toolset

Once an object reaches direct geometry editing, the first serious toolset
should borrow proven fundamentals from workflows like Blender while keeping the
behavior engineering-friendly.

The initial direct-geometry tools should include:

- move, rotate, and scale with numeric precision and axis locking
- grid, point, edge, face, and increment snapping
- extrude
- bevel/chamfer
- loop cut
- knife/cut
- merge
- dissolve
- duplicate/split where useful

These tools should work on explicit geometry elements such as:

- vertices
- edges
- faces

They should remain grounded in engineering expectations:

- unit-aware input
- precise pivots and coordinate systems
- predictable selection behavior
- compatibility with assemblies and later analysis workflows

## First Modifier Set

The first modifier family should focus on the highest-value operations for
practical engineering iteration.

Initial modifier targets:

- mirror
- array/pattern
- boolean
- shell/thickness
- bevel/chamfer

Those modifiers give a strong early foundation for creating real engineered
parts quickly without forcing immediate manual topology work.

## Simulation And Analysis

Creation Engineer must provide a technical analysis layer tied directly to the
modeled system.

Required direction:

- parameter-driven simulation
- repeatable studies and sweeps
- numerical computation and derived values
- graphing, charts, and technical result inspection
- domain solvers that can be added over time instead of one giant monolith

Examples of intended use:

- electrical/circuit-style analysis
- mechanical or kinematic studies
- structural evaluation of practical parts and assemblies
- control/system response analysis
- engineering what-if exploration

## Electrical System Modeling

Creation Engineer must support electrical modeling at the system level, not
just isolated schematic fragments.

Important direction:

- model larger electrical building blocks as connected subsystems
- support workflows like:
  - power source
  - regulator
  - control unit
  - motor or actuator
- evaluate current flow through the whole chain
- estimate battery life and power consumption
- reason about component ratings and operating margins
- connect electrical behavior to the physical system being designed

This should make it possible to study a practical device as an engineered
system rather than only as disconnected circuit sheets.

## Circuit Analysis

Creation Engineer should also support lower-level circuit analysis for
electrical design where needed.

Required direction:

- resistive circuit analysis
- capacitive and inductive behavior
- AC analysis for practical circuits
- evaluation of voltage, current, impedance, and response
- practical mixed modeling between subsystem-level power paths and lower-level
  circuit behavior

## Low-Level Electronics Modeling

Creation Engineer should leave room for more SPICE-like electronics work at the
component level.

Expected direction:

- resistor-level and passive-component analysis
- semiconductor-aware circuit modeling
- low-level electronics behavior where that matters to the design

The goal is not to become an exhaustive semiconductor research package before
everything else exists.

The goal is to support the range from:

- system-level electrical architecture

down to:

- meaningful low-level electronics analysis

when real engineering work needs both.

## Mechanical Engineering Focus

Creation Engineer must take mechanical engineering seriously.

Required direction:

- dimension-driven part modeling
- assemblies with real mating and constraint logic
- mass, fit, clearance, and motion-aware design work
- practical tolerance-aware engineering workflows
- accuracy appropriate for real product and mechanism design

The intent is closer to real mechanical engineering authoring than casual 3D
shape creation.

## Dual Mechanical Viewing Modes

Creation Engineer should support two complementary mechanical viewing modes
over the same underlying engineering data.

The first is the expected engineering design view:

- precise CAD-style technical viewing
- orthographic and engineering camera controls
- dimension, reference, section, and constraint overlays
- direct inspection of part relationships and editable design intent

The second is an immersive assembly-floor view:

- a rendered spatial walkthrough mode
- the ability to stand inside or around an assembly like a game environment
- practical inspection of space claims, fit, reach, access, and human-scale
  clearances
- visual validation of how parts come together in a more physical sense than a
  purely technical viewport provides

These are not separate products.
They are two views over the same engineering model and assembly state.

The goal is to let an engineer move fluidly between:

- exact design work

and:

- spatial, rendered, experiential inspection

without rebuilding the model in another tool.

## Practical Finite Element Analysis

Creation Engineer should support finite element analysis as a practical design
tool.

This is not aimed at extreme aerospace, defense, or giant-enterprise analysis
from day one.

It is aimed at:

- home robotics
- electromechanical devices
- mechanisms
- enclosures
- brackets
- frames
- practical engineered consumer or small-system parts

Required direction:

- stress and deformation analysis for real parts
- GPU-assisted solving where that materially improves iteration speed
- engineering feedback that is fast enough to use during design iteration
- mesh/result workflows that remain understandable to working engineers

The standard is not "simulate an airliner wing on day one."
The standard is "help a real engineer design and validate practical hardware."

## Visualization

Creation Engineer must include a strong OpenGL-driven technical viewport and
result visualization layer.

Required direction:

- precise 3D viewing for engineered models
- a real scene-based viewport rather than a painted 2D proxy
- selectable model components and technical overlays
- technical display modes rather than purely artistic shading
- immediate visual response to parameter or solver changes
- result overlays such as annotations, guides, vectors, states, or field views

That same surface should also be able to host orthographic planar workflows
when the engineering task is layer-driven rather than volumetric, including:

- electronics layout views
- layered board design
- future system-design views where components, paths, or cable runs must be
  understood both spatially and schematically

## CEL Programming Surface

CEL is a core part of the product, not an afterthought.

Creation Engineer must support CEL for:

- parameter definitions
- automation of model generation
- simulation setup
- analysis scripts
- design studies and sweeps
- custom derived metrics
- repeatable engineering workflows

The tool should be programmable by engineers without forcing everything through
a traditional external coding environment.

## Asset Model

Creation Engineer should own engineering-domain assets inside the shared suite
project model.

Expected asset classes:

- engineering models
- assemblies
- parameter sets
- simulation definitions
- solver setups
- analysis scripts
- result datasets
- plots, reports, and derived technical outputs

## Shared Platform Dependencies

Creation Engineer should consume the suite platform for:

- VFS-backed project storage
- project discovery and shared project context
- shared AI/provider routing
- shared shell and account/session behavior
- shared asset identity and interop rules

## Import And Export

Creation Engineer should support practical technical interchange where it
matters, while keeping suite-native assets first-class.

Required direction:

- import of relevant engineering data and geometry formats where licensing allows
- export of reports, result tables, and technical artifacts to the filesystem
- export of engineered outputs needed by downstream tools

## Explicit Non-Goals

Creation Engineer is not intended to be:

- a full clone of any one legacy engineering package
- a general-purpose artistic modeling package
- an unrestricted symbolic-math research environment
- a kitchen-sink CAD platform with every niche feature before core workflows exist

The standard remains the same as the rest of the suite:

- implement what real users actually need
- keep the architecture programmable and extensible
- avoid dead-weight surface area that engineers rarely touch
