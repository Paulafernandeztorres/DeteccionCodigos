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
    Mat img = camera->getImage();

    namedWindow("Imagen Capturada", WINDOW_NORMAL);
    imshow("Imagen Capturada", img);

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