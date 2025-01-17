#include "DeteccionCodigos.h"

/**
 * @brief Constructor de la clase DeteccionCodigos.
 *
 * Este constructor inicializa la interfaz de usuario y configura las conexiones
 * entre los botones de la interfaz gr�fica y sus correspondientes slots. Tambi�n
 * inicializa la adquisici�n de video desde la c�mara.
 *
 * @param parent Puntero al widget padre. Por defecto es nullptr.
 */
DeteccionCodigos::DeteccionCodigos(QWidget *parent)
    : QMainWindow(parent)
{
    // Configuraci�n inicial de la interfaz gr�fica de usuario.
    ui.setupUi(this);

    // Inicializaci�n del objeto de adquisici�n de video con la c�mara predeterminada (ID 0).
    //camera = new CVideoAcquisition(0);
    // Tambi�n se podr�a usar un flujo RTSP en lugar de la c�mara local.
    // camera = new CVideoAcquisition("rtsp://admin:admin@192.168.1.101:8554/profile0");
    // Flujo RTSP del ESP32
    camera = new CVideoAcquisition("rtsp://10.119.60.99:8554/mjpeg/1");

    qDebug() << "Conectando con la camara...";

    // Configuraci�n de botones de la interfaz como botones de tipo "checkable" (pueden mantenerse pulsados).
    ui.btnStop->setCheckable(true);
    ui.btnRecord->setCheckable(true);
    ui.btnDecodeImage->setCheckable(true);
    ui.btnRedMask->setCheckable(true);
    ui.btnGreenMask->setCheckable(true);

    // Conexi�n de se�ales de los botones a sus respectivos slots.
    connect(ui.btnRecord, SIGNAL(clicked(bool)), this, SLOT(RecordButton(bool)));
    connect(ui.btnStop, SIGNAL(clicked(bool)), this, SLOT(StropButton(bool)));
    connect(ui.btnDecodeImage, SIGNAL(clicked(bool)), this, SLOT(DecodeImagen(bool)));
    connect(ui.btnRedMask, SIGNAL(clicked(bool)), this, SLOT(ViewRedMask(bool)));
    connect(ui.btnGreenMask, SIGNAL(clicked(bool)), this, SLOT(ViewGreenMask(bool)));
    connect(ui.btnSaveImage, SIGNAL(clicked()), this, SLOT(SaveDecodedCode()));

    // Nota: Los botones 'Record', 'Stop', etc., est�n definidos en el archivo .ui asociado.
}


/**
 * @brief Destructor de la clase DeteccionCodigos.
 *
 * Libera los recursos asignados din�micamente durante la vida �til del objeto,
 * asegur�ndose de evitar fugas de memoria al eliminar el objeto de adquisici�n de video.
 */
DeteccionCodigos::~DeteccionCodigos()
{
    // Liberar la memoria asignada al objeto de la c�mara.
    if (camera != nullptr) {
        delete camera;
    }

    // Nota: No es necesario liberar recursos que est�n gestionados por el framework Qt,
    // ya que Qt se encarga de eliminar widgets hijos y otros elementos al destruir el objeto principal.
}

/**
 * @brief Actualiza la imagen mostrada en la interfaz seg�n el modo actual seleccionado.
 *
 * Esta funci�n captura una imagen desde la c�mara, la procesa seg�n el modo actual
 * (Normal, Decoded, RedMask, GreenMask) y muestra la imagen procesada en un QLabel
 * de la interfaz gr�fica.
 */
void DeteccionCodigos::UpdateImage() {
    // Capturar la imagen desde la c�mara.
    imgcapturada = camera->getImage();

    // Declaraci�n de la imagen final en formato QImage para mostrarla en la interfaz.
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
            // Modo m�scara roja: aplicar varios pasos de procesamiento.
            // 1. Filtrar la imagen para suavizarla y reducir el ruido.
            imagenFinal = BlurImage(imgcapturada, 7);
            // 2. Convertir la imagen a formato HSV.
            imagenFinal = convertHSVImage(imagenFinal);
            // 3. Generar la m�scara roja.
            imagenFinal = getRedMask(imagenFinal);
            // 4. Aplicar la m�scara a la imagen original.
            imagenFinal = applyMaskToImage(imgcapturada, imagenFinal);
            qimg = QImage((const unsigned char *)( imagenFinal.data ),
                          imagenFinal.cols,
                          imagenFinal.rows,
                          imagenFinal.step,
                          QImage::Format_BGR888);
            break;
        case GreenMask:
            // Modo m�scara verde: aplicar varios pasos de procesamiento.
            // 1. Filtrar la imagen para suavizarla y reducir el ruido.
            imagenFinal = BlurImage(imgcapturada, 7);
            // 2. Convertir la imagen a formato HSV.
            imagenFinal = convertHSVImage(imagenFinal);
            // 3. Generar la m�scara verde.
            imagenFinal = getGreenMask(imagenFinal);
            // 4. Aplicar la m�scara a la imagen original.
            imagenFinal = applyMaskToImage(imgcapturada, imagenFinal);
            qimg = QImage((const unsigned char *)( imagenFinal.data ),
                          imagenFinal.cols,
                          imagenFinal.rows,
                          imagenFinal.step,
                          QImage::Format_BGR888);
            break;
    }
    // Mostrar la imagen procesada en el QLabel de la interfaz gr�fica.
    ui.label->setPixmap(QPixmap::fromImage(qimg));
}

/**
 * @brief Maneja el comportamiento del bot�n "Record".
 *
 * Esta funci�n inicia o detiene la captura de im�genes desde la c�mara seg�n el estado del bot�n.
 * Tambi�n configura un temporizador para actualizar la imagen mostrada en la interfaz gr�fica.
 *
 * @param captura Indica si el bot�n "Record" est� activado (`true`) o desactivado (`false`).
 */
void DeteccionCodigos::RecordButton(bool captura) {
    // Mensaje de depuraci�n para verificar el estado del bot�n.
    qDebug() << "Boton Record pulsado: " << captura;

    if (captura) {
        // Si el bot�n est� activado, iniciar la captura de im�genes.
        if (!timer) {
            // Crear un temporizador si no existe y conectarlo al slot `UpdateImage`.
            timer = new QTimer(this);
            connect(timer, &QTimer::timeout, this, &DeteccionCodigos::UpdateImage);
            timer->start(30); // Configurar el temporizador para actualizar cada 30 ms.
        }

        // Comenzar la captura de video desde la c�mara.
        camera->startStopCapture(true);
    }
    else {
        // Si el bot�n est� desactivado, detener la captura de im�genes.
        if (timer) {
            // Detener el temporizador, eliminarlo y liberar la memoria.
            timer->stop();
            delete timer;
            timer = nullptr;
        }

        // Detener la captura de video desde la c�mara.
        camera->startStopCapture(false);

        // Limpiar el QLabel que muestra la imagen.
        ui.label->clear();
    }
}


/**
 * @brief Maneja el comportamiento del bot�n "Stop".
 *
 * Esta funci�n detiene o reinicia la captura de im�genes desde la c�mara,
 * dependiendo del estado del bot�n "Stop".
 *
 * @param captura Indica si el bot�n "Stop" est� activado (`true`) o desactivado (`false`).
 */
void DeteccionCodigos::StropButton(bool captura) {
    // Mensaje de depuraci�n para verificar el estado del bot�n.
    qDebug() << "Boton Stop pulsado: " << captura;

    // Alternar la captura de video seg�n el estado del bot�n.
    // Si `captura` es `true`, detiene la captura; si es `false`, reinicia la captura.
    camera->startStopCapture(!captura);
}


