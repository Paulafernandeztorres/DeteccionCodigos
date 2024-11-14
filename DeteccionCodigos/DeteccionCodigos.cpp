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
    Mat img = camera->getImage();
    QImage qimg = QImage((const unsigned char *)( img.data ), img.cols, img.rows, img.step, QImage::Format_BGR888);

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
    imgcapturada = camera->getImage();

    namedWindow("Imagen Capturada", WINDOW_NORMAL);
    imshow("Imagen Capturada", imgcapturada);
}

void DeteccionCodigos::ProcesarImagen()
{
    // Paso 1: Convertir la imagen a escala de grises
    Mat gris;
    cvtColor(imgcapturada, gris, cv::COLOR_BGR2GRAY);

    // Paso 2: Aplicar un filtro Gaussiano para suavizar la imagen
    Mat suavizada;
    GaussianBlur(gris, suavizada, cv::Size(5, 5), 0);

    // Paso 3: Aplicar un umbral (threshold) adaptativo
    Mat umbralizada;
    threshold(suavizada, umbralizada, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);

    // Mostrar la imagen procesada
    namedWindow("Imagen Procesada 1", WINDOW_NORMAL); // O WINDOW_AUTOSIZE
    imshow("Imagen Procesada 1", umbralizada);

    // Paso 4: Detección de bordes con Canny
    Mat bordes;
    Canny(umbralizada, bordes, 50, 150);

    // Paso 5: Detección de contornos
    vector<vector<Point>> contornos;
    findContours(bordes, contornos, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Paso 6: Encontrar el contorno más grande
    int indiceContornoMax = 0;
    double areaMax = contourArea(contornos[0]);
    for (size_t i = 1; i < contornos.size(); i++) {
        double area = contourArea(contornos[i]);
        if (area > areaMax) {
            areaMax = area;
            indiceContornoMax = i;
        }
    }

    // Paso 7: Crear una máscara para el área dentro del contorno
    Mat mascara = Mat::zeros(umbralizada.size(), CV_8UC1);
    drawContours(mascara, contornos, indiceContornoMax, Scalar(255), cv::FILLED);

    // Paso 9: Aplicar la máscara para conservar solo el área dentro del contorno en la imagen binarizada
    Mat resultado;
    umbralizada.copyTo(resultado, mascara);

    // Paso 8: Rellenar huecos en la máscara usando un cierre morfológico
    Mat kernel = getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5)); // Kernel de tamaño ajustable
    morphologyEx(resultado, resultado, cv::MORPH_CLOSE, kernel);

    //// Mostrar la imagen procesada
    //namedWindow("Imagen Procesada 2", WINDOW_NORMAL); // O WINDOW_AUTOSIZE
    //imshow("Imagen Procesada 2", resultado);
}



//// Función para activar los demas botones
//void DeteccionCodigos::EnableButtons(bool) {
//    isActive = !isActive; // Cambiar de estado el booleano si se pulsa el boton Enable
//    if (isActive) {
//        // si isActive es true activamos los botones
//        connect(ui.btnRecord, SIGNAL(toggle(bool)), camera, SLOT(StartStopCapture(bool)));
//        connect(ui.btnSaveImage, SIGNAL(clicked()), this, SLOT(SaveImage()));
//        connect(ui.btnLastImage, SIGNAL(clicked()), this, SLOT(GetImage()));
//        connect(ui.btnClear, SIGNAL(clicked()), this, SLOT(Clear()));
//        connect(camera, SIGNAL(newImageSignal(Mat)), this, SLOT(NewImage(Mat)));
//    }
//    else {
//        // si isActive es false desactivamos los botones
//        disconnect(ui.btnRecord, SIGNAL(toggle(bool)), camera, SLOT(StartStopCapture(bool)));
//        disconnect(ui.btnSaveImage, SIGNAL(clicked()), this, SLOT(SaveImage()));
//        disconnect(ui.btnLastImage, SIGNAL(clicked()), this, SLOT(GetImage()));
//        disconnect(ui.btnClear, SIGNAL(clicked()), this, SLOT(Clear()));
//        disconnect(camera, SIGNAL(newImageSignal(Mat)), this, SLOT(NewImage(Mat)));
//    }
//}
//
//
//void DeteccionCodigos::StartStopCapture(bool) {
//    
//}
//
//void DeteccionCodigos::SaveImage() {
//
//}
//
//void DeteccionCodigos::GetImage() {
//
//}
//
//void DeteccionCodigos::Clear() {
//
//}
//
//void DeteccionCodigos::NewImage(Mat) {
//
//}