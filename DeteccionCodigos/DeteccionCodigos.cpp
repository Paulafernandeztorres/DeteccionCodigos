#include "DeteccionCodigos.h"

/**
 * @brief Constructor de la clase DeteccionCodigos.
 *
 * Este constructor inicializa la interfaz de usuario y configura las conexiones
 * entre los botones de la interfaz gráfica y sus correspondientes slots. También
 * inicializa la adquisición de video desde la cámara.
 *
 * @param parent Puntero al widget padre. Por defecto es nullptr.
 */
DeteccionCodigos::DeteccionCodigos(QWidget *parent)
    : QMainWindow(parent)
{
    // Configuración inicial de la interfaz gráfica de usuario.
    ui.setupUi(this);

    // Inicialización del objeto de adquisición de video con la cámara predeterminada (ID 0).
    //camera = new CVideoAcquisition(0);
    // También se podría usar un flujo RTSP en lugar de la cámara local.
    // camera = new CVideoAcquisition("rtsp://admin:admin@192.168.1.101:8554/profile0");
    // Flujo RTSP del ESP32
    camera = new CVideoAcquisition("rtsp://10.119.60.99:8554/mjpeg/1");

    qDebug() << "Conectando con la camara...";

    // Configuración de botones de la interfaz como botones de tipo "checkable" (pueden mantenerse pulsados).
    ui.btnStop->setCheckable(true);
    ui.btnRecord->setCheckable(true);
    ui.btnDecodeImage->setCheckable(true);
    ui.btnRedMask->setCheckable(true);
    ui.btnGreenMask->setCheckable(true);

    // Conexión de señales de los botones a sus respectivos slots.
    connect(ui.btnRecord, SIGNAL(clicked(bool)), this, SLOT(RecordButton(bool)));
    connect(ui.btnStop, SIGNAL(clicked(bool)), this, SLOT(StropButton(bool)));
    connect(ui.btnDecodeImage, SIGNAL(clicked(bool)), this, SLOT(DecodeImagen(bool)));
    connect(ui.btnRedMask, SIGNAL(clicked(bool)), this, SLOT(ViewRedMask(bool)));
    connect(ui.btnGreenMask, SIGNAL(clicked(bool)), this, SLOT(ViewGreenMask(bool)));
    connect(ui.btnSaveImage, SIGNAL(clicked()), this, SLOT(SaveDecodedCode()));

    // Nota: Los botones 'Record', 'Stop', etc., están definidos en el archivo .ui asociado.
}


/**
 * @brief Destructor de la clase DeteccionCodigos.
 *
 * Libera los recursos asignados dinámicamente durante la vida útil del objeto,
 * asegurándose de evitar fugas de memoria al eliminar el objeto de adquisición de video.
 */
DeteccionCodigos::~DeteccionCodigos()
{
    // Liberar la memoria asignada al objeto de la cámara.
    if (camera != nullptr) {
        delete camera;
    }

    // Nota: No es necesario liberar recursos que estén gestionados por el framework Qt,
    // ya que Qt se encarga de eliminar widgets hijos y otros elementos al destruir el objeto principal.
}

/**
 * @brief Actualiza la imagen mostrada en la interfaz según el modo actual seleccionado.
 *
 * Esta función captura una imagen desde la cámara, la procesa según el modo actual
 * (Normal, Decoded, RedMask, GreenMask) y muestra la imagen procesada en un QLabel
 * de la interfaz gráfica.
 */
void DeteccionCodigos::UpdateImage() {
    // Capturar la imagen desde la cámara.
    imgcapturada = camera->getImage();

    // Declaración de la imagen final en formato QImage para mostrarla en la interfaz.
    QImage qimg;

    // Procesar la imagen de acuerdo al modo seleccionado.
    switch (currentMode) {
        case Normal:
            // Modo normal: mostrar la imagen capturada sin modificaciones.
            imagenFinal = imgcapturada;
            qimg = QImage((const unsigned char *)( imagenFinal.data ),
                          imagenFinal.cols,
                          imagenFinal.rows,
                          imagenFinal.step,
                          QImage::Format_BGR888);
            break;
        case Decoded:
            // Modo decodificado: procesar la imagen (por ejemplo, pasarla a escala de grises).
            imagenFinal = getSegmentedImage(imgcapturada);
            qimg = QImage((const unsigned char *)( imagenFinal.data ),
                          imagenFinal.cols,
                          imagenFinal.rows,
                          imagenFinal.step,
                          QImage::Format_BGR888);
            break;
        case RedMask:
            // Modo máscara roja: aplicar varios pasos de procesamiento.
            // 1. Filtrar la imagen para suavizarla y reducir el ruido.
            imagenFinal = BlurImage(imgcapturada, 7);
            // 2. Convertir la imagen a formato HSV.
            imagenFinal = convertHSVImage(imagenFinal);
            // 3. Generar la máscara roja.
            imagenFinal = getRedMask(imagenFinal);
            // 4. Aplicar la máscara a la imagen original.
            imagenFinal = applyMaskToImage(imgcapturada, imagenFinal);
            qimg = QImage((const unsigned char *)( imagenFinal.data ),
                          imagenFinal.cols,
                          imagenFinal.rows,
                          imagenFinal.step,
                          QImage::Format_BGR888);
            break;
        case GreenMask:
            // Modo máscara verde: aplicar varios pasos de procesamiento.
            // 1. Filtrar la imagen para suavizarla y reducir el ruido.
            imagenFinal = BlurImage(imgcapturada, 7);
            // 2. Convertir la imagen a formato HSV.
            imagenFinal = convertHSVImage(imagenFinal);
            // 3. Generar la máscara verde.
            imagenFinal = getGreenMask(imagenFinal);
            // 4. Aplicar la máscara a la imagen original.
            imagenFinal = applyMaskToImage(imgcapturada, imagenFinal);
            qimg = QImage((const unsigned char *)( imagenFinal.data ),
                          imagenFinal.cols,
                          imagenFinal.rows,
                          imagenFinal.step,
                          QImage::Format_BGR888);
            break;
    }
    // Mostrar la imagen procesada en el QLabel de la interfaz gráfica.
    ui.label->setPixmap(QPixmap::fromImage(qimg));
}

/**
 * @brief Maneja el comportamiento del botón "Record".
 *
 * Esta función inicia o detiene la captura de imágenes desde la cámara según el estado del botón.
 * También configura un temporizador para actualizar la imagen mostrada en la interfaz gráfica.
 *
 * @param captura Indica si el botón "Record" está activado (`true`) o desactivado (`false`).
 */
void DeteccionCodigos::RecordButton(bool captura) {
    // Mensaje de depuración para verificar el estado del botón.
    qDebug() << "Boton Record pulsado: " << captura;

    if (captura) {
        // Si el botón está activado, iniciar la captura de imágenes.
        if (!timer) {
            // Crear un temporizador si no existe y conectarlo al slot `UpdateImage`.
            timer = new QTimer(this);
            connect(timer, &QTimer::timeout, this, &DeteccionCodigos::UpdateImage);
            timer->start(30); // Configurar el temporizador para actualizar cada 30 ms.
        }

        // Comenzar la captura de video desde la cámara.
        camera->startStopCapture(true);
    }
    else {
        // Si el botón está desactivado, detener la captura de imágenes.
        if (timer) {
            // Detener el temporizador, eliminarlo y liberar la memoria.
            timer->stop();
            delete timer;
            timer = nullptr;
        }

        // Detener la captura de video desde la cámara.
        camera->startStopCapture(false);

        // Limpiar el QLabel que muestra la imagen.
        ui.label->clear();
    }
}


/**
 * @brief Maneja el comportamiento del botón "Stop".
 *
 * Esta función detiene o reinicia la captura de imágenes desde la cámara,
 * dependiendo del estado del botón "Stop".
 *
 * @param captura Indica si el botón "Stop" está activado (`true`) o desactivado (`false`).
 */
