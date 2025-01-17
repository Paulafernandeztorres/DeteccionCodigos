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
 * @brief Enum para definir los diferentes modos de visualización disponibles.
 */
enum ViewMode { 
    Normal,      /**< Modo normal, muestra la imagen original */
    Decoded,     /**< Modo de visualización de códigos decodificados */
    RedMask,     /**< Modo para visualizar la máscara roja */
    GreenMask    /**< Modo para visualizar la máscara verde */
};

/**
 * @struct ContourInfo
 * @brief Estructura para almacenar la información de un contorno.
 *
 * Esta estructura almacena datos relacionados con un contorno detectado en una imagen, como sus esquinas,
 * el centro, las dimensiones (ancho, alto), área, relación de aspecto, perímetro y ángulo de rotación.
 */
struct ContourInfo {
    std::vector<Point> corners;   /**< Esquinas del contorno */
    Point2f center;               /**< Centro del contorno */
    float width;                  /**< Ancho del contorno */
    float height;                 /**< Alto del contorno */
    float area;                   /**< Área del contorno */
    float aspect_ratio;           /**< Relación de aspecto del contorno */
    float perimeter;              /**< Perímetro del contorno */
    float angle;                  /**< Ángulo de rotación del contorno */
};

/**
 * @struct SegmentInfo
 * @brief Estructura para almacenar la información de un segmento de contornos.
 *
 * Esta estructura almacena datos sobre los segmentos de contornos, como el número de contornos, sus orientaciones,
 * las relaciones de área y la relación entre áreas si el segmento tiene exactamente dos contornos.
 */
struct SegmentInfo {
    size_t numContours;                         /**< Número de contornos en el segmento */
    std::vector<std::string> orientations;      /**< Orientación de cada contorno (horizontal o vertical) */
    std::vector<double> areaRatios;             /**< Relación de áreas de cada contorno respecto a la imagen */
    double areaRatioRelation;                   /**< Relación entre las áreas de los contornos (solo si hay 2) */
};

/**
 * @class DeteccionCodigos
 * @brief Clase principal para la detección y decodificación de códigos en imágenes.
 *
 * Esta clase proporciona la funcionalidad para capturar, segmentar y decodificar códigos a partir de imágenes.
 * También permite visualizar los resultados de los códigos decodificados y los contornos segmentados.
 */
class DeteccionCodigos: public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructor de la clase DeteccionCodigos.
     *
     * Inicializa la interfaz gráfica de usuario y configura la adquisición de imágenes desde la cámara.
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
     * @brief Activa o desactiva el botón de parada.
     *
     * @param state Estado del botón.
     */
    void StropButton(bool state);

    /**
     * @brief Activa o desactiva el botón de grabación.
     *
     * @param state Estado del botón.
     */
    void RecordButton(bool state);

    /**
     * @brief Procesa y decodifica la imagen capturada.
     *
     * @param state Estado del botón de decodificación.
     */
    void DecodeImagen(bool state);

    /**
     * @brief Muestra la máscara roja de la imagen.
     *
     * @param state Estado del botón para mostrar la máscara roja.
     */
    void ViewRedMask(bool state);

    /**
     * @brief Muestra la máscara verde de la imagen.
     *
     * @param state Estado del botón para mostrar la máscara verde.
     */
    void ViewGreenMask(bool state);

    /**
     * @brief Guarda el código decodificado en un archivo.
     */
    void SaveDecodedCode();

