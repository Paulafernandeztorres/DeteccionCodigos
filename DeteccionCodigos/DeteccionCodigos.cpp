#include "DeteccionCodigos.h"

DeteccionCodigos::DeteccionCodigos(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    camera = new CVideoAcquisition(0);
    // camera = new CVideoAcquisition("rtsp://admin:admin@192.168.1.101:8554/profile0");

    qDebug() << "Conectando con la cámara...";

    // Configurar el botón como checkable
    ui.btnStop->setCheckable(true);
    ui.btnRecord->setCheckable(true);
	ui.btnDecodeImage->setCheckable(true);
	ui.btnRedMask->setCheckable(true);
	ui.btnGreenMask->setCheckable(true);

	// Conectar los botones a sus respectivas funciones
	connect(ui.btnRecord, SIGNAL(clicked(bool)), this, SLOT(RecordButton(bool)));
    connect(ui.btnStop, SIGNAL(clicked(bool)), this, SLOT(StropButton(bool)));
    connect(ui.btnDecodeImage, SIGNAL(clicked(bool)), this, SLOT(DecodeImagen(bool)));
    connect(ui.btnRedMask, SIGNAL(clicked(bool)), this, SLOT(ViewRedMask(bool)));
    connect(ui.btnGreenMask, SIGNAL(clicked(bool)), this, SLOT(ViewGreenMask(bool)));
	connect(ui.btnSaveImage, SIGNAL(clicked()), this, SLOT(SaveDecodedCode()));
}

// Destructor
DeteccionCodigos::~DeteccionCodigos()
{
	// Eliminar la cámara
	delete camera;
}

// Función para actualizar la imagen
void DeteccionCodigos::UpdateImage() {
    imgcapturada = camera->getImage();
    // Procesar la imagen según el modo actual
    QImage qimg;
    switch (currentMode) {
        case Normal:
			// Mostrar la imagen capturada (la imagen capturada es igual a la imagen final)
			imagenFinal = imgcapturada;
			// Espejo horizontal de la imagen
			flip(imagenFinal, imagenFinal, 1);
            qimg = QImage((const unsigned char *)( imagenFinal.data ),
                          imagenFinal.cols,
                          imagenFinal.rows,
                          imagenFinal.step,
                          QImage::Format_BGR888);
            break;
        case Decoded:
            // Llamar a la función de decodificación y procesar la imagen
            // Ejemplo pasar la imagen a escala de grises
            imagenFinal = convertGrayImage(imgcapturada);
			// Espejo horizontal de la imagen
			flip(imagenFinal, imagenFinal, 1);
            qimg = QImage((const unsigned char *)( imagenFinal.data ),
                          imagenFinal.cols,
                          imagenFinal.rows,
                          imagenFinal.step,
                          QImage::Format_Grayscale8);
            break;
        case RedMask:
        {
            // Generar y mostrar la máscara roja
			// Filtrar la imagen para suavizarla y reducir el ruido
			imagenFinal = BlurImage(imgcapturada, 7);
			// Convertir la imagen a hsv
			imagenFinal = convertHSVImage(imagenFinal);
			// Obtener la máscara roja
			imagenFinal = getRedMask(imagenFinal);
			// Aplicar la mascara a la imagen original
			imagenFinal = applyMaskToImage(imgcapturada, imagenFinal);
			// Espejo horizontal de la imagen
			flip(imagenFinal, imagenFinal, 1);
            qimg = QImage((const unsigned char *)( imagenFinal.data ),
                          imagenFinal.cols,
                          imagenFinal.rows,
                          imagenFinal.step,
                          QImage::Format_BGR888);
            break;
        }
        case GreenMask:
        {
            // Generar y mostrar la máscara verde
			// Filtrar la imagen para suavizarla y reducir el ruido
			imagenFinal = BlurImage(imgcapturada, 7);
			// Convertir la imagen a hsv
			imagenFinal = convertHSVImage(imagenFinal);
			// Obtener la máscara verde
			imagenFinal = getGreenMask(imagenFinal);
			// Aplicar la mascara a la imagen original
			imagenFinal = applyMaskToImage(imgcapturada, imagenFinal);
            // Espejo horizontal de la imagen
            flip(imagenFinal, imagenFinal, 1);
            qimg = QImage((const unsigned char *)( imagenFinal.data ),
                          imagenFinal.cols,
                          imagenFinal.rows,
                          imagenFinal.step,
                          QImage::Format_BGR888);
            break;
        }
    }
    // Mostrar la imagen procesada en el label
    ui.label->setPixmap(QPixmap::fromImage(qimg));
}

