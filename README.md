# Triangles
![CI status](https://github.com/kachkov98/Triangles/workflows/Build/badge.svg)  
Triangle intersection check implementation is based on [this article](https://web.stanford.edu/class/cs277/resources/papers/Moller1997b.pdf).  
Visualization is done using GLFW and Vulkan-hpp.  

**Controls:**  
Left mouse button - camera rotation  
Right mouse button - camera movement  
Mouse scroll - zoom

## Build
### Prerequisites
Vulkan SDK should be installed.
```text
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
