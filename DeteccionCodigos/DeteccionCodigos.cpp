#include "DeteccionCodigos.h"

DeteccionCodigos::DeteccionCodigos(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    camera = new CVideoAcquisition("rtsp://admin::admin@192.168.1.103:8554/profile0");
    ImageIndex = 0;

    connect(ui.btnRecord, SIGNAL(toggle(bool)), this, SLOT(EnableButtons(bool)));
    
}

// Destructor
DeteccionCodigos::~DeteccionCodigos()
{}

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