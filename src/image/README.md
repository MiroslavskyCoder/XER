# Flux Image Engine: Professional Grade Processing

## Описание
Данный модуль является агрегатором лучших в индустрии библиотек для обработки изображений. Архитектура построена на принципе "использовать лучший инструмент для каждой задачи".

## Технологический стек (The Engine Core)
*   **Чтение и Декодирование:**
    *   `LibRaw`: Профессиональная интерпретация сенсорных данных (DNG, CR2, NEF).
    *   `libjpeg-turbo`: Молниеносное декодирование JPEG (использование SIMD).
    *   `LibHEIF`: Поддержка стандартов HEIC/HEIF.
    *   `OpenImageIO`: Метаданные и сложные форматы.
*   **Математика и Манипуляции:**
    *   `Eigen`: Линейная алгебра, тензорные преобразования для нейросетей.
    *   `ZIMG`: Самые качественные алгоритмы ресайза (Lanczos, Spline36).
*   **Цвет и HDR:**
    *   `OpenColorIO`: Профессиональное управление цветом (ACES workflow).
    *   `OpenEXR`: Работа с 32-битными данными (Float).
*   **Алгоритмы и AI:**
    *   `OpenCV (dnn + ximgproc)`: Нейросетевое увеличение разрешения (Super Resolution) и денойзинг.
    *   `VTK`: Геометрические искажения и визуализация 3D-проекций.
    *   `Tesseract`: Встроенный OCR для извлечения текста с фотографий (документы, номера машин).

## Почему это "Убийца формата"?
1.  **Формат-независимость:** Мы не смотрим на расширение. `MagicNumberDetector` направляет поток в соответствующий адаптер.
2.  **Неразрушающий AI:** Применяем AI-денойзинг (через OpenCV DNN) как отдельный слой графа, не меняя исходные данные.
3.  **Цветовая точность:** Используя `OpenColorIO`, мы гарантируем, что RAW-файл при конвертации в JPEG сохранит точность передачи тонов, соответствующую профессиональным цветовым профилям мониторов.

