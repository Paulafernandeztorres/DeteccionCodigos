#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_DeteccionCodigos.h"
#include "VideoAcquisition.h"
#include "opencv2/opencv.hpp"
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QFileDialog>
#include <cmath>
#include <algorithm>

/**
 * @enum ViewMode
 * @brief Enum para definir los diferentes modos de visualizaci�n disponibles.
 */
enum ViewMode { 
    Normal,      /**< Modo normal, muestra la imagen original */
    Decoded,     /**< Modo de visualizaci�n de c�digos decodificados */
    RedMask,     /**< Modo para visualizar la m�scara roja */
    GreenMask    /**< Modo para visualizar la m�scara verde */
};

/**
 * @struct ContourInfo
 * @brief Estructura para almacenar la informaci�n de un contorno.
 *
 * Esta estructura almacena datos relacionados con un contorno detectado en una imagen, como sus esquinas,
 * el centro, las dimensiones (ancho, alto), �rea, relaci�n de aspecto, per�metro y �ngulo de rotaci�n.
 */
struct ContourInfo {
    std::vector<Point> corners;   /**< Esquinas del contorno */
    Point2f center;               /**< Centro del contorno */
    float width;                  /**< Ancho del contorno */
    float height;                 /**< Alto del contorno */
    float area;                   /**< �rea del contorno */
    float aspect_ratio;           /**< Relaci�n de aspecto del contorno */
    float perimeter;              /**< Per�metro del contorno */
    float angle;                  /**< �ngulo de rotaci�n del contorno */
};

/**
 * @struct SegmentInfo
 * @brief Estructura para almacenar la informaci�n de un segmento de contornos.
 *
 * Esta estructura almacena datos sobre los segmentos de contornos, como el n�mero de contornos, sus orientaciones,
 * las relaciones de �rea y la relaci�n entre �reas si el segmento tiene exactamente dos contornos.
 */
struct SegmentInfo {
    size_t numContours;                         /**< N�mero de contornos en el segmento */
    std::vector<std::string> orientations;      /**< Orientaci�n de cada contorno (horizontal o vertical) */
    std::vector<double> areaRatios;             /**< Relaci�n de �reas de cada contorno respecto a la imagen */
    double areaRatioRelation;                   /**< Relaci�n entre las �reas de los contornos (solo si hay 2) */
};

/**
 * @class DeteccionCodigos
 * @brief Clase principal para la detecci�n y decodificaci�n de c�digos en im�genes.
 *
 * Esta clase proporciona la funcionalidad para capturar, segmentar y decodificar c�digos a partir de im�genes.
 * Tambi�n permite visualizar los resultados de los c�digos decodificados y los contornos segmentados.
 */
class DeteccionCodigos: public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructor de la clase DeteccionCodigos.
     *
     * Inicializa la interfaz gr�fica de usuario y configura la adquisici�n de im�genes desde la c�mara.
     *
     * @param parent Puntero al widget padre (opcional).
     */
    DeteccionCodigos(QWidget *parent = nullptr);

    /**
     * @brief Destructor de la clase DeteccionCodigos.
     *
     * Limpia los recursos utilizados por la clase.
     */
    ~DeteccionCodigos();

private slots:
    /**
     * @brief Actualiza la imagen mostrada en la interfaz.
     */
    void UpdateImage();

    /**
     * @brief Activa o desactiva el bot�n de parada.
     *
     * @param state Estado del bot�n.
     */
    void StropButton(bool state);

    /**
     * @brief Activa o desactiva el bot�n de grabaci�n.
     *
     * @param state Estado del bot�n.
     */
    void RecordButton(bool state);

    /**
     * @brief Procesa y decodifica la imagen capturada.
     *
     * @param state Estado del bot�n de decodificaci�n.
     */
    void DecodeImagen(bool state);

    /**
     * @brief Muestra la m�scara roja de la imagen.
     *
     * @param state Estado del bot�n para mostrar la m�scara roja.
     */
    void ViewRedMask(bool state);

    /**
     * @brief Muestra la m�scara verde de la imagen.
     *
     * @param state Estado del bot�n para mostrar la m�scara verde.
     */
    void ViewGreenMask(bool state);

    /**
     * @brief Guarda el c�digo decodificado en un archivo.
     */
    void SaveDecodedCode();

