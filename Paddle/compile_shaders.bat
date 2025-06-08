REM Compile all shaders
%VULKAN_SDK%\Bin\glslc.exe .\shader.vert -o .\shader.vert.spv
%VULKAN_SDK%\Bin\glslc.exe .\shader.frag -o .\shader.frag.spv

%VULKAN_SDK%\Bin\glslc.exe .\font.vert -o .\font.vert.spv
%VULKAN_SDK%\Bin\glslc.exe .\font.frag -o .\font.frag.spv
