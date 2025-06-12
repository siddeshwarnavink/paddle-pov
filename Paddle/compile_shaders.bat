REM Compile all shaders
%VULKAN_SDK%\Bin\glslc.exe .\Shader\shader.vert -o .\Shader\shader.vert.spv
%VULKAN_SDK%\Bin\glslc.exe .\Shader\shader.frag -o .\Shader\shader.frag.spv

%VULKAN_SDK%\Bin\glslc.exe .\Shader\font.vert -o .\Shader\font.vert.spv
%VULKAN_SDK%\Bin\glslc.exe .\Shader\font.frag -o .\Shader\font.frag.spv