void DeteccionCodigos::StropButton(bool captura) {
    // Mensaje de depuración para verificar el estado del botón.
    qDebug() << "Boton Stop pulsado: " << captura;

    // Alternar la captura de video según el estado del botón.
    // Si `captura` es `true`, detiene la captura; si es `false`, reinicia la captura.
    camera->startStopCapture(!captura);
}


/**
 * @brief Activa o desactiva el modo de decodificación de imágenes.
 *
 * Esta función cambia el modo actual de procesamiento de imágenes entre
 * "Decoded" (decodificado) y "Normal", dependiendo del estado del botón.
 *
 * @param captura Indica si el botón "Decode" está activado (`true`) o desactivado (`false`).
 */
void DeteccionCodigos::DecodeImagen(bool captura) {
    if (captura) {
        // Activar el modo de decodificación.
        qDebug() << "Decodificando imagen...";
        currentMode = Decoded;
    }
    else {
        // Restaurar el modo normal de visualización.
        currentMode = Normal;
    }
}


/**
 * @brief Activa o desactiva el modo de visualización de la máscara roja.
 *
 * Esta función cambia el modo actual de procesamiento de imágenes entre
 * "RedMask" (máscara roja) y "Normal", dependiendo del estado del botón.
 *
 * @param captura Indica si el botón "Red Mask" está activado (`true`) o desactivado (`false`).
 */
void DeteccionCodigos::ViewRedMask(bool captura) {
    if (captura) {
        // Activar el modo de máscara roja.
        qDebug() << "Mostrando mascara roja...";
        currentMode = RedMask;
    }
    else {
        // Restaurar el modo normal de visualización.
        currentMode = Normal;
    }
}


/**
 * @brief Activa o desactiva el modo de visualización de la máscara verde.
 *
 * Esta función cambia el modo actual de procesamiento de imágenes entre
 * "GreenMask" (máscara verde) y "Normal", dependiendo del estado del botón.
 *
 * @param captura Indica si el botón "Green Mask" está activado (`true`) o desactivado (`false`).
 */
void DeteccionCodigos::ViewGreenMask(bool captura) {
    if (captura) {
        // Activar el modo de máscara verde.
        qDebug() << "Mostrando mascara verde...";
        currentMode = GreenMask;
    }
    else {
        // Restaurar el modo normal de visualización.
        currentMode = Normal;
    }
}


/**
 * @brief Guarda la imagen decodificada como un archivo de imagen.
 *
 * Esta función permite al usuario guardar la imagen procesada (`imagenFinal`)
 * como un archivo de imagen en formato JPG, utilizando un cuadro de diálogo
 * para seleccionar la ubicación y el nombre del archivo.
 */
void DeteccionCodigos::SaveDecodedCode()
{
    // Mensaje de depuración para indicar que se está guardando el código decodificado.
    qDebug() << "Guardando codigo decodificado...";

    // Abrir un cuadro de diálogo para permitir al usuario seleccionar la ubicación y nombre del archivo.
    QString fileName = QFileDialog::getSaveFileName(this, tr("Guardar imagen"), "", tr("Archivos de imagen (*.jpg)"));

    // Verificar si el nombre del archivo no está vacío (es decir, si el usuario seleccionó un archivo).
    if (!fileName.isEmpty()) {
        // Guardar la imagen final en el archivo especificado por el usuario.
        imwrite(fileName.toStdString(), imagenFinal);
    }
}


/**
 * @brief Aplica un filtro de desenfoque gaussiano a la imagen.
 *
 * Esta función aplica un desenfoque gaussiano a la imagen proporcionada para
 * reducir el ruido y suavizar la imagen. El tamaño del filtro se puede ajustar
 * a través del parámetro `kernerSsize`.
 *
 * @param image La imagen original sobre la que se aplicará el desenfoque.
 * @param kernerSsize El tamaño del kernel para el filtro de desenfoque (debe ser un número impar).
 *
 * @return Mat La imagen procesada con el desenfoque gaussiano aplicado.
 */
Mat DeteccionCodigos::BlurImage(const Mat &image, uint8_t kernerSsize) {
    // Aplicar el filtro de desenfoque gaussiano con el tamaño de kernel especificado
    Mat blurImage;
    GaussianBlur(image, blurImage, Size(kernerSsize, kernerSsize), 0);
    return blurImage;
}


/**
 * @brief Convierte una imagen a escala de grises.
 *
 * Esta función toma una imagen en color (en formato BGR) y la convierte a una
 * imagen en escala de grises. La conversión es útil para simplificar el procesamiento
 * de imágenes en tareas como la detección de bordes o la segmentación.
 *
 * @param image La imagen original que se va a convertir (en formato BGR).
 *
 * @return Mat La imagen convertida a escala de grises.
 */
Mat DeteccionCodigos::convertGrayImage(const Mat &image) {
    // Convertir la imagen de BGR a escala de grises usando la función cvtColor
    Mat grayImage;
    cvtColor(image, grayImage, COLOR_BGR2GRAY);
    return grayImage;
}


/**
 * @brief Convierte una imagen al espacio de color HSV.
 *
 * Esta función toma una imagen en el formato BGR y la convierte a HSV (Hue, Saturation, Value).
 * El espacio de color HSV es frecuentemente utilizado en procesamiento de imágenes para tareas como
 * la segmentación de colores y la detección de objetos, ya que separa la información de tono (hue)
 * de la de intensidad (valor), lo que puede facilitar la detección de colores específicos.
 *
 * @param image La imagen original que se va a convertir (en formato BGR).
 *
 * @return Mat La imagen convertida al espacio de color HSV.
 */
Mat DeteccionCodigos::convertHSVImage(const Mat &image) {
    // Convertir la imagen de BGR a HSV usando la función cvtColor
    Mat hsvImage;
    cvtColor(image, hsvImage, COLOR_BGR2HSV);
    return hsvImage;
}


/**
 * @brief Genera una máscara para detectar el color rojo en una imagen en espacio HSV.
 *
 * Esta función utiliza los valores del espacio de color HSV para crear una máscara que detecte
 * los píxeles correspondientes al color rojo. Dado que el color rojo se encuentra en dos rangos
 * en el espacio HSV, se crean dos máscaras que se combinan para cubrir ambos rangos del color rojo.
 *
 * @param image La imagen en espacio HSV sobre la que se aplicará la máscara.
 *
 * @return Mat La máscara generada donde los píxeles correspondientes al color rojo son blancos
 *             (valor 255) y los demás son negros (valor 0).
 */
Mat DeteccionCodigos::getRedMask(const Mat &image) {
    // Crear dos máscaras separadas para los dos rangos de color rojo en el espacio HSV
    Mat mascaraRoja, mascaraRoja2;

    // Rango 1: Detectar rojo en el rango de tono [0, 10]
    inRange(image, Scalar(0, 50, 50), Scalar(10, 255, 255), mascaraRoja);

    // Rango 2: Detectar rojo en el rango de tono [150, 179]
    inRange(image, Scalar(150, 50, 50), Scalar(179, 255, 255), mascaraRoja2);

    // Combinar las dos máscaras utilizando la operación OR
    mascaraRoja = mascaraRoja | mascaraRoja2;

    // Retornar la máscara combinada
    return mascaraRoja;
}


/**
 * @brief Genera una máscara para detectar el color verde en una imagen en espacio HSV.
 *
 * Esta función utiliza los valores del espacio de color HSV para crear una máscara que detecte
 * los píxeles correspondientes al color verde. La máscara es generada usando un rango específico
 * de valores para el tono (Hue), la saturación (Saturation) y el valor (Value) en el espacio HSV.
 *
 * @param image La imagen en espacio HSV sobre la que se aplicará la máscara.
 *
 * @return Mat La máscara generada donde los píxeles correspondientes al color verde son blancos
 *             (valor 255) y los demás son negros (valor 0).
 */
