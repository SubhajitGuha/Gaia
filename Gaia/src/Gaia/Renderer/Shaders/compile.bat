C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 deferred/g_buffer.vert -o g_buffer.vert.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 deferred/g_buffer.frag -o g_buffer.frag.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 deferred/global_illumination.vert -o global_illumination.vert.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 deferred/global_illumination.frag -o global_illumination.frag.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 deferred/deferred.vert -o deferred.vert.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 deferred/deferred.frag -o deferred.frag.spv

C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 shadow.vert -o vert_shadow.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 shadow.frag -o frag_shadow.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 rayGen.rgen -o rayGen.rgen.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 rayMiss.rmiss -o rayMiss.rmiss.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 shadowMiss.rmiss -o shadowMiss.rmiss.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 closestHit.rchit -o closestHit.rchit.spv

C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 ddgi/gi_rayGen.rgen -o gi_rayGen.rgen.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 ddgi/gi_rayMiss.rmiss -o gi_rayMiss.rmiss.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 ddgi/gi_rayMissShadow.rmiss -o gi_rayMissShadow.rmiss.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 ddgi/gi_closestHit.rchit -o gi_closestHit.rchit.spv

C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 ddgi/gi_depth_probe_update.comp -o gi_depth_probe_update.comp.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 ddgi/gi_irradiance_probe_update.comp -o gi_irradiance_probe_update.comp.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 ddgi/gi_irradiance_border_update.comp -o gi_irradiance_border_update.comp.spv
C:\VulkanSDK\1.3.296.0\Bin\glslc --target-spv=spv1.6 ddgi/gi_depth_border_update.comp -o gi_depth_border_update.comp.spv


pause