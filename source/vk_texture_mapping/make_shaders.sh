#!/bin/bash

echo "Compiling shaders..."
glslc triangle.vert -o triangle.vert.spv
glslc triangle.frag -o triangle.frag.spv
