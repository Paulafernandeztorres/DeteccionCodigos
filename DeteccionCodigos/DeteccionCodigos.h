#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_DeteccionCodigos.h"
#include "VideoAcquisition.h"
#include "opencv2/opencv.hpp"

class DeteccionCodigos : public QMainWindow
{
    Q_OBJECT

public:
    DeteccionCodigos(QWidget *parent = nullptr);
    ~DeteccionCodigos();

private slots:
    // Create the Slots
    void EnableButtons(bool);
    void StartStopCapture(bool);
    void SaveImage();
    void GetImage();
    void Clear();
    void NewImage(cv::Mat);


private:
    Ui::DeteccionCodigosClass ui;
    CVideoAcquisition* camera;
    cv::Mat ImageIndex;
    bool isActive = false;
};


