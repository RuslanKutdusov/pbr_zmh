rmdir /S /Q pbr_zmh_release
mkdir pbr_zmh_release
xcopy Media\* pbr_zmh_release\Media /s /i
xcopy pbr_zmh\shaders\* pbr_zmh_release\shaders /s /i
copy pbr_zmh\bin\pbr_zmh.exe pbr_zmh_release\
copy pbr_zmh\bin\assimp-vc140-mt.dll pbr_zmh_release
copy pbr_zmh\bin\d3dcompiler_47.dll pbr_zmh_release
copy vc_redist.x64.exe pbr_zmh_release
copy RELEASE_README.txt pbr_zmh_release
