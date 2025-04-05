# Gaia

GAIA is a work in progress physically based Vulkan renderer. It supports GLTF model loading, a pbr material system, smart handling of vulkan objects using lightweight handles and object pools.

Features I plan to implement next:-
* Cascaded shadow mapping or RT shadows.
* Physically based lighting using disney BSDF.
* A real time GI technique (need to do some research).
* Explore and implement Forward, Deferred, Forward+ rendering (any one of these).
* Ambient occlusion (any technique).
* Entity component system.
* Skeletal animations.
* PCG forest generation on large scale and rendering(same as my Hazel engine).
* Rendering terrain.
* Frustum and occlusion culling.
* Implement different lights directional, point, spot, area.

Some screen-shots of my renderer at current state.

path-trace render using hardware accelerated ray-tracing.
![Screenshot (26)](https://github.com/user-attachments/assets/cda92d47-5ba5-4885-b239-18ac18fa3699)

![Screenshot (27)](https://github.com/user-attachments/assets/0925805e-637d-4bed-94dc-f2b6d5687186)

Cascaded Shadow Mapping.
![Screenshot 2025-03-28 121310](https://github.com/user-attachments/assets/b5763a37-120b-45d4-9bde-8449f2731fbf)
