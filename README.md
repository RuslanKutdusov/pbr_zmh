# pbr_zmh
Physically Based Rendering Sample.

У сэмпла есть глобальные контролы
![](https://raw.githubusercontent.com/RuslanKutdusov/pbr_zmh/master/screenshots/controls.png)
С помощью мыши осуществляется управление камерой.

В самом низу этих контролов, есть выпадающий список со сценами. 
Сцены 3:
# 1. Одна сфера

![](https://raw.githubusercontent.com/RuslanKutdusov/pbr_zmh/master/screenshots/one_scene_controls.png)
Сцена представляет собой одну сферу. Камера вращается вокруг нее. С помощью контролов можно задать metalness(металличность), 
roughness(шероховатость), reflectance, albedo(цвет поверхности для диэлектриков и F0 для металлов). Если будет активирован checkbox "Use material", то эти параметры буду браться из материала,
выбранного в списке ниже.
![](https://raw.githubusercontent.com/RuslanKutdusov/pbr_zmh/master/screenshots/one_scene_material.png)

# 2. Множество сфер

На этой сцене расположено несколько сфер. У каждой сферы свое значение metalness и roughness. Metalness меняется от 0 до 1 вдоль оси X, а roughness меняется от 0 до 1 вдоль оси Z.
Контрол здесь только один - albedo.
![](https://raw.githubusercontent.com/RuslanKutdusov/pbr_zmh/master/screenshots/multiple_spheres.png)

# 3. Sponza

Эта сцена освещена направленным источником света и точечным источником, который движется вдоль сцены. Контролы позволяют задать его яркость
![](https://raw.githubusercontent.com/RuslanKutdusov/pbr_zmh/master/screenshots/sponza.png)

# Ссылки

* SIGGRAPH Courses
http://blog.selfshadow.com/publications/s2012-shading-course/
http://blog.selfshadow.com/publications/s2013-shading-course/
http://blog.selfshadow.com/publications/s2014-shading-course/
http://blog.selfshadow.com/publications/s2015-shading-course/
http://blog.selfshadow.com/publications/s2016-shading-course/
http://blog.selfshadow.com/publications/s2017-shading-course/

* SIGGRAPH 2013 Course. Background: Physics and Math of Shading (Naty Hoffman).
http://blog.selfshadow.com/publications/s2013-shading-course/hoffman/s2013_pbs_physics_math_notes.pdf

* Microfacet Models for Refraction through Rough Surfaces. Walter 2007.
https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf

* SIGGRAPH 2014 Course. Moving Frostbite to PBR (Sébastien Lagarde & Charles de Rousiers).
https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf

* Real-Time Rendering, Third Edition.
http://www.realtimerendering.com/book.html

* Physically Based Rendering, Third Edition.
http://www.pbrt.org/

* PBR Diffuse Lighting for GGX+Smith Microsurfaces. GDC 2017.
https://www.gdcvault.com/play/1024478/PBR-Diffuse-Lighting-for-GGX

* Specular BRDF Reference.
http://graphicrants.blogspot.ru/2013/08/specular-brdf-reference.html