Mat DeteccionCodigos::getGreenMask(const Mat &image) {
    // Crear la máscara para el color verde en el espacio HSV con un rango ajustado
    Mat mascaraVerde;

    // Rango del verde en el espacio HSV: H [30, 90], S [20, 255], V [20, 255]
    inRange(image, Scalar(30, 55, 55), Scalar(90, 255, 255), mascaraVerde);

    // Retornar la máscara generada
    return mascaraVerde;
}


/**
 * @brief Aplica una máscara a una imagen.
 *
 * Esta función toma una imagen y una máscara binaria, y aplica la máscara a la imagen.
 * Los píxeles de la imagen que corresponden a los valores no nulos de la máscara (generalmente 255)
 * se mantienen intactos, mientras que los píxeles correspondientes a los valores cero de la máscara
 * se establecen a cero en la imagen resultante.
 *
 * @param image La imagen original sobre la que se aplicará la máscara.
 * @param mask La máscara binaria que se aplicará sobre la imagen. Los píxeles con valor 255
 *             en la máscara se conservarán en la imagen final, mientras que los píxeles con valor 0
 *             se eliminarán (se establecerán en 0).
 *
 * @return Mat La imagen resultante con la máscara aplicada, donde los píxeles que no están en la máscara
 *             serán eliminados (negros) y los que están en la máscara se mantendrán intactos.
 */
Mat DeteccionCodigos::applyMaskToImage(const Mat &image, Mat mask) {
    // Crear una copia de la imagen original y aplicar la máscara sobre ella.
    Mat maskedImage;
    // La función copyTo copia los píxeles de la imagen original a 'maskedImage', pero solo donde la máscara tiene valor 255
    image.copyTo(maskedImage, mask);

    // Retornar la imagen con la máscara aplicada
    return maskedImage;
}


/**
 * @brief Aplica el filtro Sobel para detectar bordes en una imagen.
 *
 * Esta función utiliza el filtro Sobel en las direcciones X e Y para calcular el gradiente de la imagen.
 * Luego, calcula la magnitud del gradiente para detectar los bordes y aplica una umbralización binaria
 * para obtener una imagen binaria donde los bordes son visibles.
 *
 * @param image La imagen de entrada sobre la que se aplicará el filtro Sobel. Debe ser una imagen en escala de grises.
 * @param kernelSize El tamaño del kernel que se utilizará para el filtro Sobel. Un valor típico es 3 o 5.
 *
 * @return Mat La imagen resultante con los bordes detectados, en formato binario.
 */
Mat DeteccionCodigos::sobelFilter(const Mat &image, uint8_t kernelSize) {
    // Declaración de las imágenes intermedias para los resultados de los filtros Sobel en X y Y
    Mat img_sobel_x, img_sobel_y, img_sobel, filtered_image;

    // Aplicar el filtro Sobel en la dirección X (detecta bordes en la dirección horizontal)
    Sobel(image, img_sobel_x, CV_64F, 1, 0, kernelSize);

    // Aplicar el filtro Sobel en la dirección Y (detecta bordes en la dirección vertical)
    Sobel(image, img_sobel_y, CV_64F, 0, 1, kernelSize);

    // Calcular la magnitud del gradiente a partir de las imágenes resultantes de Sobel en X y Y
    magnitude(img_sobel_x, img_sobel_y, img_sobel);

    // Normalizar la imagen resultante para que los valores estén en el rango [0, 255]
    // Esto es necesario para poder trabajar con una imagen de tipo CV_8U (escala de grises en 8 bits)
    normalize(img_sobel, img_sobel, 0, 255, NORM_MINMAX, CV_8U);

    // Aplicar umbralización binaria para resaltar los bordes detectados
    // Los píxeles con un valor superior a 30 serán establecidos a 255 (blanco), el resto será 0 (negro)
    threshold(img_sobel, filtered_image, 30, 255, THRESH_BINARY);

    // Retornar la imagen con los bordes detectados
    return filtered_image;
}


/**
 * @brief Encuentra y filtra los contornos detectados en una imagen usando el filtro Sobel.
 *
 * Esta función aplica el filtro Sobel para detectar los bordes en la imagen, luego encuentra todos los contornos
 * en la imagen binarizada resultante. Posteriormente, filtra los contornos según su área y aspecto (proporción de
 * ancho a altura) para asegurarse de que solo se retengan los contornos de interés.
 *
 * @param image La imagen de entrada sobre la cual se detectarán los contornos. La imagen debe estar en formato
 *              de escala de grises (en el caso del filtro Sobel).
 *
 * @return std::vector<std::vector<Point>> Un vector de vectores de puntos que representan los contornos
 *         detectados y filtrados. Cada contorno es un vector de puntos (Point) que forman el contorno de un objeto.
 */