## Архитектура Pipeline (Пример)
```cpp
// 1. RAW-конвертация (LibRaw -> Eigen Tensor)
RawAdapter raw_loader;
auto tensor = raw_loader.load_to_eigen(path);

// 2. AI Enhancement (OpenCV DNN)
DNNSuperRes model;
model.upscale(tensor, 2.0); // Увеличиваем разрешение нейросетью

// 3. Цветокоррекция (OpenColorIO)
OCIOColorPipe pipe("aces_cg", "srgb");
pipe.apply(tensor);

// 4. OCR (Tesseract)
if (settings.extract_text) {
    auto text = TesseractWrapper::recognize(tensor);
}
---

# Модуль Image Processing (src/image)

## Обзор
Модуль `src/image` — это профессиональный движок для обработки изображений, разработанный для высокоточных визуальных задач. В отличие от простых библиотек, он обеспечивает сквозной пайплайн: от распознавания "сырых" байтовых потоков до сложного визуального композитинга и анализа данных.

## Архитектурный стек
Система построена на интеграции мощных индустриальных библиотек:
*   **[OpenImageIO (OIIO)](https://openimageio.org/):** Ядро для I/O, чтения метаданных и работы с HDR/RAW форматами.
*   **[OpenColorIO (OCIO)](https://opencolorio.org/):** Управление цветом и LUT-преобразования (Color Managed Pipeline).
*   **[OpenEXR](https://www.openexr.com/):** Работа с высокодинамическим диапазоном (Deep Pixels).
*   **[Eigen](https://eigen.tuxfamily.org/):** Линейная алгебра для высокоскоростных матричных операций (эффекты, деформации).
*   **[VTK](https://vtk.org/):** Продвинутая визуализация и алгоритмы обработки данных.
*   **[OpenCV](https://opencv.org/):** Компьютерное зрение, базовые фильтры и морфология.

## Распознавание без расширений (Magic Bytes Engine)
Модуль использует `MagicNumberDetector` для идентификации типа файла. Система считывает первые 16 байт потока и сопоставляет их с сигнатурами из базы. Это гарантирует поддержку файлов с любыми именами или вовсе без расширений (типично для баз данных и сетевых передач).

## Визуальные эффекты (VFX)
Система VFX работает через **графовый процессор**:
1.  **Filter Chain:** Очередь неразрушающих операций (Blur, Sharpen, Noise reduction).
2.  **Color Pipeline:** Преобразование цветовых пространств через OCIO (ACES, LogC, sRGB).
3.  **VFX Engine:** Использование Eigen для матричных искажений (Warp, Displacement) и VTK для 3D-манипуляций (Projection Mapping).

## Инструментарий фотографа
*   **Non-Destructive Layers:** Система слоев, где каждая операция — это нод в графе.
*   **Retouch Tool:** Использование алгоритмов OpenCV (Inpainting, Delaunay triangulation для ретуши).
*   **Histogram Analyzer:** Анализ плотности пикселей с использованием тензорных вычислений Eigen.

## Иерархия модулей

```
src/image/
├── core/           # Общие структуры (ImageBuffer, PixelFormat)
├── decoding/       # Фабрика декодеров (Magic Byte Identification)
├── formats/        # Специализированные адаптеры (OIIO/OpenEXR wrappers)
├── effects/        # VFX движок (VFXEngine, FilterChain)
├── tools/          # Фото-инструментарий (Layers, Retouch)
└── io/             # Интерфейсы записи/чтения
```

## Установка и зависимости
Для сборки модуля необходимы следующие зависимости:
```bash
# Пример для Linux (Ubuntu/Debian)
apt-get install libopenimageio-dev libopencolorio-dev libopenexr-dev libvtk9-dev libopencv-dev
```

## Пример использования: Обработка изображения

```cpp
#include "src/image/decoding/decoder_factory.h"
#include "src/image/effects/vfx_engine.h"
#include "src/image/core/image_buffer.h"

// 1. Авто-определение и декодирование через OIIO/OpenEXR
auto decoder = DecoderFactory::instance().create(raw_stream);
ImageBuffer image = decoder->decode(raw_stream);

// 2. Применение Color Management через OpenColorIO
image.apply_colorspace("linear", "rec709");

// 3. VFX движок (Использование матриц Eigen для деформации)
VFXEngine engine;
engine.add_effect<DisplacementMap>(map_texture);
engine.process(image);

// 4. Анализ через OpenCV/VTK
HistogramAnalyzer analyzer;
analyzer.compute(image);
```

## Контрибьюторам
При добавлении нового формата:
1. Создайте класс-наследник `BaseDecoder`.
2. Зарегистрируйте сигнатуру (Magic Bytes) в `MagicNumberDetector`.
3. Реализуйте метод `decode()` через API `OpenImageIO`.
4. Добавьте юнит-тест в `tests/image/`.

---
*Модуль разработан для профессиональной обработки изображений, обеспечивая точность цвета уровня ACES и производительность на базе SIMD-инструкций Eigen.*


### Дополнительные советы по реализации:
*   **Zero-Copy:** Чтобы не тормозить, не копируйте массив пикселей из OpenCV в Eigen. Используйте `Eigen::Map`, который просто указывает на существующий массив в памяти OpenCV (`cv::Mat::data`).
*   **Threading:** Используйте `tbb` (Intel Threading Building Blocks), так как OpenCV и Eigen умеют с ним работать. Это позволит распределить тяжелую обработку фотографии на все ядра процессора.
*   **GPU:** Поскольку вы упоминали CUDA ранее, убедитесь, что OpenCV и VTK собраны с `WITH_CUDA=ON`. Тогда AI-апскейлинг будет работать на видеокарте.