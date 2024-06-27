I suppose need to write these down to remind myself

# Scene Management
The engine provides a generic Scene struct that should be embedded in a another struct, which provide scene-specific data.

As such, the engine will manage pointers to Scene struct. When using the engine, it is expected to provide the a pointer to the embedded Scene struct to the engine for scene management. All scenes should be declared upfront and an array of the Scene pointers should be provided.

## What is a scene?
A scene is a data struct containing these major fields:
1. Entity Manager
2. Input-Action Map
3. Systems
4. Particle System
5. plus other fields

These fields exists to perform the scene update loop of:
1. Input handling
2. Scene Update
3. Scene Rendering

## Scene State
In this implementation, there will always be a 'root' scene at any given time. The program ends if there is no 'root' scene. A scene change/transition occurs when this 'root' scene is changed to another scene.

A scene has an active state and a render state.
- Active: Should it run its scene update procedures?
- Render: Should it be render?
Hence, it is possible to have a scene updated but hidden, or a paused scene (not updated but rendered).

There is also a 'focused' scene. This is the scene to intercept inputs and run its action update procedure. There can only be at most one such scene. Two or more scenes are not allowed to receive inputs in this implementation. It is possible to have zero focused scene.

# Scene Hierachy
A scene can have multiple children scenes but can have only one parent scene. This implemented as a intrusive linked-list.

The implementation assumes a single-threaded environment for simplicity. The scene update travesal logic is as such:
1. The current scene first
2. If the scene has a child scene, traverse that child first.
3. If the scene has a next scene, traverse there then.
4. Repeat from (1) until no possible scene is traversable