/**
 * @brief Activa o desactiva el modo de decodificaci�n de im�genes.
 *
 * Esta funci�n cambia el modo actual de procesamiento de im�genes entre
 * "Decoded" (decodificado) y "Normal", dependiendo del estado del bot�n.
 *
 * @param captura Indica si el bot�n "Decode" est� activado (`true`) o desactivado (`false`).
 */
void DeteccionCodigos::DecodeImagen(bool captura) {
    if (captura) {
        // Activar el modo de decodificaci�n.
        qDebug() << "Decodificando imagen...";
        currentMode = Decoded;
    }
    else {
        // Restaurar el modo normal de visualizaci�n.
        currentMode = Normal;
    }
}


/**
 * @brief Activa o desactiva el modo de visualizaci�n de la m�scara roja.
 *
 * Esta funci�n cambia el modo actual de procesamiento de im�genes entre
 * "RedMask" (m�scara roja) y "Normal", dependiendo del estado del bot�n.
 *
 * @param captura Indica si el bot�n "Red Mask" est� activado (`true`) o desactivado (`false`).
 */
void DeteccionCodigos::ViewRedMask(bool captura) {
    if (captura) {
        // Activar el modo de m�scara roja.
        qDebug() << "Mostrando mascara roja...";
        currentMode = RedMask;
    }
    else {
        // Restaurar el modo normal de visualizaci�n.
        currentMode = Normal;
    }
}


/**
 * @brief Activa o desactiva el modo de visualizaci�n de la m�scara verde.
 *
 * Esta funci�n cambia el modo actual de procesamiento de im�genes entre
 * "GreenMask" (m�scara verde) y "Normal", dependiendo del estado del bot�n.
 *
 * @param captura Indica si el bot�n "Green Mask" est� activado (`true`) o desactivado (`false`).
 */
void DeteccionCodigos::ViewGreenMask(bool captura) {
    if (captura) {
        // Activar el modo de m�scara verde.
        qDebug() << "Mostrando mascara verde...";
        currentMode = GreenMask;
    }
    else {
        // Restaurar el modo normal de visualizaci�n.
        currentMode = Normal;
    }
}


/**
 * @brief Guarda la imagen decodificada como un archivo de imagen.
 *
 * Esta funci�n permite al usuario guardar la imagen procesada (`imagenFinal`)
 * como un archivo de imagen en formato JPG, utilizando un cuadro de di�logo
 * para seleccionar la ubicaci�n y el nombre del archivo.
 */
void DeteccionCodigos::SaveDecodedCode()
{
    // Mensaje de depuraci�n para indicar que se est� guardando el c�digo decodificado.
    qDebug() << "Guardando codigo decodificado...";

    // Abrir un cuadro de di�logo para permitir al usuario seleccionar la ubicaci�n y nombre del archivo.
    QString fileName = QFileDialog::getSaveFileName(this, tr("Guardar imagen"), "", tr("Archivos de imagen (*.jpg)"));

    // Verificar si el nombre del archivo no est� vac�o (es decir, si el usuario seleccion� un archivo).
    if (!fileName.isEmpty()) {
        // Guardar la imagen final en el archivo especificado por el usuario.
        imwrite(fileName.toStdString(), imagenFinal);
    }
}


/**
 * @brief Aplica un filtro de desenfoque gaussiano a la imagen.
 *
 * Esta funci�n aplica un desenfoque gaussiano a la imagen proporcionada para
 * reducir el ruido y suavizar la imagen. El tama�o del filtro se puede ajustar
 * a trav�s del par�metro `kernerSsize`.
 *
 * @param image La imagen original sobre la que se aplicar� el desenfoque.
 * @param kernerSsize El tama�o del kernel para el filtro de desenfoque (debe ser un n�mero impar).
 *
 * @return Mat La imagen procesada con el desenfoque gaussiano aplicado.
 */
Mat DeteccionCodigos::BlurImage(const Mat &image, uint8_t kernerSsize) {
    // Aplicar el filtro de desenfoque gaussiano con el tama�o de kernel especificado
    Mat blurImage;
    GaussianBlur(image, blurImage, Size(kernerSsize, kernerSsize), 0);
    return blurImage;
}


/**
 * @brief Convierte una imagen a escala de grises.
 *
 * Esta funci�n toma una imagen en color (en formato BGR) y la convierte a una
 * imagen en escala de grises. La conversi�n es �til para simplificar el procesamiento
 * de im�genes en tareas como la detecci�n de bordes o la segmentaci�n.
 *
 * @param image La imagen original que se va a convertir (en formato BGR).
 *
 * @return Mat La imagen convertida a escala de grises.
 */
Mat DeteccionCodigos::convertGrayImage(const Mat &image) {
    // Convertir la imagen de BGR a escala de grises usando la funci�n cvtColor
    Mat grayImage;
    cvtColor(image, grayImage, COLOR_BGR2GRAY);
    return grayImage;
}


/**
 * @brief Convierte una imagen al espacio de color HSV.
 *
 * Esta funci�n toma una imagen en el formato BGR y la convierte a HSV (Hue, Saturation, Value).
 * El espacio de color HSV es frecuentemente utilizado en procesamiento de im�genes para tareas como
 * la segmentaci�n de colores y la detecci�n de objetos, ya que separa la informaci�n de tono (hue)
 * de la de intensidad (valor), lo que puede facilitar la detecci�n de colores espec�ficos.
 *
 * @param image La imagen original que se va a convertir (en formato BGR).
 *
 * @return Mat La imagen convertida al espacio de color HSV.
 */
Mat DeteccionCodigos::convertHSVImage(const Mat &image) {
    // Convertir la imagen de BGR a HSV usando la funci�n cvtColor
    Mat hsvImage;
    cvtColor(image, hsvImage, COLOR_BGR2HSV);
    return hsvImage;
}


/**
 * @brief Genera una m�scara para detectar el color rojo en una imagen en espacio HSV.
 *
 * Esta funci�n utiliza los valores del espacio de color HSV para crear una m�scara que detecte
 * los p�xeles correspondientes al color rojo. Dado que el color rojo se encuentra en dos rangos
 * en el espacio HSV, se crean dos m�scaras que se combinan para cubrir ambos rangos del color rojo.
 *
 * @param image La imagen en espacio HSV sobre la que se aplicar� la m�scara.
 *
 * @return Mat La m�scara generada donde los p�xeles correspondientes al color rojo son blancos
 *             (valor 255) y los dem�s son negros (valor 0).
 */
Mat DeteccionCodigos::getRedMask(const Mat &image) {
    // Crear dos m�scaras separadas para los dos rangos de color rojo en el espacio HSV
    Mat mascaraRoja, mascaraRoja2;

    // Rango 1: Detectar rojo en el rango de tono [0, 10]
    inRange(image, Scalar(0, 50, 50), Scalar(10, 255, 255), mascaraRoja);

    // Rango 2: Detectar rojo en el rango de tono [150, 179]
    inRange(image, Scalar(150, 50, 50), Scalar(179, 255, 255), mascaraRoja2);

    // Combinar las dos m�scaras utilizando la operaci�n OR
    mascaraRoja = mascaraRoja | mascaraRoja2;

    // Retornar la m�scara combinada
    return mascaraRoja;
}


