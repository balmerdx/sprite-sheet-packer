# Что нового в этой ветке
Прежде всего - все новые фичи доступны только из консольки. Из GUI ничего нового не доступно. Так-же исправлено достаточно большое количество багов. Т.к. у нас достаточно крупный проект с большим количеством картинок, то нашли множество случаев, при которых Sprite Sheet Packer вел себя некорректно. Из последнего - спрайты шириной либо высотой 1 пиксель (и одинаковых размеров) всегда считались одинаковыми по содержимому.

Из новых фич. Прежде всего - полигональная упаковка. Теперь она работает без ошибок. Кроме того, она работает примерно на уровне коммерческих конкурентов как по плотности упаковки, так и по скорости. Параметр "количество полигонов на единицу площади" и "минимизация площади полигонов" так-же достаточно хорошо сбалансированы на нашем датасете картинок.
![Полигональная упаковка](/images/polygonal_example.webp)

### Выравнивение спрайтов на определённую границу.
Так-же есть довольно необычная поддержка ASTC формата. Дело в том, что мы многие атласы собираем "на лету". Раньше мы для этого использовали RGBA8888 формат и соответственно спрайты могли располагаться произвольно. Сейчас же мы используем ASTC 4x4 формат, и соответсвенно нам нужно как выравнивание паддингов на границу 4 пикселя, так и требования по минимальному расстоянию. Флаги *granularityX/granularityY*

### Ускорение генерации атласов.
Для ускорения генерации полигональных атласов есть возможность разделить этапы генерации полигонов для спрайтов и генерации атласов. Флаги *make-polygon-info/use-polygon-info*.
Если требуется только атлас, без изображения, можно отключить его генерацию disable-save-image.

### Поддержка "атлас в атласе".
У нас есть случаи, когда атласы сторонних приложений вставляются в большие атласы. В этом случае есть вожномжность не обрезать их полигонально используя флаг *trim-rect-list*.

# SpriteSheet Packer
Sprite sheet generator base on Qt created by Aleksey Makaseev.

### Features ###
* Support multiple screen resolutions
* Pack multiple sprite sheets at once
* Trimming / Cropping (Save space by removing transparency)
* Include GUI and command-line interface.
* Support MacOS, Windows and Linux

Uses [Qt - Digia Plc](http://qt-project.org), LGPL license.

### Supported publish spritesheet formats ###
* [cocos2d-x](http://www.cocos2d-x.org) (plist)
* [pixijs](http://www.pixijs.com) (json)
* [phaser](https://phaser.io) (json)
* simple json


## Documentation
See the [Documentation](http://amakaseev.github.io/sprite-sheet-packer) for use.


## Release builds
Download [pre-build](https://github.com/amakaseev/sprite-sheet-packer/releases)


## License
See the [LICENSE](LICENSE.md) file for license rights and limitations (MIT).


## Changelog
[CHANGELOG](CHANGELOG.md) file for all change logs.


## Developers
See the [AUTHORS](AUTHORS.md) file.