void DeteccionCodigos::RecordButton(bool captura) {
    qDebug() << "Boton Record pulsado: " << captura;
    if (captura) {
        if (!timer) {
            timer = new QTimer(this);
            connect(timer, &QTimer::timeout, this, &DeteccionCodigos::UpdateImage);
            timer->start(30); // Actualizar cada 30 ms
        }
        camera->startStopCapture(true);
    }
    else {
        if (timer) {
            timer->stop();
            delete timer;
            timer = nullptr;
        }
        camera->startStopCapture(false);
        ui.label->clear();
    }
}

void DeteccionCodigos::StropButton(bool captura) {
	qDebug() << "Boton Stop pulsado: " << captura;
	// Si el botón está pulsado está activo
    camera->startStopCapture(!captura);
}

void DeteccionCodigos::DecodeImagen(bool captura) {
    if (captura) {
        qDebug() << "Decodificando imagen...";
        currentMode = Decoded;
    }
    else {
        currentMode = Normal;
    }
}

void DeteccionCodigos::ViewRedMask(bool captura) {
    if (captura) {
        qDebug() << "Mostrando mascara roja...";
        currentMode = RedMask;
    }
    else {
        currentMode = Normal;
    }
}

void DeteccionCodigos::ViewGreenMask(bool captura) {
    if (captura) {
        qDebug() << "Mostrando mascara verde...";
        currentMode = GreenMask;
    }
    else {
        currentMode = Normal;
    }
}

void DeteccionCodigos::SaveDecodedCode()
{
    qDebug() << "Guardando codigo decodificado...";
}

Mat DeteccionCodigos::BlurImage(const Mat& image, uint8_t kernerSsize) {
	// Aplicar el filtro de desenfoque gaussiano
    Mat blurImage;
	GaussianBlur(image, blurImage, Size(kernerSsize, kernerSsize), 0);
	return blurImage;
}

Mat DeteccionCodigos::convertGrayImage(const Mat& image) {
	// Convertir la imagen a escala de grises
	cvtColor(image, grayImage, COLOR_BGR2GRAY);
	return grayImage;
}

Mat DeteccionCodigos::convertHSVImage(const Mat& image) {
	// Convertir la imagen a espacio de color HSV
	cvtColor(image, hsvImage, COLOR_BGR2HSV);
	return hsvImage;
}

Mat DeteccionCodigos::getRedMask(const Mat& image) {
	// Rango para el color rojo (dos segmentos)
	Mat mascaraRoja, mascaraRoja2;
	inRange(image, Scalar(0, 20, 0), Scalar(8, 255, 255), mascaraRoja);
	inRange(image, Scalar(125, 20, 0), Scalar(179, 255, 255), mascaraRoja2);
	// Combinar las dos máscaras
	mascaraRoja = mascaraRoja | mascaraRoja2;
	return mascaraRoja;
}

Mat DeteccionCodigos::getGreenMask(const Mat& image) {
	// Rango para el color verde (ajustado para evitar falsos positivos)
	Mat mascaraVerde;
	inRange(image, Scalar(30, 40, 0), Scalar(90, 255, 190), mascaraVerde);
	return mascaraVerde;
}

Mat DeteccionCodigos::applyMaskToImage(const Mat& image, Mat mask) {
	// Aplicar la máscara a la imagen
	Mat maskedImage;
	image.copyTo(maskedImage, mask);
	return maskedImage;
}

