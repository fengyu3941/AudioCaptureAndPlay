#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    error(0),
    audioChannel(0),
    ptrVoEngine(NULL),
    ptrVoEBase(NULL),
    ptrVoEVolumeControl(NULL),
    ptrVoEFile(NULL),
    ptrVoEHardware(NULL)
{
    ui->setupUi(this);
    creatVoiceEngine();
    initialVoiceEngine();
    setDevice();
    setChannel();

    connect(ui->horizontalSliderMicrophoneVolume,SIGNAL(valueChanged(int)),this,SLOT(slotSetMicrophoneVolumeValue(int)));
    connect(ui->horizontalSliderSpeakerVolume,SIGNAL(valueChanged(int)),this,SLOT(slotSetSpeakerVolumeValue(int)));

    int vol=getMicrophoneVolumeValue();
    ui->horizontalSliderMicrophoneVolume->setValue(vol);
    ui->lineEditMicrophoneVolumeValue->setText(QString::number(vol));

    vol=getSpeakerVolumeValue();
    ui->horizontalSliderSpeakerVolume->setValue(vol);
    ui->lineEditSpeakerVolumeValue->setText(QString::number(vol));

	timerRecordLevel = new QTimer();
	timerPlayLevel = new QTimer();
	connect(timerRecordLevel, SIGNAL(timeout()), this, SLOT(onTimerRecordOut()));
	connect(timerPlayLevel, SIGNAL(timeout()), this, SLOT(onTimerPlayOut()));
	this->ui->progressBarRecord->setEnabled(false);
	this->ui->progressBarPlay->setEnabled(false);
	this->ui->progressBarRecord->setValue(0);
	this->ui->progressBarPlay->setValue(0);
}

MainWindow::~MainWindow()
{
    delete ui;
    unInitialVoiceEngine();
}

void MainWindow::creatVoiceEngine()
{
    ptrVoEngine = VoiceEngine::Create();
    ptrVoEBase = VoEBase::GetInterface(ptrVoEngine);
    ptrVoEVolumeControl = VoEVolumeControl::GetInterface(ptrVoEngine);
    ptrVoEFile = VoEFile::GetInterface(ptrVoEngine);
    ptrVoEHardware = VoEHardware::GetInterface(ptrVoEngine);
	ptrAudioproc = VoEAudioProcessing::GetInterface(ptrVoEngine);
}

int MainWindow::initialVoiceEngine()
{
    error = ptrVoEBase->Init();
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEBase::Init";
        return error;
    }
    error = ptrVoEBase->RegisterVoiceEngineObserver(myObserver);
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEBase:;RegisterVoiceEngineObserver";
        return error;
    }
    char temp[1024];
    error = ptrVoEBase->GetVersion(temp);
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEBase::GetVersion";
        return error;
    }
	ptrAudioproc->SetAgcStatus(true, kAgcAdaptiveAnalog);
	//ptrAudioproc->SetEcStatus(true, kEcConference);
	ptrAudioproc->SetNsStatus(true, kNsConference);
    return 100;
}

int MainWindow::unInitialVoiceEngine()
{
    //Stop Playout
    error = ptrVoEBase->StopPlayout(audioChannel);
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEBase::StopPlayout";
        return error;
    }
    error = ptrVoEFile->StopPlayingFileLocally(audioChannel);
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEFile::StopPlayingFileLocally";
        return error;
    }
    //Stop Record
    error = ptrVoEFile->StopRecordingMicrophone();
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEFile::StopRecordingMicrophone";
        return error;
    }

    //Delete Channel
    error = ptrVoEBase->DeleteChannel(audioChannel);
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEBase::DeleteChannel";
        return error;
    }
    //DeRegister observer
    ptrVoEBase->DeRegisterVoiceEngineObserver();
    error = ptrVoEBase->Terminate();
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEBase::Terminate";
        return error;
    }

    if(ptrVoEBase)
    {
        ptrVoEBase->Release();
    }

    if(ptrVoEVolumeControl)
    {
        ptrVoEVolumeControl->Release();
    }

    if(ptrVoEFile)
    {
        ptrVoEFile->Release();
    }

    if(ptrVoEHardware)
    {
        ptrVoEHardware->Release();
    }
	if (ptrAudioproc)
	{
		ptrAudioproc->Release();
	}

    bool flag = VoiceEngine::Delete(ptrVoEngine);
    if (!flag)
    {
        qDebug()<<"ERROR in VoiceEngine::Delete";
        return -1;
    }
    return 100;
}

int MainWindow::setDevice()
{
    int rNum(-1), pNum(-1);
    error = ptrVoEHardware->GetNumOfRecordingDevices(rNum);
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEHardware::GetNumOfRecordingDevices";
        return error;
    }
    error = ptrVoEHardware->GetNumOfPlayoutDevices(pNum);
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEHardware::GetNumOfPlayoutDevices";
        return error;
    }

    char name[128] = { 0 };
    char guid[128] = { 0 };

    for (int j = 0; j < rNum; ++j)
    {
        error = ptrVoEHardware->GetRecordingDeviceName(j, name, guid);
        if (error != 0)
        {
            qDebug()<<"ERROR in VoEHardware::GetRecordingDeviceName";
            return error;
        }
        ui->comboBoxRecordingDevice->addItem(QString(name));
    }

    for (int j = 0; j < pNum; ++j)
    {
        error = ptrVoEHardware->GetPlayoutDeviceName(j, name, guid);
        if (error != 0)
        {
            qDebug()<<"ERROR in VoEHardware::GetPlayoutDeviceName";
            return error;
        }
        ui->comboBoxPlayoutDevice->addItem(QString(name));
    }

    error = ptrVoEHardware->SetRecordingDevice(ui->comboBoxRecordingDevice->currentIndex());
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEHardware::SetRecordingDevice";
        return error;
    }

    error = ptrVoEHardware->SetPlayoutDevice(ui->comboBoxPlayoutDevice->currentIndex());
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEHardware::SetPlayoutDevice";
        return error;
    }

    return 100;
}