/**
 * @brief Genera una m�scara para detectar el color verde en una imagen en espacio HSV.
 *
 * Esta funci�n utiliza los valores del espacio de color HSV para crear una m�scara que detecte
 * los p�xeles correspondientes al color verde. La m�scara es generada usando un rango espec�fico
 * de valores para el tono (Hue), la saturaci�n (Saturation) y el valor (Value) en el espacio HSV.
 *
 * @param image La imagen en espacio HSV sobre la que se aplicar� la m�scara.
 *
 * @return Mat La m�scara generada donde los p�xeles correspondientes al color verde son blancos
 *             (valor 255) y los dem�s son negros (valor 0).
 */
Mat DeteccionCodigos::getGreenMask(const Mat &image) {
    // Crear la m�scara para el color verde en el espacio HSV con un rango ajustado
    Mat mascaraVerde;

    // Rango del verde en el espacio HSV: H [30, 90], S [20, 255], V [20, 255]
    inRange(image, Scalar(30, 55, 55), Scalar(90, 255, 255), mascaraVerde);

    // Retornar la m�scara generada
    return mascaraVerde;
}


/**
 * @brief Aplica una m�scara a una imagen.
 *
 * Esta funci�n toma una imagen y una m�scara binaria, y aplica la m�scara a la imagen.
 * Los p�xeles de la imagen que corresponden a los valores no nulos de la m�scara (generalmente 255)
 * se mantienen intactos, mientras que los p�xeles correspondientes a los valores cero de la m�scara
 * se establecen a cero en la imagen resultante.
 *
 * @param image La imagen original sobre la que se aplicar� la m�scara.
 * @param mask La m�scara binaria que se aplicar� sobre la imagen. Los p�xeles con valor 255
 *             en la m�scara se conservar�n en la imagen final, mientras que los p�xeles con valor 0
 *             se eliminar�n (se establecer�n en 0).
 *
 * @return Mat La imagen resultante con la m�scara aplicada, donde los p�xeles que no est�n en la m�scara
 *             ser�n eliminados (negros) y los que est�n en la m�scara se mantendr�n intactos.
 */
Mat DeteccionCodigos::applyMaskToImage(const Mat &image, Mat mask) {
    // Crear una copia de la imagen original y aplicar la m�scara sobre ella.
    Mat maskedImage;
    // La funci�n copyTo copia los p�xeles de la imagen original a 'maskedImage', pero solo donde la m�scara tiene valor 255
    image.copyTo(maskedImage, mask);

    // Retornar la imagen con la m�scara aplicada
    return maskedImage;
}


/**
 * @brief Aplica el filtro Sobel para detectar bordes en una imagen.
 *
 * Esta funci�n utiliza el filtro Sobel en las direcciones X e Y para calcular el gradiente de la imagen.
 * Luego, calcula la magnitud del gradiente para detectar los bordes y aplica una umbralizaci�n binaria
 * para obtener una imagen binaria donde los bordes son visibles.
 *
 * @param image La imagen de entrada sobre la que se aplicar� el filtro Sobel. Debe ser una imagen en escala de grises.
 * @param kernelSize El tama�o del kernel que se utilizar� para el filtro Sobel. Un valor t�pico es 3 o 5.
 *
 * @return Mat La imagen resultante con los bordes detectados, en formato binario.
 */
Mat DeteccionCodigos::sobelFilter(const Mat &image, uint8_t kernelSize) {
    // Declaraci�n de las im�genes intermedias para los resultados de los filtros Sobel en X y Y
    Mat img_sobel_x, img_sobel_y, img_sobel, filtered_image;

    // Aplicar el filtro Sobel en la direcci�n X (detecta bordes en la direcci�n horizontal)
    Sobel(image, img_sobel_x, CV_64F, 1, 0, kernelSize);

    // Aplicar el filtro Sobel en la direcci�n Y (detecta bordes en la direcci�n vertical)
    Sobel(image, img_sobel_y, CV_64F, 0, 1, kernelSize);

    // Calcular la magnitud del gradiente a partir de las im�genes resultantes de Sobel en X y Y
    magnitude(img_sobel_x, img_sobel_y, img_sobel);

    // Normalizar la imagen resultante para que los valores est�n en el rango [0, 255]
    // Esto es necesario para poder trabajar con una imagen de tipo CV_8U (escala de grises en 8 bits)
    normalize(img_sobel, img_sobel, 0, 255, NORM_MINMAX, CV_8U);

    // Aplicar umbralizaci�n binaria para resaltar los bordes detectados
    // Los p�xeles con un valor superior a 30 ser�n establecidos a 255 (blanco), el resto ser� 0 (negro)
    threshold(img_sobel, filtered_image, 30, 255, THRESH_BINARY);

    // Retornar la imagen con los bordes detectados
    return filtered_image;
}


/**
 * @brief Encuentra y filtra los contornos detectados en una imagen usando el filtro Sobel.
 *
 * Esta funci�n aplica el filtro Sobel para detectar los bordes en la imagen, luego encuentra todos los contornos
 * en la imagen binarizada resultante. Posteriormente, filtra los contornos seg�n su �rea y aspecto (proporci�n de
 * ancho a altura) para asegurarse de que solo se retengan los contornos de inter�s.
 *
 * @param image La imagen de entrada sobre la cual se detectar�n los contornos. La imagen debe estar en formato
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

    // Paso 3: Filtrar los contornos seg�n su �rea y su aspecto (relaci�n entre ancho y alto)
    std::vector<std::vector<Point>> filteredContours;
    for (const auto &contour : contours) {
        // Calcular el �rea del contorno
        double area = contourArea(contour);

        // Calcular el rect�ngulo delimitador del contorno
        Rect boundingBox = boundingRect(contour);

        // Calcular el 1% del �rea total de la imagen (para establecer umbrales de �rea)
        double areaImage = image.rows * image.cols;
        double umbralBajoArea = 0.01 * areaImage; // 5% del �rea de la imagen
        double umbralAltoArea = 0.25 * areaImage; // 25% del �rea de la imagen

        // Calcular la relaci�n de aspecto (aspect ratio) del rect�ngulo delimitador
        double aspectRatio = static_cast<double>( boundingBox.width ) / boundingBox.height;

        // Filtrar los contornos por �rea y relaci�n de aspecto
        // El contorno debe tener un �rea dentro de un rango espec�fico y una relaci�n de aspecto entre 0.5 y 1.3
        if (area > umbralBajoArea && area < umbralAltoArea && aspectRatio > 0.5 && aspectRatio < 1.3) {
            filteredContours.push_back(contour);
        }
    }

    // Paso 4: Devolver los contornos filtrados
    return filteredContours;
}

/**
 * @brief Extrae informaci�n relevante de los contornos proporcionados.
 *
 * Esta funci�n toma un conjunto de contornos detectados y extrae caracter�sticas geom�tricas importantes para
 * cada uno de ellos, como el �rea, el per�metro, las esquinas del contorno, el centro, las dimensiones (ancho y alto),
 * la relaci�n de aspecto, y el �ngulo de orientaci�n.
 *
 * @param contours Un vector de vectores de puntos que representan los contornos de la imagen.
 *                 Cada contorno es una secuencia cerrada de puntos que forma un objeto detectado.
 *
 * @return std::vector<ContourInfo> Un vector de estructuras `ContourInfo` que contienen la informaci�n
 *         extra�da de cada contorno. Cada estructura contiene los detalles geom�tricos de un contorno.
 */
