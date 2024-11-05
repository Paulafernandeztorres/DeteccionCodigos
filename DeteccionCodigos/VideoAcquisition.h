#include <QObject>
#include <QtCore>
#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

//clase que hereda de la clase "hilos"
class CVideoAcquisition : public QThread
{
	Q_OBJECT 

private:
	//variables de la clase de acceso privado
	VideoCapture* vidcap; //variable para gestionar la captura de imagenes 
	Mat image; //variable para guardar la �ltima imagen
	QMutex mutex; //variable para bloquear el hilo de lectura de im�genes
	bool capturing; //variable para habilitar/deshabilitar la captura
	bool cameraOK; //variable para indicar si se ha realizado la comunicaci�n con la c�mara

private:
	//m�todo que se ejecutar� cuando se llame a la funci�n start de esta clase
	void run();

public:
	//constructor por defecto
	CVideoAcquisition(QString videoStreamAddress);

	//destructor
	~CVideoAcquisition();

	//funci�n que devuelve la �ltima imagen
	Mat getImage();

//se�ales
signals:	
	//se�al que se produce cada vez que hay una nueva imagen disponible
	void newImageSignal(Mat img);

//slots
public slots:	
	//slot para ejecutar/parar la adquisici�n de im�genes
	void startStopCapture(bool startCapture);
};