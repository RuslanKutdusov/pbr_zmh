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