void MainWindow::setChannel()
{
    audioChannel = ptrVoEBase->CreateChannel();
    if (audioChannel < 0)
    {
        qDebug()<<"ERROR in VoEBase::CreateChannel";
    }
    error = ptrVoEBase->StartPlayout(audioChannel);
    if(error != 0)
    {
        qDebug()<<"ERROR in VoEBase::StartPlayout";
    }
}

int MainWindow::getMicrophoneVolumeValue()
{
    unsigned int vol = 999;
    error = ptrVoEVolumeControl->GetMicVolume(vol);
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEVolume::GetMicVolume";
        return 0;
    }
    if ((vol > 255) || (vol < 0))
    {
        qDebug()<<"ERROR in GetMicVolume";
        return 0;
    }
    return vol;
}

int MainWindow::getSpeakerVolumeValue()
{
    unsigned int vol = 999;
    error = ptrVoEVolumeControl->GetSpeakerVolume(vol);
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEVolume::GetSpeakerVolume";
        return 0;
    }
    if ((vol > 255) || (vol < 0))
    {
        qDebug()<<"ERROR in GetSpeakerVolume";
        return 0;
    }
    return vol;
}

void MainWindow::on_pushButtonRecording_clicked()
{
    static bool flag=true;
    if(flag)
    {
		recordName = QFileDialog::getOpenFileName(this,
			tr("Open File"),
			"",
			"",
			0);
		if (recordName.isNull())
		{
			return;
		}
		
        //录制麦克风的音频，默认采样率是8000HZ
        error = ptrVoEFile->StartRecordingMicrophone(recordName.toStdString().c_str());
        if (error != 0)
        {
            qDebug()<<"ERROR in VoEFile::StartRecordingMicrophone";
        }
        else
        {
			this->ui->progressBarRecord->setEnabled(true);
			timerRecordLevel->start(100);
            ui->pushButtonRecording->setText(QStringLiteral("Stop Record"));
        }
    }
    else
    {
		this->ui->progressBarRecord->setValue(0);
		this->ui->progressBarRecord->setEnabled(false);
		timerRecordLevel->stop();
        error = ptrVoEFile->StopRecordingMicrophone();
        if (error != 0)
        {
            qDebug()<<"ERROR in VoEFile::StopRecordingMicrophone";
        }
        else
        {
            ui->pushButtonRecording->setText(QStringLiteral("Start Record"));
        }
    }

    flag=!flag;
}

void MainWindow::on_pushButtonPlayout_clicked()
{
    static bool flag=true;
    if(flag)
    {
		
		
        error = ptrVoEFile->StartPlayingFileLocally(audioChannel, recordName.toStdString().c_str());
        if (error != 0)
        {
            qDebug()<<"ERROR in VoEFile::StartPlayingFileLocally";
        } 
        else
        {
			 this->ui->progressBarPlay->setEnabled(true);
			 timerPlayLevel->start(100);
             ui->pushButtonPlayout->setText(QStringLiteral("Stop Play"));
        }
    }
    else
    {
		this->ui->progressBarPlay->setValue(0);
		this->ui->progressBarPlay->setEnabled(false);
		timerPlayLevel->stop();
        error = ptrVoEFile->StopPlayingFileLocally(audioChannel);
        if (error != 0)
        {
            qDebug()<<"ERROR in VoEFile::StopPlayingFileLocally";
        }
        else
        {
            ui->pushButtonPlayout->setText(QStringLiteral("Start Play"));
        }
    }

    flag=!flag;
}

void MainWindow::slotSetMicrophoneVolumeValue(int value)
{
    error = ptrVoEVolumeControl->SetMicVolume(value);
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEVolume::SetMicVolume";
    }
    else
    {
        ui->lineEditMicrophoneVolumeValue->setText(QString::number(value));
    }
}
void MainWindow::slotSetSpeakerVolumeValue(int value)
{
    error = ptrVoEVolumeControl->SetSpeakerVolume(value);
    if (error != 0)
    {
        qDebug()<<"ERROR in VoEVolume::SetSpeakerVolume";
    }
    else
    {
        ui->lineEditSpeakerVolumeValue->setText(QString::number(value));
    }
}

void MainWindow::onTimerRecordOut()
{
	unsigned int level = 0;
	ptrVoEVolumeControl->GetSpeechInputLevel(level);
	this->ui->progressBarRecord->setValue(level * 10 + 10);
}

void MainWindow::onTimerPlayOut()
{
	unsigned int level = 0;
	ptrVoEVolumeControl->GetSpeechOutputLevel(audioChannel, level);
	this->ui->progressBarPlay->setValue(level * 10 + 10);
}
