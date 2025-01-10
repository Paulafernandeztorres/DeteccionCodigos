#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_DeteccionCodigos.h"
#include "VideoAcquisition.h"
#include "opencv2/opencv.hpp"
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <cmath>
#include <algorithm>

// Estructura para almacenar la información de un contorno.
struct ContourInfo {
	std::vector<Point> corners; // Esquinas del contorno
	Point2f center;				// Centro del contorno
	float width;				// Ancho del contorno
	float height;				// Alto del contorno
	float area;					// Área del contorno	
	float aspect_ratio;			// Relación de aspecto del contorno
	float perimeter;			// Perímetro del contorno
	float angle;				// Ángulo de rotación del contorno
};

// Estructura para almacenar la información de un segmento.
struct SegmentInfo {
	size_t numContours;							// Número de contornos en el segmento
	std::vector<std::string> orientations;		// Orientación de cada contorno (horizontal o vertical)
	std::vector<double> areaRatios;				// Relación de áreas de cada contorno respecto a la imagen
	double areaRatioRelation;					// Relación entre las áreas de los contornos (solo si hay 2)
};

class DeteccionCodigos : public QMainWindow
{
    Q_OBJECT

public:
    DeteccionCodigos(QWidget *parent = nullptr);
    ~DeteccionCodigos();

private slots:
    void updateImage();
	void StropButton(bool);
	void RecordButton(bool);
    void ProcesarImagen();
	void SaveImageButton();
	void DecodificarCodigoDeBarras();
	void SegmentarImagen();

private:
    Ui::DeteccionCodigosClass ui;
    CVideoAcquisition* camera;
	// timer para actualizar la imagen
	QTimer *timer;
    Mat imgcapturada;
	Mat imagen_final;

	// Variables para guardar la imagen
	Mat grayImage;
	Mat hsvImage;
	Mat imagenRoja;
	Mat imagenVerde;

	// contador de imagen capturada
	int contadorImagen = 0;
	std::vector<std::vector<Point>> contornos;


	void showImageInLabel();
	
	// Funciones de segmentacion de imagen
	Mat BlurImage(const Mat& image, uint8_t kernelSize);
	Mat convertGrayImage(const Mat &image);
	Mat convertHSVImage(const Mat &image);
	Mat getRedMask(const Mat &image);
	Mat getGreenMask(const Mat &image);
	Mat applyMaskToImage(const Mat &image, Mat mask);
	Mat sobelFilter(const Mat &image, uint8_t kernelSize);
	std::vector<std::vector<Point>> findFilteredContours(const Mat &image);
	std::vector<ContourInfo> extractContourInfo(const vector<vector<Point>> &contours);
	std::vector<std::pair<ContourInfo, ContourInfo>> matchContours(const std::vector<ContourInfo> &redContoursInfo,
																   const std::vector<ContourInfo> &greenContoursInfo);
	std::vector<Mat> cutBoundingBox(const std::vector<pair<ContourInfo, ContourInfo>> &matchedContours, const Mat &image);
	void extractCodes(const Mat &imagenRoja, const Mat &imagenVerde);

	// Funciones de procesamiento de imagen
	Mat thresholdImage(const Mat &image, int threshold = 2);
	pair<std::vector<std::vector<Point>>, std::vector<std::vector<Point>>> classifyContours(
		const std::vector<std::vector<Point>> &contours, const Size &imageShape);
	std::vector<std::vector<Point>> filterInsideContours(const std::vector<std::vector<Point>> &contours);
	std::vector<std::vector<Point>> getContours(const Mat &thresholdedImage, const Mat &image);
	std::vector<std::vector<std::vector<Point>>> separateContoursBySegments(const std::vector<std::vector<Point>> &contours, int imageWidth);
	std::vector<std::vector<std::vector<Point>>> orderContours(const std::vector<std::vector<std::vector<Point>>> &segments);
	std::vector<double> getAreaRatio(const std::vector<std::vector<Point>> &contours, const Mat &image);
	std::vector<SegmentInfo> getSegmentInfo(const std::vector<std::vector<std::vector<Point>>> &orderedSegments, const Mat &image);
	std::string decodeNumber(const std::vector<SegmentInfo> &segmentInfo);
};


