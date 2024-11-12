#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_DeteccionCodigos.h"
#include "VideoAcquisition.h"
#include "opencv2/opencv.hpp"
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
	void StopButtons(bool);


private:
    Ui::DeteccionCodigosClass ui;
    CVideoAcquisition* camera;
    cv::Mat ImageIndex;
    bool isActive = false;

	void showImageInLabel();
};


