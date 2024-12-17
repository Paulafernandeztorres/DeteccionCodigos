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
    connect(ui.btnProcesarImagen, SIGNAL(clicked(bool)), this, SLOT(ProcesarImagen()));
    connect(ui.btnDecodificar, SIGNAL(clicked(bool)), this, SLOT(DecodificarCodigoDeBarras()));
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
    // Capturar la imagen desde la c�mara
    imgcapturada = camera->getImage();

    // Convertir la imagen a formato QImage para mostrarla (en color, sin procesar)
    QImage qimg(imgcapturada.data,
        imgcapturada.cols,
        imgcapturada.rows,
        imgcapturada.step,
        QImage::Format_BGR888);

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
    // Cargar la imagen desde un archivo en tu ordenador
    imgcapturada = imread("C:/Users/paufe/OneDrive/Escritorio/UNIVERSIDAD/MASTER/1�/Procesado de se�ales multimedia/Proyecto/images/1103.jpg", IMREAD_COLOR); // Cargar en color (BGR)

    // Verificar que la imagen se haya cargado correctamente
    if (imgcapturada.empty()) {
        qDebug() << "Error: No se pudo cargar la imagen.";
        return;
    }

    // Convertir la imagen a escala de grises
    Mat imagen_gris;
    cvtColor(imgcapturada, imagen_gris, COLOR_BGR2GRAY);

    // Filtrado del ruido en la imagen (suavizado)
    Mat imagen_suavizada;
    blur(imagen_gris, imagen_suavizada, Size(7, 7));

    // Umbralizaci�n de la imagen con Otsu
    Mat imagen_binaria;
    double umbral = threshold(imagen_suavizada, imagen_binaria, 0, 255, THRESH_BINARY + THRESH_OTSU);

    // Eliminaci�n de colores en un rango
    Mat mascara;
    inRange(imagen_gris, 20, 200, mascara);
    imagen_binaria.setTo(255, mascara);
    bitwise_not(imagen_binaria, imagen_binaria);

    // Suavizado para borrar puntos
    Mat imagen_suavizada2;
    blur(imagen_binaria, imagen_suavizada2, Size(3, 3));

    // Dilataci�n de la imagen
    Mat kernel = getStructuringElement(MORPH_RECT, Size(8, 8));
    Mat imagen_cerrada;
    morphologyEx(imagen_suavizada2, imagen_cerrada, MORPH_CLOSE, kernel);

    // Relleno de huecos
    Mat imagen_rellena;
    morphologyEx(imagen_cerrada, imagen_rellena, MORPH_CLOSE, Mat());

    // Operaci�n de apertura para eliminar peque�os puntos
    Mat kernel_apertura = getStructuringElement(MORPH_RECT, Size(10, 10));
    Mat imagen_filtrada;
    morphologyEx(imagen_rellena, imagen_filtrada, MORPH_CLOSE, kernel_apertura);

    // Eliminar objetos peque�os
    Mat etiquetas, stats, centroids;
    int num_labels = connectedComponentsWithStats(imagen_filtrada, etiquetas, stats, centroids);

    Mat imagen_final = Mat::zeros(imagen_filtrada.size(), CV_8UC1);
    int umbral_area = 0.01 * imagen_filtrada.rows * imagen_filtrada.cols;

    for (int i = 1; i < num_labels; ++i) {
        if (stats.at<int>(i, CC_STAT_AREA) >= umbral_area) {
            imagen_final.setTo(255, etiquetas == i);
        }
    }

    // Mostrar la imagen final
    imshow("Imagen final", imagen_final);
    waitKey(0); // Mantener la ventana abierta hasta que se presione una tecla
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