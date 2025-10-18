Rendering features:
- Depth Buffer pre-pass
- Deferred Rendering pipeline
- MSAA
- SSAO
- Simple ShadowMap for global directional light source (sun)
- PBR
- Point lights
- Omnidirectional Shadow Maps

Rendering architecture
- Provide clean user API for issuing draw calls
- Encapsulate graphis-related code, e.g. OpenGL context configuration and rendering pipeline creation
- Use defines list of mesh types and material types they intend to draw in the application, context builds required objects (Pipeline, DrawPack types, ..)
- Context handles state its managed resources state transitions during frame time, e.g. call beginGeneration()/endGeneration() on required StreamBuffer types
- Context exposes exposes api: .beginFrame(), .endFrame(), betwen beginFrame and endFrame .draw() calls are allowed
- draw function template allows for handling various draw call types - AnimatedMesh with AnimationPlayer, static model, dynamic model, etc.

Refactor
- consider improving naming convention in \*/gl/draw/\*.h implementations
- refactor \*/gl/draw/\*.h implementation to improve readability