std::vector<std::vector<Point>> DeteccionCodigos::findFilteredContours(const Mat &image) {
    // Paso 1: Aplicar el filtro Sobel para detectar los bordes
    Mat sobelImage = sobelFilter(image, 11);

    // Paso 2: Encontrar los contornos en la imagen binarizada obtenida del filtro Sobel
    std::vector<std::vector<Point>> contours;
    std::vector<Vec4i> hierarchy;
    findContours(sobelImage, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // Paso 3: Filtrar los contornos según su área y su aspecto (relación entre ancho y alto)
    std::vector<std::vector<Point>> filteredContours;
    for (const auto &contour : contours) {
        // Calcular el área del contorno
        double area = contourArea(contour);

        // Calcular el rectángulo delimitador del contorno
        Rect boundingBox = boundingRect(contour);

        // Calcular el 1% del área total de la imagen (para establecer umbrales de área)
        double areaImage = image.rows * image.cols;
        double umbralBajoArea = 0.01 * areaImage; // 5% del área de la imagen
        double umbralAltoArea = 0.25 * areaImage; // 25% del área de la imagen

        // Calcular la relación de aspecto (aspect ratio) del rectángulo delimitador
        double aspectRatio = static_cast<double>( boundingBox.width ) / boundingBox.height;

        // Filtrar los contornos por área y relación de aspecto
        // El contorno debe tener un área dentro de un rango específico y una relación de aspecto entre 0.5 y 1.3
        if (area > umbralBajoArea && area < umbralAltoArea && aspectRatio > 0.5 && aspectRatio < 1.3) {
            filteredContours.push_back(contour);
        }
    }

    // Paso 4: Devolver los contornos filtrados
    return filteredContours;
}

/**
 * @brief Extrae información relevante de los contornos proporcionados.
 *
 * Esta función toma un conjunto de contornos detectados y extrae características geométricas importantes para
 * cada uno de ellos, como el área, el perímetro, las esquinas del contorno, el centro, las dimensiones (ancho y alto),
 * la relación de aspecto, y el ángulo de orientación.
 *
 * @param contours Un vector de vectores de puntos que representan los contornos de la imagen.
 *                 Cada contorno es una secuencia cerrada de puntos que forma un objeto detectado.
 *
 * @return std::vector<ContourInfo> Un vector de estructuras `ContourInfo` que contienen la información
 *         extraída de cada contorno. Cada estructura contiene los detalles geométricos de un contorno.
 */
std::vector<ContourInfo> DeteccionCodigos::extractContourInfo(const std::vector<std::vector<Point>> &contours) {
    // Paso 1: Declarar el vector que almacenará la información de cada contorno
    std::vector<ContourInfo> contour_info;

    // Paso 2: Iterar sobre cada contorno para extraer su información
    for (const auto &contour : contours) {
        ContourInfo info;

        // Paso 3: Calcular el área del contorno
        info.area = contourArea(contour);

        // Paso 4: Obtener el rectángulo delimitador del contorno (bounding box)
        Rect boundingBox = boundingRect(contour);

        // Paso 5: Calcular el perímetro del contorno (longitud de su frontera)
        info.perimeter = arcLength(contour, true);

        // Paso 6: Obtener el rectángulo rotado que ajusta el contorno más estrechamente
        RotatedRect rect = minAreaRect(contour);
        Point2f box[4];
        rect.points(box); // Obtener las coordenadas de los 4 vértices del rectángulo rotado

        // Paso 7: Almacenar las esquinas del rectángulo en la estructura de información del contorno
        for (int i = 0; i < 4; i++) {
            info.corners.push_back(Point(static_cast<int>( box[i].x ), static_cast<int>( box[i].y )));
        }

        // Paso 8: Calcular el centro del contorno como el centro del rectángulo delimitador
        info.center = Point2f(( boundingBox.x + boundingBox.width ) / 2.0,
                              ( boundingBox.y + boundingBox.height ) / 2.0);

        // Paso 9: Almacenar el ancho y el alto del rectángulo delimitador
        info.width = boundingBox.width;
        info.height = boundingBox.height;

        // Paso 10: Calcular la relación de aspecto (ancho/alto) del rectángulo y almacenarla
        info.aspect_ratio = ( boundingBox.height != 0 ) ? boundingBox.width / static_cast<float>( boundingBox.height ) : 0;

        // Paso 11: Almacenar el ángulo de rotación del rectángulo respecto a la horizontal
        info.angle = rect.angle;

        // Paso 12: Añadir la información del contorno a la lista de resultados
        contour_info.push_back(info);

        // Paso 13 (opcional): Mostrar la información del contorno en el log para depuración
        /*qDebug() << "Area: " << info.area
                 << " Perimetro: " << info.perimeter
                 << " Centro: " << info.center.x << ", " << info.center.y
                 << " Ancho: " << info.width
                 << " Alto: " << info.height
                 << " Relacion de aspecto: " << info.aspect_ratio
                 << " Angulo: " << info.angle;*/
    }

    // Paso 14: Devolver el vector con la información de todos los contornos procesados
    return contour_info;
}


/**
 * @brief Empareja contornos rojos con contornos verdes basándose en criterios geométricos y de similitud.
 *
 * Esta función toma dos conjuntos de contornos detectados (uno rojo y otro verde) y los empareja en pares
 * basándose en criterios como la proximidad entre sus centros, la diferencia en su orientación (ángulo),
 * y similitudes en el área y el perímetro. Solo se emparejan los contornos que cumplen con estos criterios.
 *
 * @param redContoursInfo Un vector de estructuras `ContourInfo` que contienen la información geométrica
 *                        de los contornos detectados en la máscara roja.
 * @param greenContoursInfo Un vector de estructuras `ContourInfo` que contienen la información geométrica
 *                          de los contornos detectados en la máscara verde.
 *
 * @return std::vector<std::pair<ContourInfo, ContourInfo>> Un vector de pares de contornos emparejados.
 *         Cada par contiene un contorno rojo y un contorno verde que se consideran coincidentes.
 */
std::vector<std::pair<ContourInfo, ContourInfo>> DeteccionCodigos::matchContours(
    const std::vector<ContourInfo> &redContoursInfo,
    const std::vector<ContourInfo> &greenContoursInfo) {

    // Paso 1: Declarar el vector que almacenará los pares de contornos emparejados
    std::vector<std::pair<ContourInfo, ContourInfo>> matches;

    // Paso 2: Crear conjuntos para llevar un seguimiento de los contornos ya emparejados
    std::set<const ContourInfo *> usedRedContours;
    std::set<const ContourInfo *> usedGreenContours;

    // Paso 3: Iterar sobre cada contorno rojo
    for (const auto &redContour : redContoursInfo) {
        // Verificar si este contorno rojo ya está emparejado
        if (usedRedContours.find(&redContour) != usedRedContours.end()) {
            continue; // Saltar este contorno si ya está emparejado
        }

        // Paso 4: Inicializar variables para encontrar el mejor match para el contorno rojo actual
        const ContourInfo *bestMatch = nullptr;               // Apuntador al mejor contorno verde encontrado
        double bestAngleMatch = std::numeric_limits<double>::infinity(); // Mejor diferencia de ángulo
        double bestScore = std::numeric_limits<double>::infinity();      // Mejor puntaje para desempatar

        // Paso 5: Iterar sobre cada contorno verde
        for (const auto &greenContour : greenContoursInfo) {
            // Verificar si este contorno verde ya está emparejado
            if (usedGreenContours.find(&greenContour) != usedGreenContours.end()) {
                continue; // Saltar este contorno si ya está emparejado
            }

            // Paso 6: Calcular la distancia entre los centros de los contornos
            double centerDistance = cv::norm(redContour.center - greenContour.center);

            // Paso 7: Descartar contornos que estén demasiado lejos (más de 3.5 veces el ancho del contorno rojo)
            if (centerDistance >  redContour.perimeter / 2.5) {
                continue; // Saltar este contorno verde por estar demasiado lejos
            }

            //// Paso 8: Descartar contornos que estén demasiado cerca (más de 2.5 veces el ancho del contorno rojo)
            if (centerDistance < redContour.perimeter / 3.5) {
                continue; // Saltar este contorno verde por estar demasiado cerca
            }

            // Paso 9: Calcular la diferencia de ángulo entre los contornos
            double angleDiff = std::abs(redContour.angle - greenContour.angle);

            // Paso 10: Calcular diferencias en área y perímetro para usar como criterios de desempate
            double areaDiff = std::abs(redContour.area - greenContour.area);
            double perimeterDiff = std::abs(redContour.perimeter - greenContour.perimeter);

            // Paso 11: Calcular un puntaje de desempate basado en las diferencias de área y perímetro
            double score = areaDiff + perimeterDiff;

            // Paso 12: Actualizar el mejor match si el ángulo es menor, o si el puntaje es menor en caso de empate
            if (angleDiff < bestAngleMatch || ( angleDiff == bestAngleMatch && score < bestScore )) {
                bestAngleMatch = angleDiff;
                bestScore = score;
                bestMatch = &greenContour; // Actualizar el mejor contorno verde encontrado
            }
        }

        // Paso 13: Si se encontró un match válido para el contorno rojo actual
        if (bestMatch) {
            // Añadir el par de contornos emparejados al vector de resultados
            matches.emplace_back(redContour, *bestMatch);

            // Marcar ambos contornos como emparejados
            usedRedContours.insert(&redContour);
            usedGreenContours.insert(bestMatch);
        }
    }

    // Paso 14: Información de depuración para verificar el número de contornos emparejados
    /*qDebug() << "Numero de contornos emparejados: " << matches.size();*/

    // Paso 15: Devolver el vector de pares de contornos emparejados
    return matches;
}


/**
 * @brief Extrae regiones de la imagen delimitadas por los contornos emparejados, alineando cada región para que la línea entre los contornos sea horizontal.
 *
 * Esta función toma un conjunto de contornos emparejados (rojo y verde) y recorta la región de la imagen original
 * que contiene ambos contornos. Además, rota la imagen para alinear los contornos horizontalmente y recorta la región resultante.
 *
 * @param matchedContours Un vector de pares de contornos emparejados (rojo y verde). Cada par contiene la información
 *                        geométrica de dos contornos que se han identificado como relacionados.
 * @param image La imagen original de la cual se extraerán las regiones delimitadas por los contornos.
 *
 * @return std::vector<Mat> Un vector de imágenes recortadas (Mat) que contienen las regiones extraídas de la imagen original.
 *         Cada imagen corresponde a una región delimitada por un par de contornos emparejados.
 */
std::vector<Mat> DeteccionCodigos::cutBoundingBox(const std::vector<pair<ContourInfo, ContourInfo>> &matchedContours, const Mat &image) {
    // Paso 1: Inicializar un vector para almacenar las imágenes recortadas
    std::vector<Mat> extractedImages;

    // Paso 2: Iterar sobre cada par de contornos emparejados
    for (const auto &match : matchedContours) {
        const ContourInfo &redContour = match.first;
        const ContourInfo &greenContour = match.second;

        // Paso 3: Obtener los puntos de los contornos rojo y verde
        std::vector<Point> redPoints = redContour.corners;
        std::vector<Point> greenPoints = greenContour.corners;

        // Paso 4: Combinar los puntos de ambos contornos en un solo vector
        std::vector<Point> allPoints;
        allPoints.insert(allPoints.end(), redPoints.begin(), redPoints.end());
        allPoints.insert(allPoints.end(), greenPoints.begin(), greenPoints.end());

        // Paso 5: Calcular el rectángulo delimitador (bounding box) que contenga todos los puntos
        Rect boundingBox = boundingRect(allPoints);

        // Paso 6: Calcular el centro del rectángulo delimitador
        Point boundingBoxCenter = Point(boundingBox.x + boundingBox.width / 2, boundingBox.y + boundingBox.height / 2);

        // Paso 7: Calcular el ángulo entre los centros de los contornos rojo y verde
        double x0 = redContour.center.x;
        double y0 = redContour.center.y;
        double x1 = greenContour.center.x;
        double y1 = greenContour.center.y;
        double angle = atan2(y1 - y0, x1 - x0) * 180 / CV_PI;

        // Paso 8: Rotar la imagen para que la línea entre los contornos sea horizontal
        Mat M = getRotationMatrix2D(boundingBoxCenter, angle, 1);
        Mat rotatedImage;
        warpAffine(image, rotatedImage, M, image.size());

        // Paso 9: Rotar los puntos de los contornos usando la matriz de transformación
        std::vector<Point> transformedPoints;
        for (const Point &pt : allPoints) {
            double xNew = M.at<double>(0, 0) * pt.x + M.at<double>(0, 1) * pt.y + M.at<double>(0, 2);
            double yNew = M.at<double>(1, 0) * pt.x + M.at<double>(1, 1) * pt.y + M.at<double>(1, 2);
            transformedPoints.emplace_back(cvRound(xNew), cvRound(yNew));
        }

        // Paso 10: Calcular la nueva bounding box después de la rotación
        Rect transformedBoundingBox = boundingRect(transformedPoints);

        // Paso 11: Verificar si la bounding box transformada está dentro de los límites de la imagen
        if (transformedBoundingBox.x >= 0 && transformedBoundingBox.y >= 0 &&
            transformedBoundingBox.x + transformedBoundingBox.width <= rotatedImage.cols &&
            transformedBoundingBox.y + transformedBoundingBox.height <= rotatedImage.rows) {
            // Paso 12: Recortar la región de la imagen rotada
            Mat extractedImage = rotatedImage(transformedBoundingBox);
            extractedImages.push_back(extractedImage);
        }
        else {
            std::cout << "La bounding box transformada está fuera de los limites." << std::endl;
        }
    }

    // Paso 13: Devolver el vector de imágenes recortadas
    return extractedImages;
}


/**
 * @brief Aplica un umbral adaptativo a la imagen y realiza operaciones morfológicas para suavizar los bordes.
 *
 * Esta función convierte una imagen en escala de grises en una binaria utilizando un umbral adaptativo basado en
 * el método de medias ponderadas (GAUSSIAN). Posteriormente, aplica operaciones morfológicas como cierre y erosión
 * para eliminar imperfecciones en los bordes y suavizar el resultado.
 *
 * @param image La imagen en escala de grises sobre la que se aplicará el umbral adaptativo y las operaciones morfológicas.
 *              Se espera que esta imagen sea de tipo `CV_8UC1` (1 canal, escala de grises).
 * @param threshold Un valor de ajuste para el cálculo del umbral adaptativo. Este parámetro influye en la segmentación
 *                  de la imagen, permitiendo afinar los detalles capturados en la binarización.
 *
 * @return Mat La imagen resultante después de aplicar el umbral adaptativo y las operaciones de cierre y erosión.
 *             Es una imagen binaria donde los píxeles son 0 (negro) o 255 (blanco).
 */
Mat DeteccionCodigos::thresholdImage(const Mat &image, int threshold) {
    // Paso 1: Aplicar umbral adaptativo con el método GAUSSIAN
    Mat imageThresholdGaussian;
    adaptiveThreshold(image, imageThresholdGaussian, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 11, threshold);

    // Paso 2: Crear un kernel para las operaciones morfológicas
    Mat kernel = Mat::ones(Size(3, 3), CV_8U);

    // Paso 3: Aplicar la operación morfológica de cierre para unir regiones desconectadas
    Mat closeImage;
    morphologyEx(imageThresholdGaussian, closeImage, MORPH_CLOSE, kernel);

    // Paso 4: Aplicar erosión para suavizar los bordes y reducir pequeñas imperfecciones
    Mat erodeImage;
    erode(closeImage, erodeImage, kernel, Point(-1, -1), 1);

    // Paso 5: Devolver la imagen binaria resultante
    return erodeImage;
}


/**
 * @brief Clasifica los contornos en cuadrados y rectángulos según su relación de aspecto y área relativa.
 *
 * Esta función analiza un conjunto de contornos detectados y los clasifica en dos categorías:
 * contornos cuadrados y contornos rectangulares. La clasificación se basa en la relación de aspecto (ancho/alto)
 * y el área relativa del contorno respecto al tamaño de la imagen.
 *
 * @param contours Un vector de vectores de puntos que representan los contornos detectados.
 *                 Cada contorno es una secuencia cerrada de puntos que forma una figura.
 * @param imageShape Un objeto `Size` que especifica las dimensiones de la imagen original
 *                   (ancho y alto) sobre la que se detectaron los contornos.
 *
 * @return pair<std::vector<std::vector<Point>>, std::vector<std::vector<Point>>>
 *         Un par de vectores:
 *         - El primer elemento contiene los contornos clasificados como cuadrados.
 *         - El segundo elemento contiene los contornos clasificados como rectángulos.
 */
pair<std::vector<std::vector<Point>>, std::vector<std::vector<Point>>>
DeteccionCodigos::classifyContours(const std::vector<std::vector<Point>> &contours, const Size &imageShape) {
    // Paso 1: Declarar vectores para almacenar contornos cuadrados y rectangulares
    std::vector<std::vector<Point>> squareContours;
    std::vector<std::vector<Point>> rectangularContours;

    // Paso 2: Iterar sobre los contornos para clasificarlos
    for (const auto &contour : contours) {
        // Calcular el área del contorno
        double area = contourArea(contour);

        // Calcular la proporción del área del contorno con respecto al área de la imagen
        double areaRatio = area / ( imageShape.width * imageShape.height );

        // Obtener el rectángulo delimitador del contorno
        Rect boundingBox = boundingRect(contour);

        // Calcular la relación de aspecto (ancho/alto) del rectángulo delimitador
        double aspectRatio = static_cast<double>( boundingBox.width ) / boundingBox.height;

        // Paso 3: Clasificar el contorno según la relación de aspecto y el área relativa
        if (0.5 <= aspectRatio && aspectRatio <= 1.5 && areaRatio > 0.06) {
            // Contornos con relación de aspecto cercana a 1 y área significativa: cuadrados
            squareContours.push_back(contour);
        }
        else {
            // Contornos restantes: rectángulos
            rectangularContours.push_back(contour);
        }
    }

    // Paso 4: Devolver los contornos clasificados como un par
    return { squareContours, rectangularContours };
}



/**
 * @brief Filtra contornos que estén completamente dentro de otros contornos más grandes.
 *
 * Esta función elimina los contornos que están completamente contenidos dentro de otros
 * contornos más grandes. Esto es útil para evitar considerar contornos secundarios
 * o ruidos dentro de áreas cerradas que ya están representadas por un contorno más grande.
 *
 * @param contours Un vector de vectores de puntos que representan los contornos detectados.
 *                 Cada contorno es una secuencia cerrada de puntos que forma una figura.
 *
 * @return std::vector<std::vector<Point>>
 *         Un vector de contornos filtrados donde se han eliminado los contornos que están
 *         completamente contenidos dentro de otros.
 */
std::vector<std::vector<Point>>
DeteccionCodigos::filterInsideContours(const std::vector<std::vector<Point>> &contours) {
    // Paso 1: Declarar un vector para almacenar los contornos filtrados
    std::vector<std::vector<Point>> filteredContours;

    // Paso 2: Iterar sobre cada contorno en la lista original
    for (size_t i = 0; i < contours.size(); ++i) {
        const std::vector<Point> &contour = contours[i]; // Contorno actual
        const std::vector<Point> *parentContour = nullptr; // Contorno padre potencial

        // Paso 3: Comprobar si el contorno actual está dentro de otro contorno
        for (size_t j = 0; j < contours.size(); ++j) {
            if (i == j)
                continue; // Saltar la comparación del contorno consigo mismo

            const std::vector<Point> &otherContour = contours[j];
            bool isInside = true;

            // Paso 4: Verificar si todos los puntos del contorno actual están dentro del otro contorno
            for (const Point &pt : contour) {
                if (pointPolygonTest(otherContour, pt, false) <= 0) {
                    // Si algún punto no está dentro, no es un contorno interno
                    isInside = false;
                    break;
                }
            }

            // Paso 5: Actualizar el contorno padre si el contorno actual está contenido
            if (isInside) {
                if (parentContour == nullptr ||
                    contourArea(otherContour) > contourArea(*parentContour)) {
                    parentContour = &otherContour; // Actualizar al contorno más grande
                }
            }
        }

        // Paso 6: Si no hay contorno padre, agregar el contorno actual a los resultados
        if (parentContour == nullptr) {
            filteredContours.push_back(contour);
        }
    }

    // Paso 7: Devolver el vector de contornos filtrados
    return filteredContours;
}


/**
 * @brief Obtiene y filtra los contornos de una imagen binarizada.
 *
 * Esta función detecta contornos en una imagen umbralizada y aplica varios criterios de filtrado,
 * como el área, la influencia relativa al tamaño de la imagen, y la posición dentro de los límites
 * de la imagen. También clasifica los contornos detectados en cuadrados y rectángulos, y elimina
 * aquellos que están completamente contenidos dentro de otros más grandes.
 *
 * @param thresholdedImage Imagen binarizada en la que se buscarán los contornos.
 * @param image Imagen original utilizada para calcular el área relativa y validar los límites.
 *
 * @return std::vector<std::vector<Point>>
 *         Un vector de contornos filtrados que cumplen con los criterios de tamaño, forma, y posición.
 */
std::vector<std::vector<Point>>
DeteccionCodigos::getContours(const Mat &thresholdedImage, const Mat &image) {
    // Paso 1: Declarar los vectores para almacenar los contornos y la jerarquía
    std::vector<std::vector<Point>> contours;
    std::vector<Vec4i> hierarchy;

    // Paso 2: Detectar los contornos en la imagen umbralizada
    findContours(thresholdedImage, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

    // Paso 3: Declarar un vector para almacenar los contornos que pasen los filtros iniciales
    std::vector<std::vector<Point>> filteredContours;

    // Paso 4: Calcular el área de la imagen y establecer el umbral mínimo de influencia
    double imageArea = image.rows * image.cols;
    double minInfluence = 0.01;

    // Paso 5: Filtrar contornos según el área, la influencia, y su proximidad al borde
    for (const auto &contour : contours) {
        double area = contourArea(contour); // Calcular el área del contorno
        Rect boundingBox = boundingRect(contour); // Obtener la bounding box del contorno
        double influence = area / imageArea; // Calcular la influencia relativa del contorno

        // Aplicar criterios de filtrado
        if (area >= 200 && area <= 25000 && influence >= minInfluence) {
            // Ignorar contornos cercanos al borde
            if (boundingBox.x > 10 && boundingBox.y > 10 &&
                boundingBox.x + boundingBox.width < image.cols - 10 &&
                boundingBox.y + boundingBox.height < image.rows - 10) {
                filteredContours.push_back(contour); // Agregar el contorno filtrado
            }
        }
    }

    // Paso 6: Clasificar los contornos filtrados en cuadrados y rectángulos
    auto [squareContours, rectangularContours] = classifyContours(filteredContours, image.size());

    // Paso 7: Filtrar contornos que están completamente contenidos dentro de otros
    filteredContours = filterInsideContours(rectangularContours);

    // Paso 8: Devolver el conjunto final de contornos filtrados
    return filteredContours;
}


/**
 * @brief Separa los contornos en segmentos horizontales según su posición en la imagen.
 *
 * Esta función divide los contornos en cuatro segmentos horizontales, basándose en la posición
 * del centro del contorno dentro de la anchura de la imagen. Cada segmento representa una
 * división igual del ancho total de la imagen.
 *
 * @param contours Un vector de contornos representados como vectores de puntos.
 *                 Cada contorno es una secuencia de puntos que define un objeto detectado.
 * @param imageWidth La anchura de la imagen, utilizada para calcular los límites de cada segmento.
 *
 * @return std::vector<std::vector<std::vector<Point>>>
 *         Un vector que contiene cuatro vectores, donde cada uno corresponde a un segmento
 *         horizontal de la imagen y almacena los contornos pertenecientes a dicho segmento.
 */
std::vector<std::vector<std::vector<Point>>>
DeteccionCodigos::separateContoursBySegments(const std::vector<std::vector<Point>> &contours, int imageWidth) {
    // Paso 1: Crear un vector de 4 segmentos para almacenar los contornos
    std::vector<std::vector<std::vector<Point>>> segments(4);

    // Paso 2: Iterar sobre cada contorno para clasificarlo en un segmento
    for (const auto &contour : contours) {
        // Paso 3: Calcular la bounding box del contorno
        Rect boundingBox = boundingRect(contour);

        // Paso 4: Calcular la posición del centro en el eje X
        int centerX = boundingBox.x + boundingBox.width / 2;

        // Paso 5: Determinar a qué segmento pertenece el contorno
        int segment = centerX / ( imageWidth / 4 );

        // Paso 6: Asegurarse de que el segmento sea válido y agregar el contorno
        if (segment >= 0 && segment < 4) {
            segments[segment].push_back(contour);
        }
    }

    // Paso 7: Devolver los contornos clasificados por segmentos
    return segments;
}

/**
 * @brief Ordena los contornos dentro de cada segmento horizontal.
 *
 * Esta función organiza los contornos dentro de cada segmento en función de su orientación y
 * la posición de su rectángulo delimitador (bounding box). Si hay dos contornos en un segmento,
 * la función los ordena según la coordenada X o Y dependiendo de si los contornos son más anchos
 * o más altos. Si hay un solo contorno en el segmento, no se realiza ningún orden.
 *
 * @param segments Un vector de vectores de contornos, donde cada subvector representa un segmento
 *                 horizontal en la imagen. Cada contorno es un vector de puntos que define un objeto detectado.
 *
 * @return std::vector<std::vector<std::vector<Point>>> Un vector que contiene los segmentos ordenados,
 *         cada uno con los contornos ordenados por su coordenada X o Y, dependiendo de su forma.
 */
std::vector<std::vector<std::vector<Point>>> DeteccionCodigos::orderContours(
    const std::vector<std::vector<std::vector<Point>>> &segments) {

    // Paso 1: Crear el vector que almacenará los segmentos ordenados
    std::vector<std::vector<std::vector<Point>>> orderedSegments;

    // Paso 2: Iterar sobre cada segmento para ordenarlo si es necesario
    for (const auto &segment : segments) {
        if (segment.size() <= 1) {
            // Si el segmento tiene 0 o 1 contorno, no se realiza ordenación
            orderedSegments.push_back(segment);
        }
        else if (segment.size() == 2) {
            // Si el segmento tiene exactamente 2 contornos
            Rect boundingBox1 = boundingRect(segment[0]);
            Rect boundingBox2 = boundingRect(segment[1]);

            // Determinar si los contornos son más anchos (wide) que altos (no wide)
            bool isWide1 = boundingBox1.width > boundingBox1.height;
            bool isWide2 = boundingBox2.width > boundingBox2.height;

            // Copiar el segmento para ordenarlo
            std::vector<std::vector<Point>> sortedSegment = segment;

            if (isWide1 && isWide2) {
                // Si ambos contornos son anchos, ordenar por la coordenada Y de su rectángulo delimitador
                sort(sortedSegment.begin(), sortedSegment.end(),
                     [](const std::vector<Point> &a, const std::vector<Point> &b) {
                    return boundingRect(a).y < boundingRect(b).y;
                });
            }
            else if (!isWide1 && !isWide2) {
                // Si ambos contornos no son anchos, ordenar por la coordenada X de su rectángulo delimitador
                sort(sortedSegment.begin(), sortedSegment.end(),
                     [](const std::vector<Point> &a, const std::vector<Point> &b) {
                    return boundingRect(a).x < boundingRect(b).x;
                });
            }

            // Añadir el segmento ordenado a la lista
            orderedSegments.push_back(sortedSegment);
        }
    }

    // Paso 3: Devolver los segmentos ordenados
    return orderedSegments;
}


/**
 * @brief Calcula la relación entre el área de cada contorno y el área de una cuarta parte de la imagen.
 *
 * Esta función toma los contornos detectados en una imagen y calcula la relación entre el área de cada contorno
 * y el área de una cuarta parte de la imagen. Esta relación es útil para determinar qué tan grande es un contorno
 * con respecto al tamaño total de la imagen, lo que puede ayudar a filtrar contornos pequeños o grandes según se desee.
 *
 * @param contours Un vector de vectores de puntos que representan los contornos detectados en la imagen.
 *                 Cada contorno es una secuencia cerrada de puntos que forma un objeto detectado.
 * @param image La imagen en la que se detectaron los contornos. Se usa para calcular el área total de la imagen.
 *
 * @return std::vector<double> Un vector que contiene la relación del área de cada contorno con respecto
 *         al área de una cuarta parte de la imagen. El valor de cada elemento es un número decimal que
 *         representa esta relación.
 */
std::vector<double> DeteccionCodigos::getAreaRatio(const std::vector<std::vector<Point>> &contours, const Mat &image) {
    // Paso 1: Calcular el área total de la imagen dividida por 4
    double imageArea = ( image.rows * image.cols ) / 4.0;

    // Paso 2: Crear el vector para almacenar las relaciones de área
    std::vector<double> areaRatios;

    // Paso 3: Iterar sobre cada contorno para calcular su relación de área
    for (const auto &contour : contours) {
        // Calcular el área del contorno
        double area = contourArea(contour);

        // Calcular la relación entre el área del contorno y el área de la imagen
        double areaRatio = area / imageArea;

        // Almacenar la relación de área calculada en el vector
        areaRatios.push_back(areaRatio);
    }

    // Paso 4: Devolver el vector con las relaciones de área
    return areaRatios;
}

/**
 * @brief Obtiene información detallada sobre los segmentos de contornos, incluyendo el número de contornos,
 *        sus orientaciones, relaciones de área y otras métricas relevantes.
 *
 * Esta función toma los segmentos de contornos previamente ordenados y calcula varios detalles sobre cada segmento.
 * Entre las métricas calculadas están el número de contornos por segmento, la orientación (horizontal o vertical)
 * de cada contorno, la relación del área de cada contorno respecto al área de una cuarta parte de la imagen,
 * y la relación entre áreas de los contornos si un segmento tiene exactamente dos contornos.
 *
 * @param orderedSegments Un vector de segmentos, donde cada segmento es un conjunto de contornos detectados
 *                        que están organizados en 4 grupos en función de su posición en la imagen.
 * @param image La imagen original, que se utiliza para calcular la relación de áreas de los contornos.
 *
 * @return std::vector<SegmentInfo> Un vector de estructuras `SegmentInfo` donde cada estructura contiene
 *         la información detallada sobre un segmento de contornos. Cada estructura incluye:
 *         - El número de contornos en el segmento.
 *         - La orientación de cada contorno (horizontal o vertical).
 *         - Las relaciones de área de los contornos.
 *         - La relación entre áreas si hay exactamente 2 contornos en el segmento.
 */
std::vector<SegmentInfo> DeteccionCodigos::getSegmentInfo(const std::vector<std::vector<std::vector<Point>>> &orderedSegments, const Mat &image) {
    // Paso 1: Crear un vector para almacenar la información de los segmentos
    std::vector<SegmentInfo> segmentInfoList;

    // Paso 2: Iterar sobre cada segmento
    for (const auto &segment : orderedSegments) {
        SegmentInfo info;
        info.numContours = segment.size(); // Paso 2.1: Establecer el número de contornos en el segmento

        // Paso 3: Calcular las orientaciones de los contornos en el segmento
        for (const auto &contour : segment) {
            Rect boundingBox = boundingRect(contour); // Paso 3.1: Obtener el rectángulo delimitador del contorno
            string orientation = ( boundingBox.width > boundingBox.height ) ? "horizontal" : "vertical"; // Paso 3.2: Determinar la orientación
            info.orientations.push_back(orientation); // Paso 3.3: Almacenar la orientación
        }

        // Paso 4: Calcular las relaciones de área de los contornos
        info.areaRatios = getAreaRatio(segment, image); // Paso 4.1: Llamar a `getAreaRatio` para obtener las relaciones de área

        // Paso 5: Calcular la relación de áreas si hay exactamente 2 contornos
        if (info.numContours == 2) {
            double area1 = contourArea(segment[0]); // Paso 5.1: Calcular el área del primer contorno
            double area2 = contourArea(segment[1]); // Paso 5.2: Calcular el área del segundo contorno
            info.areaRatioRelation = ( area1 / area2 ); // Paso 5.3: Calcular la relación entre las áreas
        }
        else {
            info.areaRatioRelation = -1; // Paso 5.4: Usar -1 para indicar que no es aplicable si no hay exactamente 2 contornos
        }

        // Paso 6: Añadir la información del segmento a la lista
        segmentInfoList.push_back(info);
    }

    // Paso 7: Devolver el vector con la información de todos los segmentos
    return segmentInfoList;
}


/**
 * @brief Decodifica el número representado por los segmentos de contornos en la imagen.
 *
 * Esta función interpreta la información de cada segmento de contornos (número de contornos, orientaciones,
 * relación de áreas) para decodificar un número en formato de cadena de 4 dígitos. La decodificación se basa en una lógica
 * definida por las características de los contornos (por ejemplo, número de contornos, orientación, y relación de áreas).
 *
 * @param segmentInfo Un vector de objetos `SegmentInfo` que contienen información sobre los segmentos,
 *                    tales como el número de contornos, sus orientaciones, relaciones de área, etc.
 *
 * @return std::string Un número decodificado representado como una cadena de caracteres. Si la decodificación no es
 *                     posible en un segmento, se usa el carácter 'X'. Si no hay suficiente información, también se devuelve 'X'.
 */
std::string DeteccionCodigos::decodeNumber(const std::vector<SegmentInfo> &segmentInfo) {
    std::string segmentNumber;

    // Paso 1: Iterar sobre los 4 segmentos
    for (int i = 0; i < 4; ++i) {
        // Paso 2: Validar si el segmento tiene información
        if (i >= segmentInfo.size()) {
            segmentNumber += 'X';  // Si no hay información en el segmento, se asigna 'X'
            continue;
        }

        const SegmentInfo &info = segmentInfo[i];
        size_t numContours = info.numContours;  // Paso 3: Número de contornos en el segmento
        const std::vector<std::string> &orientations = info.orientations;  // Paso 4: Orientaciones de los contornos
        const std::vector<double> &areaRatios = info.areaRatios;  // Paso 5: Relaciones de área de los contornos
        double areaRatioRelation = info.areaRatioRelation;  // Paso 6: Relación entre las áreas si hay exactamente 2 contornos

        // Paso 7: Decodificar el número basado en el número de contornos
        if (numContours == 0) {
            segmentNumber += '0';  // Si no hay contornos, se asigna el dígito '0'
        }
        else if (numContours == 1) {
            // Paso 8: Si hay un solo contorno
            if (orientations[0] == "horizontal") {
                segmentNumber += '8';  // Si es horizontal, asigna el dígito '8'
            }
            else {
                if (areaRatios[0] < 0.15) {
                    segmentNumber += '1';  // Si el área es pequeña, asigna el dígito '1'
                }
                else {
                    segmentNumber += '5';  // Si el área es mayor, asigna el dígito '5'
                }
            }
        }
        else if (numContours == 2) {
            // Paso 9: Si hay dos contornos
            if (orientations[0] == "horizontal") {
                if (areaRatioRelation > 1.2) {
                    segmentNumber += '7';  // Si la relación de áreas es grande, asigna el dígito '7'
                }
                else if (areaRatioRelation < 0.8) {
                    segmentNumber += '9';  // Si la relación de áreas es pequeña, asigna el dígito '9'
                }
                else {
                    segmentNumber += '3';  // Si la relación de áreas está en el medio, asigna el dígito '3'
                }
            }
            else {
                if (areaRatioRelation > 1.2) {
                    segmentNumber += '6';  // Si la relación de áreas es grande, asigna el dígito '6'
                }
                else if (areaRatioRelation < 0.8) {
                    segmentNumber += '4';  // Si la relación de áreas es pequeña, asigna el dígito '4'
                }
                else {
                    segmentNumber += '2';  // Si la relación de áreas está en el medio, asigna el dígito '2'
                }
            }
        }
        else {
            segmentNumber += 'X';  // Si no hay un número de contornos esperado, asigna 'X'
        }
    }

    // Paso 10: Retornar el número decodificado como cadena
    return segmentNumber;
}



/**
 * @brief Muestra las imágenes de los códigos emparejados, destacando los contornos rojos y verdes con sus respectivos códigos decodificados.
 *
 * Esta función lleva a cabo un proceso de segmentación y decodificación de los contornos rojos y verdes en la imagen.
 * Se extraen los contornos de ambas máscaras, se emparejan, y luego se recortan las áreas correspondientes.
 * Después, la función procesa cada imagen recortada para decodificar el número representado por los contornos.
 * Finalmente, se dibujan los cuadros delimitadores (bounding boxes) y los números decodificados en la imagen original.
 *
 * @param imagen La imagen original sobre la cual se procesan los contornos.
 *               Esta imagen es utilizada para realizar la segmentación y para mostrar los resultados finales.
 *
 * @return Mat La imagen con los cuadros delimitadores de los contornos emparejados y los códigos decodificados
 *             visualizados sobre ella.
 */
Mat DeteccionCodigos::getSegmentedImage(const Mat &imagen) {
    // Paso 1: Crear una copia de la imagen original para modificarla sin alterar la original
    Mat copiaImagen = imagen.clone();

    /// ETAPA SEGMENTACIÓN ///

    // Paso 2: Aplicar un filtro de desenfoque para reducir el ruido
    Mat blurImage = BlurImage(imagen, 7);

    // Paso 3: Convertir la imagen a espacio de color HSV para una mejor segmentación
    Mat hsvImage = convertHSVImage(blurImage);

    // Paso 4: Convertir la imagen a escala de grises para facilitar el procesamiento
    Mat grayImage = convertGrayImage(blurImage);

    // Paso 5: Obtener las máscaras para los colores rojo y verde en la imagen
    Mat redMask = getRedMask(hsvImage);
    Mat greenMask = getGreenMask(hsvImage);

    // Paso 6: Aplicar las máscaras sobre la imagen original para aislar las áreas rojas y verdes
    redMask = applyMaskToImage(grayImage, redMask);
    greenMask = applyMaskToImage(grayImage, greenMask);

    // Paso 7: Encontrar los contornos filtrados en las imágenes con las máscaras aplicadas
    std::vector<std::vector<Point>> redContours = findFilteredContours(redMask);
    std::vector<std::vector<Point>> greenContours = findFilteredContours(greenMask);

    // Paso 8: Extraer la información relevante de los contornos encontrados
    std::vector<ContourInfo> redContoursInfo = extractContourInfo(redContours);
    std::vector<ContourInfo> greenContoursInfo = extractContourInfo(greenContours);

    // Paso 9: Emparejar los contornos rojos y verdes
    std::vector<std::pair<ContourInfo, ContourInfo>> matchedContours = matchContours(redContoursInfo, greenContoursInfo);

    // Paso 10: Recortar las regiones de interés de la imagen (bounding boxes) de los contornos emparejados
    std::vector<Mat> extractedImages = cutBoundingBox(matchedContours, imagen);

    /// ETAPA DECODIFICACIÓN ///

    // Paso 11: Procesar las imágenes recortadas para decodificar los códigos
    std::vector<std::string> decodedCodes;
    for (size_t i = 0; i < extractedImages.size(); ++i) {
        // Paso 12: Convertir cada imagen recortada a escala de grises
        extractedImages[i] = convertGrayImage(extractedImages[i]);

        // Paso 13: Aplicar un filtro gaussiano para reducir el ruido en las imágenes recortadas
        extractedImages[i] = BlurImage(extractedImages[i], 11);

        // Paso 14: Aplicar un umbral para binarizar la imagen y resaltar los contornos
        Mat thresholded = thresholdImage(extractedImages[i], 2);

        // Paso 15: Obtener los contornos de la imagen binarizada
        std::vector<std::vector<Point>> contours = getContours(thresholded, extractedImages[i]);

        // Paso 16: Separar los contornos en segmentos según su posición en la imagen
        std::vector<std::vector<std::vector<Point>>> segments = separateContoursBySegments(contours, extractedImages[i].cols);

        // Paso 17: Ordenar los contornos dentro de cada segmento para facilitar la decodificación
        std::vector<std::vector<std::vector<Point>>> orderedSegments = orderContours(segments);

        // Paso 18: Obtener información detallada sobre los segmentos de contornos
        std::vector<SegmentInfo> segmentInfo = getSegmentInfo(orderedSegments, extractedImages[i]);

        // Paso 19: Decodificar el número representado por los contornos
        std::string segmentNumber = decodeNumber(segmentInfo);

        // Paso 20: Almacenar el número decodificado
        decodedCodes.push_back(segmentNumber);
    }

    // Paso 21: Dibujar los cuadros delimitadores y los códigos decodificados sobre la imagen original
    for (size_t i = 0; i < matchedContours.size(); ++i) {
        const ContourInfo &redContour = matchedContours[i].first;
        const ContourInfo &greenContour = matchedContours[i].second;

        // Paso 22: Obtener los puntos de los contornos rojo y verde
        std::vector<Point> redPoints = redContour.corners;
        std::vector<Point> greenPoints = greenContour.corners;

        // Paso 23: Combinar los puntos de los dos contornos en un solo conjunto
        std::vector<Point> allPoints;
        allPoints.insert(allPoints.end(), redPoints.begin(), redPoints.end());
        allPoints.insert(allPoints.end(), greenPoints.begin(), greenPoints.end());

        // Paso 24: Calcular la bounding box que contiene todos los puntos combinados
        Rect boundingBox = boundingRect(allPoints);

        // Paso 25: Dibujar la bounding box sobre la imagen original
        rectangle(copiaImagen, boundingBox, Scalar(0, 255, 0), 2);

        // Paso 26: Dibujar el código decodificado cerca de la bounding box
        std::string numero = i < decodedCodes.size() ? decodedCodes[i] : "X"; // Si no hay número, usar "X"
        putText(copiaImagen, numero, Point(boundingBox.x, boundingBox.y - 10), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2);
    }

    // Paso 27: Retornar la imagen con los resultados visualizados
    return copiaImagen;
}