private:
    Ui::DeteccionCodigosClass ui; /**< Interfaz gráfica de usuario */
    CVideoAcquisition *camera;    /**< Objeto para la adquisición de video */
    QTimer *timer;                /**< Temporizador para actualizar la imagen */
    Mat imgcapturada;             /**< Imagen capturada */
    Mat imagenFinal;              /**< Imagen final procesada */

    ViewMode currentMode = Normal; /**< Modo de visualización actual */

    /// Funciones de segmentación de imagen
    /**
     * @brief Aplica un filtro de desenfoque a la imagen.
     *
     * @param image Imagen de entrada.
     * @param kernelSize Tamaño del núcleo del filtro de desenfoque.
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
     * @brief Obtiene la máscara de color rojo de una imagen en HSV.
     *
     * @param image Imagen en HSV.
     * @return Máscara de color rojo.
     */
    Mat getRedMask(const Mat &image);

    /**
     * @brief Obtiene la máscara de color verde de una imagen en HSV.
     *
     * @param image Imagen en HSV.
     * @return Máscara de color verde.
     */
    Mat getGreenMask(const Mat &image);

    /**
     * @brief Aplica una máscara a la imagen.
     *
     * @param image Imagen de entrada.
     * @param mask Máscara a aplicar.
     * @return Imagen con la máscara aplicada.
     */
    Mat applyMaskToImage(const Mat &image, Mat mask);

    /**
     * @brief Aplica un filtro de Sobel a la imagen.
     *
     * @param image Imagen de entrada.
     * @param kernelSize Tamaño del filtro.
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
     * @brief Extrae información relevante de los contornos.
     *
     * @param contours Contornos encontrados.
     * @return Información de los contornos.
     */
    std::vector<ContourInfo> extractContourInfo(const vector<vector<Point>> &contours);

    /**
     * @brief Empareja los contornos rojos y verdes.
     *
     * @param redContoursInfo Información de los contornos rojos.
     * @param greenContoursInfo Información de los contornos verdes.
     * @return Emparejamiento de los contornos rojos y verdes.
     */
    std::vector<std::pair<ContourInfo, ContourInfo>> matchContours(const std::vector<ContourInfo> &redContoursInfo,
                                                                   const std::vector<ContourInfo> &greenContoursInfo);

    /**
     * @brief Recorta las regiones de interés (bounding boxes) de los contornos emparejados.
     *
     * @param matchedContours Contornos emparejados.
     * @param image Imagen original.
     * @return Imágenes recortadas.
     */
    std::vector<Mat> cutBoundingBox(const std::vector<pair<ContourInfo, ContourInfo>> &matchedContours, const Mat &image);

    /**
     * @brief Obtiene la imagen segmentada con los códigos decodificados.
     *
     * @param imagen Imagen original.
     * @return Imagen segmentada con códigos decodificados.
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
     * @brief Clasifica los contornos en categorías según su posición en la imagen.
     *
     * @param contours Contornos encontrados.
     * @param imageShape Tamaño de la imagen.
     * @return Pareja de contornos clasificados.
     */
    pair<std::vector<std::vector<Point>>, std::vector<std::vector<Point>>> classifyContours(
        const std::vector<std::vector<Point>> &contours, const Size &imageShape);

    /**
     * @brief Filtra los contornos que están dentro de la imagen.
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
     * @brief Separa los contornos en segmentos según su posición.
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
     * @brief Obtiene la relación de áreas de los contornos con respecto a la imagen.
     *
     * @param contours Contornos encontrados.
     * @param image Imagen original.
     * @return Relación de áreas.
     */
    std::vector<double> getAreaRatio(const std::vector<std::vector<Point>> &contours, const Mat &image);

    /**
     * @brief Obtiene la información detallada de los segmentos de contornos ordenados.
     *
     * @param orderedSegments Segmentos de contornos ordenados.
     * @param image Imagen original.
     * @return Información de los segmentos.
     */
    std::vector<SegmentInfo> getSegmentInfo(const std::vector<std::vector<std::vector<Point>>> &orderedSegments, const Mat &image);

    /**
     * @brief Decodifica el número representado por los segmentos de contornos.
     *
     * @param segmentInfo Información de los segmentos.
     * @return Número decodificado como cadena de caracteres.
     */
    std::string decodeNumber(const std::vector<SegmentInfo> &segmentInfo);
};