std::vector<ContourInfo> DeteccionCodigos::extractContourInfo(const std::vector<std::vector<Point>> &contours) {
    // Paso 1: Declarar el vector que almacenar� la informaci�n de cada contorno
    std::vector<ContourInfo> contour_info;

    // Paso 2: Iterar sobre cada contorno para extraer su informaci�n
    for (const auto &contour : contours) {
        ContourInfo info;

        // Paso 3: Calcular el �rea del contorno
        info.area = contourArea(contour);

        // Paso 4: Obtener el rect�ngulo delimitador del contorno (bounding box)
        Rect boundingBox = boundingRect(contour);

        // Paso 5: Calcular el per�metro del contorno (longitud de su frontera)
        info.perimeter = arcLength(contour, true);

        // Paso 6: Obtener el rect�ngulo rotado que ajusta el contorno m�s estrechamente
        RotatedRect rect = minAreaRect(contour);
        Point2f box[4];
        rect.points(box); // Obtener las coordenadas de los 4 v�rtices del rect�ngulo rotado

        // Paso 7: Almacenar las esquinas del rect�ngulo en la estructura de informaci�n del contorno
        for (int i = 0; i < 4; i++) {
            info.corners.push_back(Point(static_cast<int>( box[i].x ), static_cast<int>( box[i].y )));
        }

        // Paso 8: Calcular el centro del contorno como el centro del rect�ngulo delimitador
        info.center = Point2f(( boundingBox.x + boundingBox.width ) / 2.0,
                              ( boundingBox.y + boundingBox.height ) / 2.0);

        // Paso 9: Almacenar el ancho y el alto del rect�ngulo delimitador
        info.width = boundingBox.width;
        info.height = boundingBox.height;

        // Paso 10: Calcular la relaci�n de aspecto (ancho/alto) del rect�ngulo y almacenarla
        info.aspect_ratio = ( boundingBox.height != 0 ) ? boundingBox.width / static_cast<float>( boundingBox.height ) : 0;

        // Paso 11: Almacenar el �ngulo de rotaci�n del rect�ngulo respecto a la horizontal
        info.angle = rect.angle;

        // Paso 12: A�adir la informaci�n del contorno a la lista de resultados
        contour_info.push_back(info);

        // Paso 13 (opcional): Mostrar la informaci�n del contorno en el log para depuraci�n
        /*qDebug() << "Area: " << info.area
                 << " Perimetro: " << info.perimeter
                 << " Centro: " << info.center.x << ", " << info.center.y
                 << " Ancho: " << info.width
                 << " Alto: " << info.height
                 << " Relacion de aspecto: " << info.aspect_ratio
                 << " Angulo: " << info.angle;*/
    }

    // Paso 14: Devolver el vector con la informaci�n de todos los contornos procesados
    return contour_info;
}


/**
 * @brief Empareja contornos rojos con contornos verdes bas�ndose en criterios geom�tricos y de similitud.
 *
 * Esta funci�n toma dos conjuntos de contornos detectados (uno rojo y otro verde) y los empareja en pares
 * bas�ndose en criterios como la proximidad entre sus centros, la diferencia en su orientaci�n (�ngulo),
 * y similitudes en el �rea y el per�metro. Solo se emparejan los contornos que cumplen con estos criterios.
 *
 * @param redContoursInfo Un vector de estructuras `ContourInfo` que contienen la informaci�n geom�trica
 *                        de los contornos detectados en la m�scara roja.
 * @param greenContoursInfo Un vector de estructuras `ContourInfo` que contienen la informaci�n geom�trica
 *                          de los contornos detectados en la m�scara verde.
 *
 * @return std::vector<std::pair<ContourInfo, ContourInfo>> Un vector de pares de contornos emparejados.
 *         Cada par contiene un contorno rojo y un contorno verde que se consideran coincidentes.
 */
std::vector<std::pair<ContourInfo, ContourInfo>> DeteccionCodigos::matchContours(
    const std::vector<ContourInfo> &redContoursInfo,
    const std::vector<ContourInfo> &greenContoursInfo) {

    // Paso 1: Declarar el vector que almacenar� los pares de contornos emparejados
    std::vector<std::pair<ContourInfo, ContourInfo>> matches;

    // Paso 2: Crear conjuntos para llevar un seguimiento de los contornos ya emparejados
    std::set<const ContourInfo *> usedRedContours;
    std::set<const ContourInfo *> usedGreenContours;

    // Paso 3: Iterar sobre cada contorno rojo
    for (const auto &redContour : redContoursInfo) {
        // Verificar si este contorno rojo ya est� emparejado
        if (usedRedContours.find(&redContour) != usedRedContours.end()) {
            continue; // Saltar este contorno si ya est� emparejado
        }

        // Paso 4: Inicializar variables para encontrar el mejor match para el contorno rojo actual
        const ContourInfo *bestMatch = nullptr;               // Apuntador al mejor contorno verde encontrado
        double bestAngleMatch = std::numeric_limits<double>::infinity(); // Mejor diferencia de �ngulo
        double bestScore = std::numeric_limits<double>::infinity();      // Mejor puntaje para desempatar

        // Paso 5: Iterar sobre cada contorno verde
        for (const auto &greenContour : greenContoursInfo) {
            // Verificar si este contorno verde ya est� emparejado
            if (usedGreenContours.find(&greenContour) != usedGreenContours.end()) {
                continue; // Saltar este contorno si ya est� emparejado
            }

            // Paso 6: Calcular la distancia entre los centros de los contornos
            double centerDistance = cv::norm(redContour.center - greenContour.center);

            // Paso 7: Descartar contornos que est�n demasiado lejos (m�s de 3.5 veces el ancho del contorno rojo)
            if (centerDistance >  redContour.perimeter / 2.5) {
                continue; // Saltar este contorno verde por estar demasiado lejos
            }

            //// Paso 8: Descartar contornos que est�n demasiado cerca (m�s de 2.5 veces el ancho del contorno rojo)
            if (centerDistance < redContour.perimeter / 3.5) {
                continue; // Saltar este contorno verde por estar demasiado cerca
            }

            // Paso 9: Calcular la diferencia de �ngulo entre los contornos
            double angleDiff = std::abs(redContour.angle - greenContour.angle);

            // Paso 10: Calcular diferencias en �rea y per�metro para usar como criterios de desempate
            double areaDiff = std::abs(redContour.area - greenContour.area);
            double perimeterDiff = std::abs(redContour.perimeter - greenContour.perimeter);

            // Paso 11: Calcular un puntaje de desempate basado en las diferencias de �rea y per�metro
            double score = areaDiff + perimeterDiff;

            // Paso 12: Actualizar el mejor match si el �ngulo es menor, o si el puntaje es menor en caso de empate
            if (angleDiff < bestAngleMatch || ( angleDiff == bestAngleMatch && score < bestScore )) {
                bestAngleMatch = angleDiff;
                bestScore = score;
                bestMatch = &greenContour; // Actualizar el mejor contorno verde encontrado
            }
        }

        // Paso 13: Si se encontr� un match v�lido para el contorno rojo actual
        if (bestMatch) {
            // A�adir el par de contornos emparejados al vector de resultados
            matches.emplace_back(redContour, *bestMatch);

            // Marcar ambos contornos como emparejados
            usedRedContours.insert(&redContour);
            usedGreenContours.insert(bestMatch);
        }
    }

    // Paso 14: Informaci�n de depuraci�n para verificar el n�mero de contornos emparejados
    /*qDebug() << "Numero de contornos emparejados: " << matches.size();*/

    // Paso 15: Devolver el vector de pares de contornos emparejados
    return matches;
}


