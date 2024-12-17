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
    connect(ui.btnDecodificar, SIGNAL(clicked()), this, SLOT(DecodificarCodigoDeBarras()));
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
    // Capturar la imagen desde la cámara
    imgcapturada = camera->getImage();

    // Convertir la imagen a formato QImage para mostrarla (en color, sin procesar)
    QImage qimg(imgcapturada.data,
        imgcapturada.cols,
        imgcapturada.rows,
        imgcapturada.step,
        QImage::Format_BGR888);

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
	// Si el botón está pulsado está activo
    camera->startStopCapture(!captura);
}

void DeteccionCodigos::ProcesarImagen()
{
    // Cargar la imagen desde un archivo en tu ordenador
    imgcapturada = imread("C:/Users/paufe/OneDrive/Escritorio/UNIVERSIDAD/MASTER/1º/Procesado de señales multimedia/Proyecto/images/3231 (7).jpg", IMREAD_COLOR); // Cargar en color (BGR)

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

    // Umbralización de la imagen con Otsu
    Mat imagen_binaria;
    double umbral = threshold(imagen_suavizada, imagen_binaria, 0, 255, THRESH_BINARY + THRESH_OTSU);

    // Convertir la imagen a espacio de color HSV para detección de colores
    Mat imagenHSV;
    cvtColor(imgcapturada, imagenHSV, COLOR_BGR2HSV);

    // Detectar bordes rojo y verde con rangos afinados
    Mat mascaraRoja, mascaraRoja2, mascaraVerde;

    // Rango para el color rojo (dos segmentos)
    inRange(imagenHSV, Scalar(0, 100, 100), Scalar(10, 255, 255), mascaraRoja);
    inRange(imagenHSV, Scalar(160, 100, 100), Scalar(180, 255, 255), mascaraRoja2);
    mascaraRoja = mascaraRoja | mascaraRoja2;

    // Rango para el color verde (ajustado para evitar falsos positivos)
    inRange(imagenHSV, Scalar(40, 50, 50), Scalar(85, 255, 255), mascaraVerde);

    // Crear una máscara final combinando las máscaras de rojo y verde
    Mat mascaraFinal = mascaraRoja | mascaraVerde;

    // Convertir los colores detectados (rojo y verde) a blanco
    imgcapturada.setTo(Scalar(255, 255, 255), mascaraFinal); // Establecer a blanco donde la máscara es verdadera

    // Invertir la imagen (blanco se convierte en negro y viceversa)
    bitwise_not(imgcapturada, imgcapturada); // Invertir los colores de la imagen

    // Convertir la imagen modificada a escala de grises para el siguiente procesamiento
    cvtColor(imgcapturada, imagen_gris, COLOR_BGR2GRAY);

    // Filtrado del ruido en la imagen (suavizado)
    blur(imagen_gris, imagen_suavizada, Size(7, 7));

    // Umbralización de la imagen con Otsu
    threshold(imagen_suavizada, imagen_binaria, 0, 255, THRESH_BINARY + THRESH_OTSU);

    // Suavizado para borrar puntos
    Mat imagen_suavizada2;
    blur(imagen_binaria, imagen_suavizada2, Size(3, 3));

    // Dilatación de la imagen
    Mat kernel = getStructuringElement(MORPH_RECT, Size(8, 8));
    Mat imagen_cerrada;
    morphologyEx(imagen_suavizada2, imagen_cerrada, MORPH_CLOSE, kernel);

    // Relleno de huecos
    Mat imagen_rellena;
    morphologyEx(imagen_cerrada, imagen_rellena, MORPH_CLOSE, Mat());

    // Operación de apertura para eliminar pequeños puntos
    Mat kernel_apertura = getStructuringElement(MORPH_RECT, Size(10, 10));
    Mat imagen_filtrada;
    morphologyEx(imagen_rellena, imagen_filtrada, MORPH_CLOSE, kernel_apertura);

    // Eliminar objetos pequeños
    Mat etiquetas, stats, centroids;
    int num_labels = connectedComponentsWithStats(imagen_filtrada, etiquetas, stats, centroids);

    imagen_final = Mat::zeros(imagen_filtrada.size(), CV_8UC1);
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
    qDebug() << "Error: La imagen final está vacía. Ejecuta ProcesarImagen primero.";

    // Obtener las dimensiones de la imagen
    int alto_etiqueta = imagen_final.rows;
    int ancho_etiqueta = imagen_final.cols;

    // Calcular la anchura de cada segmento
    int anchura_segmento = ancho_etiqueta / 4;

    // Dividir la imagen en 4 partes horizontales
    std::vector<Mat> segmentos;
    segmentos.push_back(imagen_final(Rect(0, 0, anchura_segmento, alto_etiqueta)));                       // Segmento 1
    segmentos.push_back(imagen_final(Rect(anchura_segmento, 0, anchura_segmento, alto_etiqueta)));        // Segmento 2
    segmentos.push_back(imagen_final(Rect(2 * anchura_segmento, 0, anchura_segmento, alto_etiqueta)));    // Segmento 3
    segmentos.push_back(imagen_final(Rect(3 * anchura_segmento, 0, ancho_etiqueta - 3 * anchura_segmento, alto_etiqueta))); // Segmento 4

    // Arreglo para almacenar resultados
    std::vector<char> resultados;

    // Iterar sobre cada segmento
    for (size_t s = 0; s < segmentos.size(); ++s)
    {
        Mat segmento_actual = segmentos[s];
        Mat etiquetas, stats, centroids;

        // Etiquetar los objetos en el segmento actual
        int num_objetos = connectedComponentsWithStats(segmento_actual, etiquetas, stats, centroids, 8, CV_32S);

        // Inicializar código del segmento
        char codigo_segmento = 'X';

        // Si no hay contornos, asignar '0' al resultado
        if (num_objetos <= 1) { // Primer objeto siempre es el fondo
            resultados.push_back('0');
            continue;
        }

        // Calcular propiedades de cada objeto
        std::vector<double> anchuras, alturas, areas;
        for (int i = 1; i < num_objetos; ++i)
        {
            anchuras.push_back(stats.at<int>(i, CC_STAT_WIDTH));
            alturas.push_back(stats.at<int>(i, CC_STAT_HEIGHT));
            areas.push_back(stats.at<int>(i, CC_STAT_AREA));
        }

        // Determinar orientación
        bool esHorizontal = (std::count_if(anchuras.begin(), anchuras.end(),
            [&](double w) { return w > alturas[0]; }) > num_objetos / 2);

        double area_segmento = segmento_actual.rows * segmento_actual.cols;

        // Clasificar según número de objetos
        if (num_objetos == 2) // Solo un contorno
        {
            double area_contorno = areas[0] / area_segmento;
            if (esHorizontal)
                codigo_segmento = '8';
            else
                codigo_segmento = (area_contorno < 0.15) ? '1' : '5';
        }
        else if (num_objetos >= 3) // Dos o más contornos
        {
            double area_contorno1 = areas[0] / area_segmento;
            double area_contorno2 = areas[1] / area_segmento;

            if (esHorizontal)
            {
                if (area_contorno1 < 0.15 && area_contorno2 < 0.15) codigo_segmento = '3';
                else if (area_contorno1 > 0.15 && area_contorno2 < 0.15) codigo_segmento = '7';
                else if (area_contorno1 < 0.15 && area_contorno2 > 0.15) codigo_segmento = '9';
            }
            else
            {
                if (area_contorno1 < 0.15 && area_contorno2 < 0.15) codigo_segmento = '2';
                else if (area_contorno1 < 0.15 && area_contorno2 > 0.15) codigo_segmento = '4';
                else if (area_contorno1 > 0.15 && area_contorno2 < 0.15) codigo_segmento = '6';
            }
        }

        // Guardar el resultado del segmento
        resultados.push_back(codigo_segmento);

        // Dibujar contornos sobre el segmento
        Mat contornos = Mat::zeros(segmento_actual.size(), CV_8UC3);
        for (int i = 1; i < num_objetos; ++i)
        {
            Mat obj;
            inRange(etiquetas, i, i, obj);
            std::vector<std::vector<Point>> boundary;
            findContours(obj, boundary, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
            drawContours(contornos, boundary, -1, Scalar(0, 0, 255), 2);
        }
    }

    // Crear una cadena con el resultado final concatenado
    QString resultadoFinal = "El codigo numerico equivale a: ";
    for (char c : resultados) {
        resultadoFinal += QChar(c); // Convertir char a QChar para agregarlo al QString
    }

    // Mostrar el resultado en la QLabel del .ui
    ui.labelcodigo->setText(resultadoFinal);
}

void DeteccionCodigos::SaveImageButton()
{

}