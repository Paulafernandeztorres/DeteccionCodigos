#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_DeteccionCodigos.h"
#include "VideoAcquisition.h"
#include "opencv2/opencv.hpp"
#include <QMessageBox>
#include <QTimer>
#include <QDebug>

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

	// contador de imagen capturada
	int contadorImagen = 0;
	std::vector<std::vector<Point>> contornos;


	void showImageInLabel();
};