/**
 * @brief Extrae regiones de la imagen delimitadas por los contornos emparejados, alineando cada regi�n para que la l�nea entre los contornos sea horizontal.
 *
 * Esta funci�n toma un conjunto de contornos emparejados (rojo y verde) y recorta la regi�n de la imagen original
 * que contiene ambos contornos. Adem�s, rota la imagen para alinear los contornos horizontalmente y recorta la regi�n resultante.
 *
 * @param matchedContours Un vector de pares de contornos emparejados (rojo y verde). Cada par contiene la informaci�n
 *                        geom�trica de dos contornos que se han identificado como relacionados.
 * @param image La imagen original de la cual se extraer�n las regiones delimitadas por los contornos.
 *
 * @return std::vector<Mat> Un vector de im�genes recortadas (Mat) que contienen las regiones extra�das de la imagen original.
 *         Cada imagen corresponde a una regi�n delimitada por un par de contornos emparejados.
 */
std::vector<Mat> DeteccionCodigos::cutBoundingBox(const std::vector<pair<ContourInfo, ContourInfo>> &matchedContours, const Mat &image) {
    // Paso 1: Inicializar un vector para almacenar las im�genes recortadas
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

        // Paso 5: Calcular el rect�ngulo delimitador (bounding box) que contenga todos los puntos
        Rect boundingBox = boundingRect(allPoints);

        // Paso 6: Calcular el centro del rect�ngulo delimitador
        Point boundingBoxCenter = Point(boundingBox.x + boundingBox.width / 2, boundingBox.y + boundingBox.height / 2);

        // Paso 7: Calcular el �ngulo entre los centros de los contornos rojo y verde
        double x0 = redContour.center.x;
        double y0 = redContour.center.y;
        double x1 = greenContour.center.x;
        double y1 = greenContour.center.y;
        double angle = atan2(y1 - y0, x1 - x0) * 180 / CV_PI;

        // Paso 8: Rotar la imagen para que la l�nea entre los contornos sea horizontal
        Mat M = getRotationMatrix2D(boundingBoxCenter, angle, 1);
        Mat rotatedImage;
        warpAffine(image, rotatedImage, M, image.size());

        // Paso 9: Rotar los puntos de los contornos usando la matriz de transformaci�n
        std::vector<Point> transformedPoints;
        for (const Point &pt : allPoints) {
            double xNew = M.at<double>(0, 0) * pt.x + M.at<double>(0, 1) * pt.y + M.at<double>(0, 2);
            double yNew = M.at<double>(1, 0) * pt.x + M.at<double>(1, 1) * pt.y + M.at<double>(1, 2);
            transformedPoints.emplace_back(cvRound(xNew), cvRound(yNew));
        }

        // Paso 10: Calcular la nueva bounding box despu�s de la rotaci�n
        Rect transformedBoundingBox = boundingRect(transformedPoints);

        // Paso 11: Verificar si la bounding box transformada est� dentro de los l�mites de la imagen
        if (transformedBoundingBox.x >= 0 && transformedBoundingBox.y >= 0 &&
            transformedBoundingBox.x + transformedBoundingBox.width <= rotatedImage.cols &&
            transformedBoundingBox.y + transformedBoundingBox.height <= rotatedImage.rows) {
            // Paso 12: Recortar la regi�n de la imagen rotada
            Mat extractedImage = rotatedImage(transformedBoundingBox);
            extractedImages.push_back(extractedImage);
        }
        else {
            std::cout << "La bounding box transformada est� fuera de los limites." << std::endl;
        }
    }

    // Paso 13: Devolver el vector de im�genes recortadas
    return extractedImages;
}


/**
 * @brief Aplica un umbral adaptativo a la imagen y realiza operaciones morfol�gicas para suavizar los bordes.
 *
 * Esta funci�n convierte una imagen en escala de grises en una binaria utilizando un umbral adaptativo basado en
 * el m�todo de medias ponderadas (GAUSSIAN). Posteriormente, aplica operaciones morfol�gicas como cierre y erosi�n
 * para eliminar imperfecciones en los bordes y suavizar el resultado.
 *
 * @param image La imagen en escala de grises sobre la que se aplicar� el umbral adaptativo y las operaciones morfol�gicas.
 *              Se espera que esta imagen sea de tipo `CV_8UC1` (1 canal, escala de grises).
 * @param threshold Un valor de ajuste para el c�lculo del umbral adaptativo. Este par�metro influye en la segmentaci�n
 *                  de la imagen, permitiendo afinar los detalles capturados en la binarizaci�n.
 *
 * @return Mat La imagen resultante despu�s de aplicar el umbral adaptativo y las operaciones de cierre y erosi�n.
 *             Es una imagen binaria donde los p�xeles son 0 (negro) o 255 (blanco).
 */
Mat DeteccionCodigos::thresholdImage(const Mat &image, int threshold) {
    // Paso 1: Aplicar umbral adaptativo con el m�todo GAUSSIAN
    Mat imageThresholdGaussian;
    adaptiveThreshold(image, imageThresholdGaussian, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 11, threshold);

    // Paso 2: Crear un kernel para las operaciones morfol�gicas
    Mat kernel = Mat::ones(Size(3, 3), CV_8U);

    // Paso 3: Aplicar la operaci�n morfol�gica de cierre para unir regiones desconectadas
    Mat closeImage;
    morphologyEx(imageThresholdGaussian, closeImage, MORPH_CLOSE, kernel);

    // Paso 4: Aplicar erosi�n para suavizar los bordes y reducir peque�as imperfecciones
    Mat erodeImage;
    erode(closeImage, erodeImage, kernel, Point(-1, -1), 1);

    // Paso 5: Devolver la imagen binaria resultante
    return erodeImage;
}


/**
 * @brief Clasifica los contornos en cuadrados y rect�ngulos seg�n su relaci�n de aspecto y �rea relativa.
 *
 * Esta funci�n analiza un conjunto de contornos detectados y los clasifica en dos categor�as:
 * contornos cuadrados y contornos rectangulares. La clasificaci�n se basa en la relaci�n de aspecto (ancho/alto)
 * y el �rea relativa del contorno respecto al tama�o de la imagen.
 *
 * @param contours Un vector de vectores de puntos que representan los contornos detectados.
 *                 Cada contorno es una secuencia cerrada de puntos que forma una figura.
 * @param imageShape Un objeto `Size` que especifica las dimensiones de la imagen original
 *                   (ancho y alto) sobre la que se detectaron los contornos.
 *
 * @return pair<std::vector<std::vector<Point>>, std::vector<std::vector<Point>>>
 *         Un par de vectores:
 *         - El primer elemento contiene los contornos clasificados como cuadrados.
 *         - El segundo elemento contiene los contornos clasificados como rect�ngulos.
 */
pair<std::vector<std::vector<Point>>, std::vector<std::vector<Point>>>
DeteccionCodigos::classifyContours(const std::vector<std::vector<Point>> &contours, const Size &imageShape) {
    // Paso 1: Declarar vectores para almacenar contornos cuadrados y rectangulares
    std::vector<std::vector<Point>> squareContours;
    std::vector<std::vector<Point>> rectangularContours;

    // Paso 2: Iterar sobre los contornos para clasificarlos
    for (const auto &contour : contours) {
        // Calcular el �rea del contorno
        double area = contourArea(contour);

        // Calcular la proporci�n del �rea del contorno con respecto al �rea de la imagen
        double areaRatio = area / ( imageShape.width * imageShape.height );

        // Obtener el rect�ngulo delimitador del contorno
        Rect boundingBox = boundingRect(contour);

        // Calcular la relaci�n de aspecto (ancho/alto) del rect�ngulo delimitador
        double aspectRatio = static_cast<double>( boundingBox.width ) / boundingBox.height;

        // Paso 3: Clasificar el contorno seg�n la relaci�n de aspecto y el �rea relativa
        if (0.5 <= aspectRatio && aspectRatio <= 1.5 && areaRatio > 0.06) {
            // Contornos con relaci�n de aspecto cercana a 1 y �rea significativa: cuadrados
            squareContours.push_back(contour);
        }
        else {
            // Contornos restantes: rect�ngulos
            rectangularContours.push_back(contour);
        }
    }

    // Paso 4: Devolver los contornos clasificados como un par
    return { squareContours, rectangularContours };
}



