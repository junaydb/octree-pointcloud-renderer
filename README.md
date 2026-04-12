<img src="https://ik.imagekit.io/xqhypdkfa/tr:w-2400,h-800,c-maintain_ratio,q-100/capstone/octree-vis-hd.png" alt="octree-vis-ss" width="100%"/>

## What is this?

This is software for viewing large point clouds in real-time.
It features an octree-based LOD system to render closer parts of the point cloud
in higher detail, whilst rendering distant areas in lower detail.
This allows for viewing huge point clouds in high-detail at highly interactive framerates.

You can read more about what this is and how it works in
[my blog post](https://junadb.con/posts/octree-point-cloud-renderer).

## Examples of Performance

On an M1 MacBook Pro, a 330 million point cloud can be explored at around 40 FPS
with a points-per-frame budget of 20 million.

The renderer achieves this by traversing the octree in screen-projected-size order,
rendering the most visually significant nodes first and stopping when the point budget is reached.

Without the LOD system, rendering all 330 million points every frame runs at around
1 FPS on the same machine.

The result is an interactive view that remains visually close to the original
point cloud, making it possible to accurately inspect datasets that would otherwise
be too large to render in real time.

Below, the left image shows a 330 million point cloud rendered with a 20 million
point budget, while the right image shows the same point cloud rendered with all
330 million points. Despite rendering only a fraction of the original points,
the visual difference is minimal, demonstrating how the LOD system preserves detail
while significantly reducing the rendering workload.

<table>
  <tr>
    <td width="50%">
      <a href="https://ik.imagekit.io/xqhypdkfa/capstone/elmstead-ultra-dense-ss.png">
        <img src="https://ik.imagekit.io/xqhypdkfa/tr:ar-16-10,w-985/capstone/elmstead-ultra-dense-ss.png" alt="LOD-rendered Elmstead point cloud" width="100%"/>
      </a>
    </td>
    <td width="50%">
      <a href="https://ik.imagekit.io/xqhypdkfa/capstone/elmsted-ultra-dense-original.png">
        <img src="https://ik.imagekit.io/xqhypdkfa/tr:ar-16-10,w-985/capstone/elmsted-ultra-dense-original.png" alt="Original Elmstead point cloud" width="100%"/>
      </a>
    </td>
  </tr>
  <tr>
    <td align="center"><sub>Rendering with a 20 million point budget using the LOD system</sub></td>
    <td align="center"><sub>Original 330 million point cloud</sub></td>
  </tr>
</table>

## Building

Ensure the following are installed on your system:

- A C++17 compiler (this project was developed with [GCC](https://gcc.gnu.org/install/))
- [CMake 3.10 or newer](https://cmake.org/download/)
- [GNU Make](https://www.gnu.org/software/make/)
- [SDL2](https://wiki.libsdl.org/SDL2/Installation)

On some Linux distributions, install the SDL2 package that includes headers,
such as `libsdl2-dev` or `SDL2-devel`.

Then, from the project root:

```sh
cd point-cloud-renderer
make
```

This creates a release build in `point-cloud-renderer/build/` and writes the
executable to `point-cloud-renderer/bin/PointCloudRenderer`.

For a clean rebuild:

```sh
make remove
make
```

## Usage

To start the program, from the `point-cloud-renderer/` directory, run:

```sh
./bin/PointCloudRenderer <FILE> <POINTS PER FRAME BUDGET> [POINT BUFFER BUDGET] [MIN POINTS PER NODE]
```

The arguments are the following:

- `FILE`:  
  A path to the input point cloud file. Only PLY files are supported as of now.

- `POINTS PER FRAME BUDGET`:  
  The maximum number of points to render per frame.  
  Increase this for more detail or reduce it for better performance.  
  Useful for less powerful machines.

- `POINT BUFFER BUDGET (Optional)`:  
  The maximum number of points to load into system memory (RAM).  
  When not set, there will be no buffer limit (the entire point cloud will be loaded).  
  Useful for partially loading point clouds that are too large to fit into memory.

- `MIN POINTS PER NODE (Optional)`:  
  The minimum number of points in each octree node.  
  Defaults to `10000`.  
  Lower values allow the octree to split into smaller nodes, which can improve
  LOD precision but may increase build time and memory usage.
  If this argument is set, `POINT BUFFER BUDGET` must also be provided.
