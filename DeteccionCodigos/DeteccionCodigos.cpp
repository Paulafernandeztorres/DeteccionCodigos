#include "DeteccionCodigos.h"

DeteccionCodigos::DeteccionCodigos(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    camera = new CVideoAcquisition("rtsp://admin:admin@192.168.1.101:8554/profile0");

    qDebug() << "Conectando con la c�mara...";

    // Configurar el bot�n como checkable
    ui.btnStop->setCheckable(true);
    ui.btnRecord->setCheckable(true);

    // Conectar la se�al del bot�n a la funci�n de detener captura
	connect(ui.btnRecord, SIGNAL(clicked(bool)), this, SLOT(RecordButton(bool)));
    connect(ui.btnStop, SIGNAL(clicked(bool)), this, SLOT(StropButton(bool)));
    connect(ui.btnProcesarImagen, SIGNAL(clicked(bool)), this, SLOT(DecodificarCodigoDeBarras()));
	connect(ui.btnSaveImage, SIGNAL(clicked(bool)), this, SLOT(SaveImageButton()));
}

// Destructor
DeteccionCodigos::~DeteccionCodigos()
{}

// Funci�n para mostrar la imagen en la interfaz
void DeteccionCodigos::showImageInLabel() {
    // Crear un temporizador para actualizar la imagen cada 10ms
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateImage()));
    timer->start(5);
}

