#include "DeteccionCodigos.h"

DeteccionCodigos::DeteccionCodigos(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    camera = new CVideoAcquisition("rtsp://admin:admin@192.168.1.101:8554/profile0");

    // Empezar el proceso de capturar imágenes de la librería
    camera->startStopCapture(true);
    
	// llamar a la función para empezar a capturar imágenes
	showImageInLabel();
}

// Destructor
DeteccionCodigos::~DeteccionCodigos()
{}

// Función para mostrar la imagen en la interfaz
void DeteccionCodigos::showImageInLabel() {
	// Crear un temporizador para actualizar la imagen cada 10ms
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateImage()));
	timer->start(15);
}

// Función para actualizar la imagen
void DeteccionCodigos::updateImage() {
    // Obtener la imagen de la cámara
    Mat img = camera->getImage();
    // Convertir la imagen a un formato adecuado para mostrar en la interfaz
    QImage qimg = QImage((const unsigned char *)( img.data ), img.cols, img.rows, img.step, QImage::Format_BGR888);
    
    // Espejo de la imagen
    qimg = qimg.mirrored(true, false);
    
    // Mostrar la imagen en la interfaz
    ui.label->setPixmap(QPixmap::fromImage(qimg));
}


// Función para activar los demas botones
void DeteccionCodigos::EnableButtons(bool) {
    isActive = !isActive; // Cambiar de estado el booleano si se pulsa el boton Enable
    if (isActive) {
        // si isActive es true activamos los botones
        connect(ui.btnRecord, SIGNAL(toggle(bool)), camera, SLOT(StartStopCapture(bool)));
        connect(ui.btnSaveImage, SIGNAL(clicked()), this, SLOT(SaveImage()));
        connect(ui.btnLastImage, SIGNAL(clicked()), this, SLOT(GetImage()));
        connect(ui.btnClear, SIGNAL(clicked()), this, SLOT(Clear()));
        connect(camera, SIGNAL(newImageSignal(Mat)), this, SLOT(NewImage(Mat)));
    }
    else {
        // si isActive es false desactivamos los botones
        disconnect(ui.btnRecord, SIGNAL(toggle(bool)), camera, SLOT(StartStopCapture(bool)));
        disconnect(ui.btnSaveImage, SIGNAL(clicked()), this, SLOT(SaveImage()));
        disconnect(ui.btnLastImage, SIGNAL(clicked()), this, SLOT(GetImage()));
        disconnect(ui.btnClear, SIGNAL(clicked()), this, SLOT(Clear()));
        disconnect(camera, SIGNAL(newImageSignal(Mat)), this, SLOT(NewImage(Mat)));
    }
}


void DeteccionCodigos::StartStopCapture(bool) {
    
}

void DeteccionCodigos::SaveImage() {

}

void DeteccionCodigos::GetImage() {

}

void DeteccionCodigos::Clear() {

}

void DeteccionCodigos::NewImage(Mat) {

}