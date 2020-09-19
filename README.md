Potato3d

A simple, fast 3d renderer for stuff with slow processors.


Still very much a work in progress.


Currently uses BSP to sort and cull polygons.

Very little division in the hot path. I've been able to use a reciprocal table to get rid of most division.


Renders textures perspective-correctish. Perspective correct tex corodinates are computed ever 16px and linear interpolated from there.


Rendering is front-to-back with no Z-buffer and zero overdraw. But the span buffering is still a performance hog.