// Funci�n para actualizar la imagen
void DeteccionCodigos::updateImage() {
    imgcapturada = camera->getImage();
    // Convertir la imagen a escala de grises
    Mat gris;
    cvtColor(imgcapturada, gris, cv::COLOR_BGR2GRAY);

    // Aplicar un filtro Mediana para reducir el ruido
    Mat suavizada;
	medianBlur(gris, suavizada, 11);
    /*GaussianBlur(gris, suavizada, cv::Size(5, 5), 0);*/

    //// Aplicar un umbral adaptativo para binarizar la imagen
    Mat umbralizada;
    adaptiveThreshold(suavizada, umbralizada, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 11, 2);

    // Detectar bordes usando Canny
    Mat bordes;
    Canny(suavizada, bordes, 0, 50, 3, false);

	// Buscar los contronos de la imagen
	findContours(bordes, contornos, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

	// Dibujar los contornos en la imagen
	drawContours(imgcapturada, contornos, -1, Scalar(0, 0, 255), 2);

	// Convertir la imagen a formato QImage
	QImage qimg(bordes.data, bordes.cols, bordes.rows, bordes.step, QImage::Format_Grayscale8);

    // Espejo de la imagen
    /*qimg = qimg.mirrored(true, false);*/

    // Redimensionar la imagen para que se ajuste al tama�o de la etiqueta
    qimg = qimg.scaled(ui.label->size(), Qt::KeepAspectRatio, Qt::FastTransformation);

    // Mostrar la imagen en la interfaz
    ui.label->setPixmap(QPixmap::fromImage(qimg));
}

void DeteccionCodigos::RecordButton(bool captura) {
	qDebug() << "Boton Record pulsado: " << captura;
	// Si el bot�n est� pulsado est� activo
	if (captura) {
		// si no hay temporizador, crear uno
        if (!timer) {
			camera->startStopCapture(true);
            showImageInLabel();
        }  
        else {
			timer->start(5);
        }
	}
	else {
		// eliminar el temporizador
		timer->stop();
		// borrar la imagen
		ui.label->clear();
	}
}

void DeteccionCodigos::StropButton(bool captura) {
	qDebug() << "Boton Stop pulsado: " << captura;
	// Si el bot�n est� pulsado est� activo
    camera->startStopCapture(!captura);
}

void DeteccionCodigos::ProcesarImagen()
{
    // Convertir la imagen a escala de grises y aplicar un filtro Gaussiano para suavizar la imagen
    Mat gris, suavizada;
    cvtColor(imgcapturada, gris, cv::COLOR_BGR2GRAY);
    GaussianBlur(gris, suavizada, cv::Size(5, 5), 0);

    // Aplicar un umbral (threshold) adaptativo
    Mat umbralizada;
    threshold(suavizada, umbralizada, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);

    // Mostrar la imagen procesada
    imshow("Imagen Binarizada", umbralizada);

    // Convertir la imagen original a espacio de color HSV
    Mat hsv;
    cvtColor(imgcapturada, hsv, cv::COLOR_BGR2HSV);

    // Crear m�scaras para los colores rojo y verde
    Mat mascaraRoja, mascaraRojaAlta, mascaraVerde, mascaraCombinada;
    inRange(hsv, Scalar(0, 100, 100), Scalar(10, 255, 255), mascaraRoja); // Rango para rojo en HSV
    inRange(hsv, Scalar(160, 100, 100), Scalar(179, 255, 255), mascaraRojaAlta);
    bitwise_or(mascaraRoja, mascaraRojaAlta, mascaraRoja);

    inRange(hsv, Scalar(35, 100, 100), Scalar(85, 255, 255), mascaraVerde); // Ajuste m�s preciso para verde en HSV

    // Aplicar una operaci�n de cerrado (morfolog�a) para rellenar los huecos
    static const Mat elemento = getStructuringElement(MORPH_RECT, Size(5, 5)); // Tama�o del elemento estructurante
    morphologyEx(mascaraRoja, mascaraRoja, MORPH_CLOSE, elemento);
    morphologyEx(mascaraVerde, mascaraVerde, MORPH_CLOSE, elemento);

    // Mostrar las im�genes procesadas
    imshow("Zona Roja", mascaraRoja);
    imshow("Zona Verde", mascaraVerde);

    // Sumar las m�scaras roja y verde (realizar una operaci�n OR)
    bitwise_or(mascaraRoja, mascaraVerde, mascaraCombinada);

    // Mostrar la imagen combinada
    imshow("Zona Combinada", mascaraCombinada);
}

void DeteccionCodigos::DecodificarCodigoDeBarras()
{
    int codigoContador = 0; // Contador para los c�digos de barras detectados

    // Filtrar contornos para encontrar posibles c�digos de barras
    for (const auto &contorno : contornos) {
        Rect rect = boundingRect(contorno);
        float aspectRatio = (float)rect.width / (float)rect.height;

        // Filtrar por aspecto y tama�o
        if (aspectRatio > 2.0 && rect.width > 100 && rect.height > 20) {
            // Extraer la regi�n de inter�s (ROI)
            Mat roi = imgcapturada(rect);

            // Mostrar la regi�n de inter�s en una ventana separada
            std::string windowName = "Codigo de Barras Detectado " + std::to_string(codigoContador);
            imshow(windowName, roi);

            // Incrementar el contador
            codigoContador++;
        }
    }
}

void DeteccionCodigos::SaveImageButton()
{
	// comprobar si hay texto en lineEdit
	if (ui.lineEdit->text().isEmpty()) {
		// si no hay texto, mostrar un mensaje de error
		QMessageBox::warning(this, "Error", "Introduce un nombre para la imagen");
	}
	else {
		// si hay texto, guardar la imagen
		contadorImagen++;
		// Nombre de la imagen a guardar (nombre +G1+ contador)
		QString filename = ui.lineEdit->text() + "_G1_" + QString::number(contadorImagen);
		// Ruta de la imagen en Resource Files/Imagenes dentro del proyecto
		QString rutaImagen = "C:/Users/migue/source/repos/DeteccionCodigos/Imagenes/" + filename + ".jpg";
		imwrite(rutaImagen.toStdString(), imgcapturada);
		// Mostrar un mensaje de informaci�n con el nombre de la imagen guardada
		QMessageBox::information(this, "Imagen guardada", "Imagen guardada con nombre: " + filename);
	}
}