private:
    Ui::DeteccionCodigosClass ui; /**< Interfaz gr�fica de usuario */
    CVideoAcquisition *camera;    /**< Objeto para la adquisici�n de video */
    QTimer *timer;                /**< Temporizador para actualizar la imagen */
    Mat imgcapturada;             /**< Imagen capturada */
    Mat imagenFinal;              /**< Imagen final procesada */

    ViewMode currentMode = Normal; /**< Modo de visualizaci�n actual */

    /// Funciones de segmentaci�n de imagen
    /**
     * @brief Aplica un filtro de desenfoque a la imagen.
     *
     * @param image Imagen de entrada.
     * @param kernelSize Tama�o del n�cleo del filtro de desenfoque.
     * @return Imagen desenfocada.
     */
    Mat BlurImage(const Mat &image, uint8_t kernelSize);

    /**
     * @brief Convierte una imagen a escala de grises.
     *
     * @param image Imagen de entrada.
     * @return Imagen convertida a escala de grises.
     */
    Mat convertGrayImage(const Mat &image);

    /**
     * @brief Convierte una imagen a espacio de color HSV.
     *
     * @param image Imagen de entrada.
     * @return Imagen convertida a HSV.
     */
    Mat convertHSVImage(const Mat &image);

    /**
     * @brief Obtiene la m�scara de color rojo de una imagen en HSV.
     *
     * @param image Imagen en HSV.
     * @return M�scara de color rojo.
     */
    Mat getRedMask(const Mat &image);

    /**
     * @brief Obtiene la m�scara de color verde de una imagen en HSV.
     *
     * @param image Imagen en HSV.
     * @return M�scara de color verde.
     */
    Mat getGreenMask(const Mat &image);

    /**
     * @brief Aplica una m�scara a la imagen.
     *
     * @param image Imagen de entrada.
     * @param mask M�scara a aplicar.
     * @return Imagen con la m�scara aplicada.
     */
    Mat applyMaskToImage(const Mat &image, Mat mask);

    /**
     * @brief Aplica un filtro de Sobel a la imagen.
     *
     * @param image Imagen de entrada.
     * @param kernelSize Tama�o del filtro.
     * @return Imagen filtrada.
     */
    Mat sobelFilter(const Mat &image, uint8_t kernelSize);

    /**
     * @brief Encuentra los contornos filtrados en una imagen.
     *
     * @param image Imagen filtrada.
     * @return Contornos encontrados.
     */
    std::vector<std::vector<Point>> findFilteredContours(const Mat &image);

    /**
     * @brief Extrae informaci�n relevante de los contornos.
     *
     * @param contours Contornos encontrados.
     * @return Informaci�n de los contornos.
     */
    std::vector<ContourInfo> extractContourInfo(const vector<vector<Point>> &contours);

    /**
     * @brief Empareja los contornos rojos y verdes.
     *
     * @param redContoursInfo Informaci�n de los contornos rojos.
     * @param greenContoursInfo Informaci�n de los contornos verdes.
     * @return Emparejamiento de los contornos rojos y verdes.
     */
    std::vector<std::pair<ContourInfo, ContourInfo>> matchContours(const std::vector<ContourInfo> &redContoursInfo,
                                                                   const std::vector<ContourInfo> &greenContoursInfo);

    /**
     * @brief Recorta las regiones de inter�s (bounding boxes) de los contornos emparejados.
     *
     * @param matchedContours Contornos emparejados.
     * @param image Imagen original.
     * @return Im�genes recortadas.
     */
    std::vector<Mat> cutBoundingBox(const std::vector<pair<ContourInfo, ContourInfo>> &matchedContours, const Mat &image);

    /**
     * @brief Obtiene la imagen segmentada con los c�digos decodificados.
     *
     * @param imagen Imagen original.
     * @return Imagen segmentada con c�digos decodificados.
     */
    Mat getSegmentedImage(const Mat &imagen);

    /// Funciones de procesamiento de imagen
    /**
     * @brief Aplica un umbral a la imagen para binarizarla.
     *
     * @param image Imagen de entrada.
     * @param threshold Valor del umbral.
     * @return Imagen binarizada.
     */
    Mat thresholdImage(const Mat &image, int threshold = 2);

    /**
     * @brief Clasifica los contornos en categor�as seg�n su posici�n en la imagen.
     *
     * @param contours Contornos encontrados.
     * @param imageShape Tama�o de la imagen.
     * @return Pareja de contornos clasificados.
     */
    pair<std::vector<std::vector<Point>>, std::vector<std::vector<Point>>> classifyContours(
        const std::vector<std::vector<Point>> &contours, const Size &imageShape);

    /**
     * @brief Filtra los contornos que est�n dentro de la imagen.
     *
     * @param contours Contornos encontrados.
     * @return Contornos filtrados.
     */
    std::vector<std::vector<Point>> filterInsideContours(const std::vector<std::vector<Point>> &contours);

    /**
     * @brief Obtiene los contornos de una imagen binarizada.
     *
     * @param thresholdedImage Imagen binarizada.
     * @param image Imagen original.
     * @return Contornos encontrados.
     */
    std::vector<std::vector<Point>> getContours(const Mat &thresholdedImage, const Mat &image);

    /**
     * @brief Separa los contornos en segmentos seg�n su posici�n.
     *
     * @param contours Contornos encontrados.
     * @param imageWidth Ancho de la imagen.
     * @return Contornos segmentados.
     */
    std::vector<std::vector<std::vector<Point>>> separateContoursBySegments(const std::vector<std::vector<Point>> &contours, int imageWidth);

    /**
     * @brief Ordena los contornos dentro de cada segmento.
     *
     * @param segments Segmentos de contornos.
     * @return Segmentos con los contornos ordenados.
     */
    std::vector<std::vector<std::vector<Point>>> orderContours(const std::vector<std::vector<std::vector<Point>>> &segments);

    /**
     * @brief Obtiene la relaci�n de �reas de los contornos con respecto a la imagen.
     *
     * @param contours Contornos encontrados.
     * @param image Imagen original.
     * @return Relaci�n de �reas.
     */
    std::vector<double> getAreaRatio(const std::vector<std::vector<Point>> &contours, const Mat &image);

    /**
     * @brief Obtiene la informaci�n detallada de los segmentos de contornos ordenados.
     *
     * @param orderedSegments Segmentos de contornos ordenados.
     * @param image Imagen original.
     * @return Informaci�n de los segmentos.
     */
    std::vector<SegmentInfo> getSegmentInfo(const std::vector<std::vector<std::vector<Point>>> &orderedSegments, const Mat &image);

    /**
     * @brief Decodifica el n�mero representado por los segmentos de contornos.
     *
     * @param segmentInfo Informaci�n de los segmentos.
     * @return N�mero decodificado como cadena de caracteres.
     */
    std::string decodeNumber(const std::vector<SegmentInfo> &segmentInfo);
};
