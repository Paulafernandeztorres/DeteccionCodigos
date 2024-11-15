#include "DeteccionCodigos.h"

DeteccionCodigos::DeteccionCodigos(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    camera = new CVideoAcquisition("rtsp://admin:admin@192.168.1.101:8554/profile0");

    qDebug() << "Conectando con la cámara...";

    // Configurar el botón como checkable
    ui.btnStop->setCheckable(true);
    ui.btnRecord->setCheckable(true);

    // Conectar la señal del botón a la función de detener captura
	connect(ui.btnRecord, SIGNAL(clicked(bool)), this, SLOT(RecordButton(bool)));
    connect(ui.btnStop, SIGNAL(clicked(bool)), this, SLOT(StropButton(bool)));
    connect(ui.btnProcesarImagen, SIGNAL(clicked(bool)), this, SLOT(ProcesarImagen()));
	connect(ui.btnSaveImage, SIGNAL(clicked(bool)), this, SLOT(SaveImageButton()));
}

// Destructor
DeteccionCodigos::~DeteccionCodigos()
{}

// Función para mostrar la imagen en la interfaz
void DeteccionCodigos::showImageInLabel() {
    // Crear un temporizador para actualizar la imagen cada 10ms
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateImage()));
    timer->start(5);
}

// Función para actualizar la imagen
void DeteccionCodigos::updateImage() {
    imgcapturada = camera->getImage();
    QImage qimg = QImage((const unsigned char *)( imgcapturada.data ), imgcapturada.cols, imgcapturada.rows, imgcapturada.step, QImage::Format_BGR888);

    // Espejo de la imagen
    qimg = qimg.mirrored(true, false);

    // Redimensionar la imagen para que se ajuste al tamaño de la etiqueta
    qimg = qimg.scaled(ui.label->size(), Qt::KeepAspectRatio, Qt::FastTransformation);

    // Mostrar la imagen en la interfaz
    ui.label->setPixmap(QPixmap::fromImage(qimg));
}

void DeteccionCodigos::RecordButton(bool captura) {
	qDebug() << "Boton Record pulsado: " << captura;
	// Si el botón está pulsado está activo
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

    // Invertir el valor si `startStopCapture` requiere `true` para iniciar y `false` para detener
    camera->startStopCapture(!captura);
}

void DeteccionCodigos::ProcesarImagen()
{
    // Convertir la imagen a escala de grises
    Mat gris;
    cvtColor(imgcapturada, gris, cv::COLOR_BGR2GRAY);

    // Aplicar un filtro Gaussiano para suavizar la imagen
    Mat suavizada;
    GaussianBlur(gris, suavizada, cv::Size(5, 5), 0);

    // Aplicar un umbral (threshold) adaptativo
    Mat umbralizada;
    threshold(suavizada, umbralizada, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);

    // Mostrar la imagen procesada
    namedWindow("Imagen Binarizada", WINDOW_NORMAL); // O WINDOW_AUTOSIZE
    imshow("Imagen Binarizada", umbralizada);
    
    // Convertir la imagen original a espacio de color HSV
    Mat hsv;
    cvtColor(imgcapturada, hsv, cv::COLOR_BGR2HSV);

    // Crear una máscara para el color rojo (ajustar los rangos si es necesario)
    Mat mascaraRoja;
    inRange(hsv, Scalar(0, 100, 100), Scalar(10, 255, 255), mascaraRoja); // Rango para rojo en HSV
    Mat mascaraRojaAlta;
    inRange(hsv, Scalar(160, 100, 100), Scalar(179, 255, 255), mascaraRojaAlta);
    mascaraRoja = mascaraRoja | mascaraRojaAlta;

    // Aplicar una operación de cerrado (morfología) para rellenar los huecos
    Mat mascaraRojaCerrada;
    Mat elemento = getStructuringElement(MORPH_RECT, Size(5, 5)); // Tamaño del elemento estructurante
    morphologyEx(mascaraRoja, mascaraRojaCerrada, MORPH_CLOSE, elemento);

    // Mostrar la imagen procesada
    namedWindow("Zona Roja", WINDOW_NORMAL); // O WINDOW_AUTOSIZE
    imshow("Zona Roja", mascaraRojaCerrada);

    // Crear una máscara para el color verde (ajustar los rangos si es necesario)
    Mat mascaraVerde;
    inRange(hsv, Scalar(40, 70, 70), Scalar(80, 255, 255), mascaraVerde); // Ajuste más preciso para verde en HSV

    // Aplicar una operación de cerrado (morfología) para rellenar los huecos
    Mat mascaraVerdeCerrada;
    morphologyEx(mascaraVerde, mascaraVerdeCerrada, MORPH_CLOSE, elemento);

    // Mostrar la imagen procesada para la zona verde
    namedWindow("Zona Verde", WINDOW_NORMAL); // O WINDOW_AUTOSIZE
    imshow("Zona Verde", mascaraVerdeCerrada);

    // Sumar las máscaras roja y verde (realizar una operación OR)
    Mat mascaraCombinada;
    bitwise_or(mascaraRojaCerrada, mascaraVerdeCerrada, mascaraCombinada);

    // Mostrar la imagen combinada
    namedWindow("Zona Combinada", WINDOW_NORMAL);
    imshow("Zona Combinada", mascaraCombinada);
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
		// Mostrar un mensaje de información con el nombre de la imagen guardada
		QMessageBox::information(this, "Imagen guardada", "Imagen guardada con nombre: " + filename);
	}
}