Mat DeteccionCodigos::sobelFilter(const Mat& image, uint8_t kernelSize) {
    Mat img_sobel_x, img_sobel_y, img_sobel, filtered_image;
    // Aplicar el filtro Sobel en la dirección X
    Sobel(image, img_sobel_x, CV_64F, 1, 0, kernelSize);
    // Aplicar el filtro Sobel en la dirección Y
    Sobel(image, img_sobel_y, CV_64F, 0, 1, kernelSize);
    // Calcular la magnitud del gradiente
    magnitude(img_sobel_x, img_sobel_y, img_sobel);
    // Normalizar la imagen resultante
    normalize(img_sobel, img_sobel, 0, 255, NORM_MINMAX, CV_8U);
    // Aplicar umbralización binaria
    threshold(img_sobel, filtered_image, 30, 255, THRESH_BINARY);
    return filtered_image;
}

std::vector<std::vector<Point>> DeteccionCodigos::findFilteredContours(const Mat &image) {
    // Filtro sobel para detectar bordes
    Mat sobelImage = sobelFilter(image, 11);
    // Encontrar contornos en la imagen
    std::vector<std::vector<Point>> contours;
    std::vector<Vec4i> hierarchy;
    findContours(sobelImage, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    // Filtrar los contornos por Area
    std::vector<std::vector<Point>> filteredContours;
    for (const auto &contour : contours) {
        // Calcular el área del contorno
        double area = contourArea(contour);
        // Calcular el aspect ratio del contorno
        Rect boundingBox = boundingRect(contour);

        // calcular 1% del área de image
		double areaImage = image.rows * image.cols;
		double umbralBajoArea = 0.05 * areaImage;
		double umbralAltoArea = 0.25 * areaImage;

        double aspectRatio = static_cast<double>( boundingBox.width ) / boundingBox.height;
        if (area > umbralBajoArea && area < umbralAltoArea && aspectRatio > 0.5 && aspectRatio < 1.3) {
            filteredContours.push_back(contour);
        }
    }
    // Nunero de contornos encontrados
	qDebug() << "Numero de contornos encontrados: " << filteredContours.size();

    /// Vector de vectores de puntos que representan los contornos.
    return filteredContours;
}

/// Extrae información de los contornos proporcionados.
std::vector<ContourInfo> DeteccionCodigos::extractContourInfo(const std::vector<std::vector<Point>> &contours) {
    // Vector de estructuras ContourInfo con la información de cada contorno.
    std::vector<ContourInfo> contour_info;
    for (const auto &contour : contours) {
        ContourInfo info;
        // Calcular el área del contorno
        info.area = contourArea(contour);
        // Obtener el rectángulo delimitador del contorno
        Rect boundingBox = boundingRect(contour);

        // Calcular el perímetro del contorno
        info.perimeter =arcLength(contour, true);

        // Obtener las esquinas reales del contorno
        RotatedRect rect = minAreaRect(contour);
        Point2f box[4];
        rect.points(box);
        for (int i = 0; i < 4; i++) {
            info.corners.push_back(Point(static_cast<int>( box[i].x ), static_cast<int>( box[i].y )));
        }
        // Calcular el centro del contorno
        info.center = Point2f((boundingBox.x + boundingBox.width) / 2.0,
                              (boundingBox.y + boundingBox.height) / 2.0 );
        // Calcular el ancho y alto del contorno y redondear a 3 decimales
        info.width = boundingBox.width;
        info.height = boundingBox.height;
        // Calcular la relación de aspecto del contorno y redondear a 3 decimales
        info.aspect_ratio = ( boundingBox.height != 0 ) ?  boundingBox.width / static_cast<float>( boundingBox.height ) : 0;
        // Calcular el ángulo del contorno y redondear a 3 decimales
        info.angle = rect.angle;

        contour_info.push_back(info);
		// Información del contorno
		qDebug() << "Area: " << info.area 
            << " Perimetro: " << info.perimeter 
			<< " Centro: " << info.center.x << ", " << info.center.y
            << " Ancho: " << info.width 
            << " Alto: " << info.height
            << " Relacion de aspecto: " 
            << info.aspect_ratio 
            << " Angulo: " << info.angle;
    }
    return contour_info;
}

std::vector<std::pair<ContourInfo, ContourInfo>> DeteccionCodigos::matchContours(const std::vector<ContourInfo> &redContoursInfo, 
                                                                                 const std::vector<ContourInfo> &greenContoursInfo) {
    std::vector<std::pair<ContourInfo, ContourInfo>> matches;
    std::set<const ContourInfo *> usedRedContours;
    std::set<const ContourInfo *> usedGreenContours;

    for (const auto &redContour : redContoursInfo) {
        // Verificar si este contorno rojo ya está emparejado
        if (usedRedContours.find(&redContour) != usedRedContours.end()) {
            continue;
        }

        const ContourInfo *bestMatch = nullptr;
        double bestAngleMatch = std::numeric_limits<double>::infinity();  // Para encontrar el mejor ángulo
        double bestScore = std::numeric_limits<double>::infinity();       // Para desempatar usando área y perímetro

        for (const auto &greenContour : greenContoursInfo) {
            // Verificar si este contorno verde ya está emparejado
            if (usedGreenContours.find(&greenContour) != usedGreenContours.end()) {
                continue;
            }
            // Calcular distancia entre centros
            double centerDistance = cv::norm(redContour.center - greenContour.center);
            // Descartar si la distancia excede 3.5 veces la anchura del contorno rojo
            if (centerDistance > 3.2 * redContour.width) {
                continue;
            }
            // Calcular diferencia de ángulo
            double angleDiff = std::abs(redContour.angle - greenContour.angle);
            // Descartar si la diferencia de ángulo es mayor a 10 grados
            // if (angleDiff > 20) {
            //     continue;
            // }
            // Calcular diferencias adicionales para desempatar
            double areaDiff = std::abs(redContour.area - greenContour.area);
            double perimeterDiff = std::abs(redContour.perimeter - greenContour.perimeter);
            // Crear puntaje de desempate
            double score = areaDiff + perimeterDiff;
            // Actualizar el mejor match
            if (angleDiff < bestAngleMatch || ( angleDiff == bestAngleMatch && score < bestScore )) {
                bestAngleMatch = angleDiff;
                bestScore = score;
                bestMatch = &greenContour;
            }
        }
        if (bestMatch) {
            matches.emplace_back(redContour, *bestMatch);
            usedRedContours.insert(&redContour);
            usedGreenContours.insert(bestMatch);
        }
    }
	// Información de los contornos emparejados
	qDebug() << "Numero de contornos emparejados: " << matches.size();
    return matches;
}

std::vector<Mat> DeteccionCodigos::cutBoundingBox(const std::vector<pair<ContourInfo, ContourInfo>> &matchedContours, const Mat &image) {
    std::vector<Mat> extractedImages;

    for (const auto &match : matchedContours) {
        const ContourInfo &redContour = match.first;
        const ContourInfo &greenContour = match.second;

        // Obtener los puntos de los contornos
        std::vector<Point> redPoints = redContour.corners;
        std::vector<Point> greenPoints = greenContour.corners;
        // Combinar los puntos de ambos contornos
        std::vector<Point> allPoints;
        allPoints.insert(allPoints.end(), redPoints.begin(), redPoints.end());
        allPoints.insert(allPoints.end(), greenPoints.begin(), greenPoints.end());

        // Calcular la bounding box que contenga todos los puntos
        Rect boundingBox = boundingRect(allPoints);
		// Calcular el centro de la bounding box
		Point boundingBoxCenter = Point(boundingBox.x + boundingBox.width / 2, boundingBox.y + boundingBox.height / 2);

        // extraer los centros de los contornos
		Point redCenter = redContour.center;
		Point greenCenter = greenContour.center;

		//// Dibujar los centros y la línea entre ellos en la imagen
		//circle(image, redCenter, 5, Scalar(0, 0, 255), -1);
		//circle(image, greenCenter, 5, Scalar(0, 255, 0), -1);

		//// Dibujar la línea entre los centros
		//line(image, redCenter, greenCenter, Scalar(255, 0, 0), 2);

		//// Mostrar la imagen con los centros y la línea
		//imshow("Centros y línea", image);

        // Calcular el ángulo entre los centros
        double x0 = redContour.center.x;
        double y0 = redContour.center.y;
        double x1 = greenContour.center.x;
        double y1 = greenContour.center.y;
        double angle = atan2(y1 - y0, x1 - x0) * 180 / CV_PI;

        // Rotar la imagen original para que la línea entre los centros sea horizontal
        Mat M = getRotationMatrix2D(boundingBoxCenter, angle, 1);
        Mat rotatedImage;
        warpAffine(image, rotatedImage, M, image.size());

        // Rotar los puntos de los contornos
        std::vector<Point> transformedPoints;
        for (const Point &pt : allPoints) {
            // Transformar cada punto con la matriz de rotación
            double xNew = M.at<double>(0, 0) * pt.x + M.at<double>(0, 1) * pt.y + M.at<double>(0, 2);
            double yNew = M.at<double>(1, 0) * pt.x + M.at<double>(1, 1) * pt.y + M.at<double>(1, 2);
            transformedPoints.emplace_back(cvRound(xNew), cvRound(yNew));
        }

        // Calcular la nueva bounding box después de la transformación
        Rect transformedBoundingBox = boundingRect(transformedPoints);

		// Recortar la bounding box de la imagen rotada
		Mat extractedImage = rotatedImage(transformedBoundingBox);

		// Añadir la imagen recortada al vector de imágenes
		extractedImages.push_back(extractedImage);
    }
    return extractedImages;
}

// Función mostrar las imagenes de los códigos emparejados
void DeteccionCodigos::extractCodes(const Mat &imagenRoja, const Mat &imagenVerde) {
	// Encontrar contornos filtrados en las imágenes
	std::vector<std::vector<Point>> redContours = findFilteredContours(imagenRoja);
	std::vector<std::vector<Point>> greenContours = findFilteredContours(imagenVerde);
    // Extraer información de los contornos encontrados
	std::vector<ContourInfo> redContoursInfo = extractContourInfo(redContours);
	std::vector<ContourInfo> greenContoursInfo = extractContourInfo(greenContours);
	// Emparejar los contornos rojos y verdes
	std::vector<std::pair<ContourInfo, ContourInfo>> matchedContours = matchContours(redContoursInfo, greenContoursInfo);
	// Recortar las bounding boxes de los contornos emparejados
	std::vector<Mat> extractedImages = cutBoundingBox(matchedContours, imgcapturada);

	// Mostrar las imágenes recortadas ventanas separadas
	for (size_t i = 0; i < extractedImages.size(); ++i) {
        // Convertir imagen a escala de grises
		extractedImages[i] = convertGrayImage(extractedImages[i]);
		// Aplicar filtro gaussiano para reducir el ruido y mejorar la umbralización
		extractedImages[i] = BlurImage(extractedImages[i], 11);
		Mat thresholded = thresholdImage(extractedImages[i], 2);
		// Obtener los contornos de la imagen
		std::vector<std::vector<Point>> contours = getContours(thresholded, extractedImages[i]);
		// Separar los contornos en segmentos
		std::vector<std::vector<std::vector<Point>>> segments = separateContoursBySegments(contours, extractedImages[i].cols);
		// Ordenar los contornos en cada segmento
		std::vector<std::vector<std::vector<Point>>> orderedSegments = orderContours(segments);
		// Obtener información de los segmentos
		std::vector<SegmentInfo> segmentInfo = getSegmentInfo(orderedSegments, extractedImages[i]);
		// Decodificar el número de cada segmento
		std::string segmentNumber = decodeNumber(segmentInfo);
		// Mostrar el número decodificado
		qDebug() << "Codigo " << i + 1 << ": " << segmentNumber;

		// Mostrar la imagen con los contornos
		imshow("Codigo " + segmentNumber, extractedImages[i]);
	} 
}

Mat DeteccionCodigos::thresholdImage(const Mat &image, int threshold) {
    Mat imageThresholdGaussian;
    adaptiveThreshold(image, imageThresholdGaussian, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 11, threshold);
	// Hacer los bordes más suaves con cierra y erosión
    Mat kernel = Mat::ones(Size(3, 3), CV_8U);
    Mat closeImage;
    morphologyEx(imageThresholdGaussian, closeImage, MORPH_CLOSE, kernel);
    Mat erodeImage;
    erode(closeImage, erodeImage, kernel, Point(-1, -1), 1);
    return erodeImage;
}

pair<std::vector<std::vector<Point>>, std::vector<std::vector<Point>>> DeteccionCodigos::classifyContours(
    const std::vector<std::vector<Point>> &contours, const Size &imageShape) {
    std::vector<std::vector<Point>> squareContours;
    std::vector<std::vector<Point>> rectangularContours;

    for (const auto &contour : contours) {
        double area = contourArea(contour);
        double areaRatio = area / ( imageShape.width * imageShape.height );

        Rect boundingBox = boundingRect(contour);
        double aspectRatio = static_cast<double>( boundingBox.width ) / boundingBox.height;

        // Clasificar contornos en cuadrados o rectángulos
        if (0.5 <= aspectRatio && aspectRatio <= 1.5 && areaRatio > 0.06) {
            squareContours.push_back(contour);
        }
        else {
            rectangularContours.push_back(contour);
        }
    }
    return { squareContours, rectangularContours };
}


std::vector<std::vector<Point>> DeteccionCodigos::filterInsideContours(const std::vector<std::vector<Point>> &contours) {
    std::vector<std::vector<Point>> filteredContours;
    for (size_t i = 0; i < contours.size(); ++i) {
        const std::vector<Point> &contour = contours[i];
        const std::vector<Point> *parentContour = nullptr;
        for (size_t j = 0; j < contours.size(); ++j) {
            if (i == j) 
                continue;
            const std::vector<Point> &otherContour = contours[j];
            bool isInside = true;
            for (const Point &pt : contour) {
                if (pointPolygonTest(otherContour, pt, false) <= 0) {
                    isInside = false;
                    break;
                }
            }
            if (isInside) {
                if (parentContour == nullptr || contourArea(otherContour) > contourArea(*parentContour)) {
                    parentContour = &otherContour;
                }
            }
        }
        if (parentContour == nullptr) {
            filteredContours.push_back(contour);
        }
    }
    return filteredContours;
}

std::vector<std::vector<Point>> DeteccionCodigos::getContours(const Mat &thresholdedImage, const Mat &image) {
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    // Encontrar contornos
    findContours(thresholdedImage, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

    vector<vector<Point>> filteredContours;
    double imageArea = image.rows * image.cols;
    double minInfluence = 0.01;

    for (const auto &contour : contours) {
        double area = contourArea(contour);
        Rect boundingBox = boundingRect(contour);
        double influence = area / imageArea;

        // Filtrar por área e influencia
        if (area >= 200 && area <= 25000 && influence >= minInfluence) {
            // Ignorar contornos cercanos al borde
            if (boundingBox.x > 10 && boundingBox.y > 10 &&
                boundingBox.x + boundingBox.width < image.cols - 10 &&
                boundingBox.y + boundingBox.height < image.rows - 10) {
                filteredContours.push_back(contour);
            }
        }
    }

    // Segmentar contornos
	auto [squareContours, rectangularContours] = classifyContours(filteredContours, image.size());

    // Filtrar contornos que están dentro de otros
    filteredContours = filterInsideContours(rectangularContours);

    return filteredContours;
}

std::vector<std::vector<std::vector<Point>>> DeteccionCodigos::separateContoursBySegments(const std::vector<std::vector<Point>> &contours, int imageWidth) {
    // Crear un vector de 4 segmentos
    std::vector<std::vector<std::vector<Point>>> segments(4);
    for (const auto &contour : contours) {
        Rect boundingBox = boundingRect(contour); // Obtener el rectángulo delimitador
        int centerX = boundingBox.x + boundingBox.width / 2; // Calcular el centro en X

        // Determinar a qué segmento pertenece
        int segment = centerX / ( imageWidth / 4 );
        if (segment >= 0 && segment < 4) {
            segments[segment].push_back(contour);
        }
    }
    return segments;
}

std::vector<std::vector<std::vector<Point>>> DeteccionCodigos::orderContours(const std::vector<std::vector<std::vector<Point>>> &segments) {
    std::vector<std::vector<std::vector<Point>>> orderedSegments;

    for (const auto &segment : segments) {
        if (segment.size() <= 1) {
            // Si el segmento tiene 0 o 1 contorno, no se ordena
            orderedSegments.push_back(segment);
        }
        else if (segment.size() == 2) {
            // Si el segmento tiene exactamente 2 contornos
            Rect boundingBox1 = boundingRect(segment[0]);
            Rect boundingBox2 = boundingRect(segment[1]);

            bool isWide1 = boundingBox1.width > boundingBox1.height;
            bool isWide2 = boundingBox2.width > boundingBox2.height;

            std::vector<std::vector<Point>> sortedSegment = segment;

            if (isWide1 && isWide2) {
                // Ordenar por coordenada Y
                sort(sortedSegment.begin(), sortedSegment.end(),
                     [](const std::vector<Point> &a, const std::vector<Point> &b) {
                    return boundingRect(a).y < boundingRect(b).y;
                });
            }
            else if (!isWide1 && !isWide2) {
                // Ordenar por coordenada X
                sort(sortedSegment.begin(), sortedSegment.end(),
                     [](const std::vector<Point> &a, const std::vector<Point> &b) {
                    return boundingRect(a).x < boundingRect(b).x;
                });
            }
            orderedSegments.push_back(sortedSegment);
        }
    }
    return orderedSegments;
}

std::vector<double> DeteccionCodigos::getAreaRatio(const std::vector<std::vector<Point>> &contours, const Mat &image) {
    // Calcular el área total de la imagen dividida por 4
    double imageArea = ( image.rows * image.cols ) / 4.0;
    std::vector<double> areaRatios;
    for (const auto &contour : contours) {
        // Calcular el área del contorno
        double area = contourArea(contour);
        // Calcular la relación del área del contorno con respecto al área de la imagen
        double areaRatio = area / imageArea;
        // Redondear el resultado y almacenarlo
        areaRatios.push_back(areaRatio);
    }
    return areaRatios;
}

std::vector<SegmentInfo> DeteccionCodigos::getSegmentInfo(const std::vector<std::vector<std::vector<Point>>> &orderedSegments, const Mat &image) {
    std::vector<SegmentInfo> segmentInfoList;

    for (const auto &segment : orderedSegments) {
        SegmentInfo info;
        info.numContours = segment.size();

        // Calcular las orientaciones
        for (const auto &contour : segment) {
            Rect boundingBox = boundingRect(contour);
            string orientation = ( boundingBox.width > boundingBox.height ) ? "horizontal" : "vertical";
            info.orientations.push_back(orientation);
        }

        // Calcular las relaciones de área
        info.areaRatios = getAreaRatio(segment, image);

        // Relación de las áreas si hay exactamente 2 contornos
        if (info.numContours == 2) {
            double area1 = contourArea(segment[0]);
            double area2 = contourArea(segment[1]);
            info.areaRatioRelation = ( area1 / area2 );
        }
        else {
            info.areaRatioRelation = -1; // Usar -1 como indicador de "no aplicable"
        }
        segmentInfoList.push_back(info);
    }
    return segmentInfoList;
}

std::string DeteccionCodigos::decodeNumber(const std::vector<SegmentInfo> &segmentInfo) {
    std::string segmentNumber;

    for (int i = 0; i < 4; ++i) {
        // Validar si el segmento tiene información
        if (i >= segmentInfo.size()) {
            segmentNumber += 'X';
            continue;
        }

        const SegmentInfo &info = segmentInfo[i];
        size_t numContours = info.numContours;
        const std::vector<std::string> &orientations = info.orientations;
        const std::vector<double> &areaRatios = info.areaRatios;
        double areaRatioRelation = info.areaRatioRelation;

        if (numContours == 0) {
            segmentNumber += '0';
        }
        else if (numContours == 1) {
            if (orientations[0] == "horizontal") {
                segmentNumber += '8';
            }
            else {
                if (areaRatios[0] < 0.15) {
                    segmentNumber += '1';
                }
                else {
                    segmentNumber += '5';
                }
            }
        }
        else if (numContours == 2) {
            if (orientations[0] == "horizontal") {
                if (areaRatioRelation > 1.2) {
                    segmentNumber += '7';
                }
                else if (areaRatioRelation < 0.8) {
                    segmentNumber += '9';
                }
                else {
                    segmentNumber += '3';
                }
            }
            else {
                if (areaRatioRelation > 1.2) {
                    segmentNumber += '6';
                }
                else if (areaRatioRelation < 0.8) {
                    segmentNumber += '4';
                }
                else {
                    segmentNumber += '2';
                }
            }
        }
        else {
            segmentNumber += 'X';
        }
    }
    return segmentNumber;
}