/**
 * @brief Filtra contornos que est�n completamente dentro de otros contornos m�s grandes.
 *
 * Esta funci�n elimina los contornos que est�n completamente contenidos dentro de otros
 * contornos m�s grandes. Esto es �til para evitar considerar contornos secundarios
 * o ruidos dentro de �reas cerradas que ya est�n representadas por un contorno m�s grande.
 *
 * @param contours Un vector de vectores de puntos que representan los contornos detectados.
 *                 Cada contorno es una secuencia cerrada de puntos que forma una figura.
 *
 * @return std::vector<std::vector<Point>>
 *         Un vector de contornos filtrados donde se han eliminado los contornos que est�n
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

        // Paso 3: Comprobar si el contorno actual est� dentro de otro contorno
        for (size_t j = 0; j < contours.size(); ++j) {
            if (i == j)
                continue; // Saltar la comparaci�n del contorno consigo mismo

            const std::vector<Point> &otherContour = contours[j];
            bool isInside = true;

            // Paso 4: Verificar si todos los puntos del contorno actual est�n dentro del otro contorno
            for (const Point &pt : contour) {
                if (pointPolygonTest(otherContour, pt, false) <= 0) {
                    // Si alg�n punto no est� dentro, no es un contorno interno
                    isInside = false;
                    break;
                }
            }

            // Paso 5: Actualizar el contorno padre si el contorno actual est� contenido
            if (isInside) {
                if (parentContour == nullptr ||
                    contourArea(otherContour) > contourArea(*parentContour)) {
                    parentContour = &otherContour; // Actualizar al contorno m�s grande
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
 * Esta funci�n detecta contornos en una imagen umbralizada y aplica varios criterios de filtrado,
 * como el �rea, la influencia relativa al tama�o de la imagen, y la posici�n dentro de los l�mites
 * de la imagen. Tambi�n clasifica los contornos detectados en cuadrados y rect�ngulos, y elimina
 * aquellos que est�n completamente contenidos dentro de otros m�s grandes.
 *
 * @param thresholdedImage Imagen binarizada en la que se buscar�n los contornos.
 * @param image Imagen original utilizada para calcular el �rea relativa y validar los l�mites.
 *
 * @return std::vector<std::vector<Point>>
 *         Un vector de contornos filtrados que cumplen con los criterios de tama�o, forma, y posici�n.
 */
