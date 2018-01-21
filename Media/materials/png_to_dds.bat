texconv metalness.tga -m 0 -if BOX
texconv roughness.tga -m 0 -if BOX
texconv normal.tga -m 0 -if BOX
texconv albedo.tga -m 0 -if BOX -srgbi -srgb
del *.tga