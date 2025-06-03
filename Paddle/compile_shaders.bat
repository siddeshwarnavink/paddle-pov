REM Compile shaders
%VULKAN_SDK%\Bin\glslc.exe .\simple_shader.vert -o .\simple_shader.vert.spv
%VULKAN_SDK%\Bin\glslc.exe .\simple_shader.frag -o .\simple_shader.frag.spv