std::vector<std::vector<Point>>
DeteccionCodigos::getContours(const Mat &thresholdedImage, const Mat &image) {
    // Paso 1: Declarar los vectores para almacenar los contornos y la jerarqu�a
    std::vector<std::vector<Point>> contours;
    std::vector<Vec4i> hierarchy;

    // Paso 2: Detectar los contornos en la imagen umbralizada
    findContours(thresholdedImage, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

    // Paso 3: Declarar un vector para almacenar los contornos que pasen los filtros iniciales
    std::vector<std::vector<Point>> filteredContours;

    // Paso 4: Calcular el �rea de la imagen y establecer el umbral m�nimo de influencia
    double imageArea = image.rows * image.cols;
    double minInfluence = 0.01;

    // Paso 5: Filtrar contornos seg�n el �rea, la influencia, y su proximidad al borde
    for (const auto &contour : contours) {
        double area = contourArea(contour); // Calcular el �rea del contorno
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

    // Paso 6: Clasificar los contornos filtrados en cuadrados y rect�ngulos
    auto [squareContours, rectangularContours] = classifyContours(filteredContours, image.size());

    // Paso 7: Filtrar contornos que est�n completamente contenidos dentro de otros
    filteredContours = filterInsideContours(rectangularContours);

    // Paso 8: Devolver el conjunto final de contornos filtrados
    return filteredContours;
}


/**
 * @brief Separa los contornos en segmentos horizontales seg�n su posici�n en la imagen.
 *
 * Esta funci�n divide los contornos en cuatro segmentos horizontales, bas�ndose en la posici�n
 * del centro del contorno dentro de la anchura de la imagen. Cada segmento representa una
 * divisi�n igual del ancho total de la imagen.
 *
 * @param contours Un vector de contornos representados como vectores de puntos.
 *                 Cada contorno es una secuencia de puntos que define un objeto detectado.
 * @param imageWidth La anchura de la imagen, utilizada para calcular los l�mites de cada segmento.
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

        // Paso 4: Calcular la posici�n del centro en el eje X
        int centerX = boundingBox.x + boundingBox.width / 2;

        // Paso 5: Determinar a qu� segmento pertenece el contorno
        int segment = centerX / ( imageWidth / 4 );

        // Paso 6: Asegurarse de que el segmento sea v�lido y agregar el contorno
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
 * Esta funci�n organiza los contornos dentro de cada segmento en funci�n de su orientaci�n y
 * la posici�n de su rect�ngulo delimitador (bounding box). Si hay dos contornos en un segmento,
 * la funci�n los ordena seg�n la coordenada X o Y dependiendo de si los contornos son m�s anchos
 * o m�s altos. Si hay un solo contorno en el segmento, no se realiza ning�n orden.
 *
 * @param segments Un vector de vectores de contornos, donde cada subvector representa un segmento
 *                 horizontal en la imagen. Cada contorno es un vector de puntos que define un objeto detectado.
 *
 * @return std::vector<std::vector<std::vector<Point>>> Un vector que contiene los segmentos ordenados,
 *         cada uno con los contornos ordenados por su coordenada X o Y, dependiendo de su forma.
 */
std::vector<std::vector<std::vector<Point>>> DeteccionCodigos::orderContours(
    const std::vector<std::vector<std::vector<Point>>> &segments) {

    // Paso 1: Crear el vector que almacenar� los segmentos ordenados
    std::vector<std::vector<std::vector<Point>>> orderedSegments;

    // Paso 2: Iterar sobre cada segmento para ordenarlo si es necesario
    for (const auto &segment : segments) {
        if (segment.size() <= 1) {
            // Si el segmento tiene 0 o 1 contorno, no se realiza ordenaci�n
            orderedSegments.push_back(segment);
        }
        else if (segment.size() == 2) {
            // Si el segmento tiene exactamente 2 contornos
            Rect boundingBox1 = boundingRect(segment[0]);
            Rect boundingBox2 = boundingRect(segment[1]);

            // Determinar si los contornos son m�s anchos (wide) que altos (no wide)
            bool isWide1 = boundingBox1.width > boundingBox1.height;
            bool isWide2 = boundingBox2.width > boundingBox2.height;

            // Copiar el segmento para ordenarlo
            std::vector<std::vector<Point>> sortedSegment = segment;

            if (isWide1 && isWide2) {
                // Si ambos contornos son anchos, ordenar por la coordenada Y de su rect�ngulo delimitador
                sort(sortedSegment.begin(), sortedSegment.end(),
                     [](const std::vector<Point> &a, const std::vector<Point> &b) {
                    return boundingRect(a).y < boundingRect(b).y;
                });
            }
            else if (!isWide1 && !isWide2) {
                // Si ambos contornos no son anchos, ordenar por la coordenada X de su rect�ngulo delimitador
                sort(sortedSegment.begin(), sortedSegment.end(),
                     [](const std::vector<Point> &a, const std::vector<Point> &b) {
                    return boundingRect(a).x < boundingRect(b).x;
                });
            }

            // A�adir el segmento ordenado a la lista
            orderedSegments.push_back(sortedSegment);
        }
    }

    // Paso 3: Devolver los segmentos ordenados
    return orderedSegments;
}


/**
 * @brief Calcula la relaci�n entre el �rea de cada contorno y el �rea de una cuarta parte de la imagen.
 *
 * Esta funci�n toma los contornos detectados en una imagen y calcula la relaci�n entre el �rea de cada contorno
 * y el �rea de una cuarta parte de la imagen. Esta relaci�n es �til para determinar qu� tan grande es un contorno
 * con respecto al tama�o total de la imagen, lo que puede ayudar a filtrar contornos peque�os o grandes seg�n se desee.
 *
 * @param contours Un vector de vectores de puntos que representan los contornos detectados en la imagen.
 *                 Cada contorno es una secuencia cerrada de puntos que forma un objeto detectado.
 * @param image La imagen en la que se detectaron los contornos. Se usa para calcular el �rea total de la imagen.
 *
 * @return std::vector<double> Un vector que contiene la relaci�n del �rea de cada contorno con respecto
 *         al �rea de una cuarta parte de la imagen. El valor de cada elemento es un n�mero decimal que
 *         representa esta relaci�n.
 */
std::vector<double> DeteccionCodigos::getAreaRatio(const std::vector<std::vector<Point>> &contours, const Mat &image) {
    // Paso 1: Calcular el �rea total de la imagen dividida por 4
    double imageArea = ( image.rows * image.cols ) / 4.0;

    // Paso 2: Crear el vector para almacenar las relaciones de �rea
    std::vector<double> areaRatios;

    // Paso 3: Iterar sobre cada contorno para calcular su relaci�n de �rea
    for (const auto &contour : contours) {
        // Calcular el �rea del contorno
        double area = contourArea(contour);

        // Calcular la relaci�n entre el �rea del contorno y el �rea de la imagen
        double areaRatio = area / imageArea;

        // Almacenar la relaci�n de �rea calculada en el vector
        areaRatios.push_back(areaRatio);
    }

    // Paso 4: Devolver el vector con las relaciones de �rea
    return areaRatios;
}

/**
 * @brief Obtiene informaci�n detallada sobre los segmentos de contornos, incluyendo el n�mero de contornos,
 *        sus orientaciones, relaciones de �rea y otras m�tricas relevantes.
 *
 * Esta funci�n toma los segmentos de contornos previamente ordenados y calcula varios detalles sobre cada segmento.
 * Entre las m�tricas calculadas est�n el n�mero de contornos por segmento, la orientaci�n (horizontal o vertical)
 * de cada contorno, la relaci�n del �rea de cada contorno respecto al �rea de una cuarta parte de la imagen,
 * y la relaci�n entre �reas de los contornos si un segmento tiene exactamente dos contornos.
 *
 * @param orderedSegments Un vector de segmentos, donde cada segmento es un conjunto de contornos detectados
 *                        que est�n organizados en 4 grupos en funci�n de su posici�n en la imagen.
 * @param image La imagen original, que se utiliza para calcular la relaci�n de �reas de los contornos.
 *
 * @return std::vector<SegmentInfo> Un vector de estructuras `SegmentInfo` donde cada estructura contiene
 *         la informaci�n detallada sobre un segmento de contornos. Cada estructura incluye:
 *         - El n�mero de contornos en el segmento.
 *         - La orientaci�n de cada contorno (horizontal o vertical).
 *         - Las relaciones de �rea de los contornos.
 *         - La relaci�n entre �reas si hay exactamente 2 contornos en el segmento.
 */
std::vector<SegmentInfo> DeteccionCodigos::getSegmentInfo(const std::vector<std::vector<std::vector<Point>>> &orderedSegments, const Mat &image) {
    // Paso 1: Crear un vector para almacenar la informaci�n de los segmentos
    std::vector<SegmentInfo> segmentInfoList;

    // Paso 2: Iterar sobre cada segmento
    for (const auto &segment : orderedSegments) {
        SegmentInfo info;
        info.numContours = segment.size(); // Paso 2.1: Establecer el n�mero de contornos en el segmento

        // Paso 3: Calcular las orientaciones de los contornos en el segmento
        for (const auto &contour : segment) {
            Rect boundingBox = boundingRect(contour); // Paso 3.1: Obtener el rect�ngulo delimitador del contorno
            string orientation = ( boundingBox.width > boundingBox.height ) ? "horizontal" : "vertical"; // Paso 3.2: Determinar la orientaci�n
            info.orientations.push_back(orientation); // Paso 3.3: Almacenar la orientaci�n
        }

        // Paso 4: Calcular las relaciones de �rea de los contornos
        info.areaRatios = getAreaRatio(segment, image); // Paso 4.1: Llamar a `getAreaRatio` para obtener las relaciones de �rea

        // Paso 5: Calcular la relaci�n de �reas si hay exactamente 2 contornos
        if (info.numContours == 2) {
            double area1 = contourArea(segment[0]); // Paso 5.1: Calcular el �rea del primer contorno
            double area2 = contourArea(segment[1]); // Paso 5.2: Calcular el �rea del segundo contorno
            info.areaRatioRelation = ( area1 / area2 ); // Paso 5.3: Calcular la relaci�n entre las �reas
        }
        else {
            info.areaRatioRelation = -1; // Paso 5.4: Usar -1 para indicar que no es aplicable si no hay exactamente 2 contornos
        }

        // Paso 6: A�adir la informaci�n del segmento a la lista
        segmentInfoList.push_back(info);
    }

    // Paso 7: Devolver el vector con la informaci�n de todos los segmentos
    return segmentInfoList;
}


/**
 * @brief Decodifica el n�mero representado por los segmentos de contornos en la imagen.
 *
 * Esta funci�n interpreta la informaci�n de cada segmento de contornos (n�mero de contornos, orientaciones,
 * relaci�n de �reas) para decodificar un n�mero en formato de cadena de 4 d�gitos. La decodificaci�n se basa en una l�gica
 * definida por las caracter�sticas de los contornos (por ejemplo, n�mero de contornos, orientaci�n, y relaci�n de �reas).
 *
 * @param segmentInfo Un vector de objetos `SegmentInfo` que contienen informaci�n sobre los segmentos,
 *                    tales como el n�mero de contornos, sus orientaciones, relaciones de �rea, etc.
 *
 * @return std::string Un n�mero decodificado representado como una cadena de caracteres. Si la decodificaci�n no es
 *                     posible en un segmento, se usa el car�cter 'X'. Si no hay suficiente informaci�n, tambi�n se devuelve 'X'.
 */
std::string DeteccionCodigos::decodeNumber(const std::vector<SegmentInfo> &segmentInfo) {
    std::string segmentNumber;

    // Paso 1: Iterar sobre los 4 segmentos
    for (int i = 0; i < 4; ++i) {
        // Paso 2: Validar si el segmento tiene informaci�n
        if (i >= segmentInfo.size()) {
            segmentNumber += 'X';  // Si no hay informaci�n en el segmento, se asigna 'X'
            continue;
        }

        const SegmentInfo &info = segmentInfo[i];
        size_t numContours = info.numContours;  // Paso 3: N�mero de contornos en el segmento
        const std::vector<std::string> &orientations = info.orientations;  // Paso 4: Orientaciones de los contornos
        const std::vector<double> &areaRatios = info.areaRatios;  // Paso 5: Relaciones de �rea de los contornos
        double areaRatioRelation = info.areaRatioRelation;  // Paso 6: Relaci�n entre las �reas si hay exactamente 2 contornos

        // Paso 7: Decodificar el n�mero basado en el n�mero de contornos
        if (numContours == 0) {
            segmentNumber += '0';  // Si no hay contornos, se asigna el d�gito '0'
        }
        else if (numContours == 1) {
            // Paso 8: Si hay un solo contorno
            if (orientations[0] == "horizontal") {
                segmentNumber += '8';  // Si es horizontal, asigna el d�gito '8'
            }
            else {
                if (areaRatios[0] < 0.15) {
                    segmentNumber += '1';  // Si el �rea es peque�a, asigna el d�gito '1'
                }
                else {
                    segmentNumber += '5';  // Si el �rea es mayor, asigna el d�gito '5'
                }
            }
        }
        else if (numContours == 2) {
            // Paso 9: Si hay dos contornos
            if (orientations[0] == "horizontal") {
                if (areaRatioRelation > 1.2) {
                    segmentNumber += '7';  // Si la relaci�n de �reas es grande, asigna el d�gito '7'
                }
                else if (areaRatioRelation < 0.8) {
                    segmentNumber += '9';  // Si la relaci�n de �reas es peque�a, asigna el d�gito '9'
                }
                else {
                    segmentNumber += '3';  // Si la relaci�n de �reas est� en el medio, asigna el d�gito '3'
                }
            }
            else {
                if (areaRatioRelation > 1.2) {
                    segmentNumber += '6';  // Si la relaci�n de �reas es grande, asigna el d�gito '6'
                }
                else if (areaRatioRelation < 0.8) {
                    segmentNumber += '4';  // Si la relaci�n de �reas es peque�a, asigna el d�gito '4'
                }
                else {
                    segmentNumber += '2';  // Si la relaci�n de �reas est� en el medio, asigna el d�gito '2'
                }
            }
        }
        else {
            segmentNumber += 'X';  // Si no hay un n�mero de contornos esperado, asigna 'X'
        }
    }

    // Paso 10: Retornar el n�mero decodificado como cadena
    return segmentNumber;
}



/**
 * @brief Muestra las im�genes de los c�digos emparejados, destacando los contornos rojos y verdes con sus respectivos c�digos decodificados.
 *
 * Esta funci�n lleva a cabo un proceso de segmentaci�n y decodificaci�n de los contornos rojos y verdes en la imagen.
 * Se extraen los contornos de ambas m�scaras, se emparejan, y luego se recortan las �reas correspondientes.
 * Despu�s, la funci�n procesa cada imagen recortada para decodificar el n�mero representado por los contornos.
 * Finalmente, se dibujan los cuadros delimitadores (bounding boxes) y los n�meros decodificados en la imagen original.
 *
 * @param imagen La imagen original sobre la cual se procesan los contornos.
 *               Esta imagen es utilizada para realizar la segmentaci�n y para mostrar los resultados finales.
 *
 * @return Mat La imagen con los cuadros delimitadores de los contornos emparejados y los c�digos decodificados
 *             visualizados sobre ella.
 */
Mat DeteccionCodigos::getSegmentedImage(const Mat &imagen) {
    // Paso 1: Crear una copia de la imagen original para modificarla sin alterar la original
    Mat copiaImagen = imagen.clone();

    /// ETAPA SEGMENTACI�N ///

    // Paso 2: Aplicar un filtro de desenfoque para reducir el ruido
    Mat blurImage = BlurImage(imagen, 7);

    // Paso 3: Convertir la imagen a espacio de color HSV para una mejor segmentaci�n
    Mat hsvImage = convertHSVImage(blurImage);

    // Paso 4: Convertir la imagen a escala de grises para facilitar el procesamiento
    Mat grayImage = convertGrayImage(blurImage);

    // Paso 5: Obtener las m�scaras para los colores rojo y verde en la imagen
    Mat redMask = getRedMask(hsvImage);
    Mat greenMask = getGreenMask(hsvImage);

    // Paso 6: Aplicar las m�scaras sobre la imagen original para aislar las �reas rojas y verdes
    redMask = applyMaskToImage(grayImage, redMask);
    greenMask = applyMaskToImage(grayImage, greenMask);

    // Paso 7: Encontrar los contornos filtrados en las im�genes con las m�scaras aplicadas
    std::vector<std::vector<Point>> redContours = findFilteredContours(redMask);
    std::vector<std::vector<Point>> greenContours = findFilteredContours(greenMask);

    // Paso 8: Extraer la informaci�n relevante de los contornos encontrados
    std::vector<ContourInfo> redContoursInfo = extractContourInfo(redContours);
    std::vector<ContourInfo> greenContoursInfo = extractContourInfo(greenContours);

    // Paso 9: Emparejar los contornos rojos y verdes
    std::vector<std::pair<ContourInfo, ContourInfo>> matchedContours = matchContours(redContoursInfo, greenContoursInfo);

    // Paso 10: Recortar las regiones de inter�s de la imagen (bounding boxes) de los contornos emparejados
    std::vector<Mat> extractedImages = cutBoundingBox(matchedContours, imagen);

    /// ETAPA DECODIFICACI�N ///

    // Paso 11: Procesar las im�genes recortadas para decodificar los c�digos
    std::vector<std::string> decodedCodes;
    for (size_t i = 0; i < extractedImages.size(); ++i) {
        // Paso 12: Convertir cada imagen recortada a escala de grises
        extractedImages[i] = convertGrayImage(extractedImages[i]);

        // Paso 13: Aplicar un filtro gaussiano para reducir el ruido en las im�genes recortadas
        extractedImages[i] = BlurImage(extractedImages[i], 11);

        // Paso 14: Aplicar un umbral para binarizar la imagen y resaltar los contornos
        Mat thresholded = thresholdImage(extractedImages[i], 2);

        // Paso 15: Obtener los contornos de la imagen binarizada
        std::vector<std::vector<Point>> contours = getContours(thresholded, extractedImages[i]);

        // Paso 16: Separar los contornos en segmentos seg�n su posici�n en la imagen
        std::vector<std::vector<std::vector<Point>>> segments = separateContoursBySegments(contours, extractedImages[i].cols);

        // Paso 17: Ordenar los contornos dentro de cada segmento para facilitar la decodificaci�n
        std::vector<std::vector<std::vector<Point>>> orderedSegments = orderContours(segments);

        // Paso 18: Obtener informaci�n detallada sobre los segmentos de contornos
        std::vector<SegmentInfo> segmentInfo = getSegmentInfo(orderedSegments, extractedImages[i]);

        // Paso 19: Decodificar el n�mero representado por los contornos
        std::string segmentNumber = decodeNumber(segmentInfo);

        // Paso 20: Almacenar el n�mero decodificado
        decodedCodes.push_back(segmentNumber);
    }

    // Paso 21: Dibujar los cuadros delimitadores y los c�digos decodificados sobre la imagen original
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

        // Paso 26: Dibujar el c�digo decodificado cerca de la bounding box
        std::string numero = i < decodedCodes.size() ? decodedCodes[i] : "X"; // Si no hay n�mero, usar "X"
        putText(copiaImagen, numero, Point(boundingBox.x, boundingBox.y - 10), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2);
    }

    // Paso 27: Retornar la imagen con los resultados visualizados
    return copiaImagen;
}



