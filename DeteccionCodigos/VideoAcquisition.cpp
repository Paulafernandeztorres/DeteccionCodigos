#include "VideoAcquisition.h"

//constructor
CVideoAcquisition::CVideoAcquisition(QString videoStreamAddress)
{
	//crea el objeto que se utiliza para capturar imagenes
	vidcap = new VideoCapture();

	//conexi�n con webcam
	cameraOK = vidcap->open(0);

	//conexi�n con c�mara ip
	//cameraOK = vidcap->open(videoStreamAddress.toStdString());

	//inicializaci�n para no empezar a capturar im�genes
	capturing = false;
}

//destructor
CVideoAcquisition::~CVideoAcquisition(void)
{
	//parar captura
	startStopCapture(false);

	//terminar el hilo
	this->terminate();

	//liberar la memoria del objeto
	vidcap->release();
}

//funcion para empezar a capturar imagenes o parar
void CVideoAcquisition::startStopCapture(bool startCapture)
{
	//si se ha podido abrir la c�mara
	if (cameraOK)
	{
		//si hay que capturar
		if (startCapture)
		{
			//se habilita la captura
			capturing = true;
			//se lanza el hilo de captura
			this->start();
		}
		else
			//si no hay que capturar, se deshabilita la captura
			capturing = false;
	}
	else
		//si la c�mara no se ha podido abrir, se lanza un mensaje de error
		qDebug() << QString("ERROR: La c�mara no ha podido ser abierta");
}

//funci�n que captura im�genes cada 10ms
void CVideoAcquisition::run(void)
{
	//mientras haya que capturar
	while (capturing)
	{
		//se bloquea el hilo
		mutex.lock();
		//se captura la imagen y se guarda en la variable image
		vidcap->read(image);
		//se lanza la se�al de que ya hay disponible una nueva imagen
		emit newImageSignal(image);			
		//se desbloquea el hilo
		mutex.unlock();
		//se esperan 10ms para realizar la siguiente captura
		this_thread::sleep_for(chrono::milliseconds(10));		
	}
}

//funci�n que devuelve la ultima imagen obtenida
Mat CVideoAcquisition::getImage()
{
	//se bloquea el hilo
	mutex.lock();
	//se guarda la ultima imagen capturada
	Mat img = image.clone();
	//se desbloquea el hilo
	mutex.unlock();
	//re devuelve la imagen
	return img;
}