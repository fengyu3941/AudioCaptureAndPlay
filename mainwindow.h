#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "myobserver.h"
#include "webrtc/base/safe_conversions.h"
#include "webrtc/voice_engine/include/voe_base.h"
#include "webrtc/voice_engine/include/voe_errors.h"
#include "webrtc/voice_engine/include/voe_file.h"
#include "webrtc/voice_engine/include/voe_hardware.h"
#include "webrtc/voice_engine/include/voe_volume_control.h"
#include "webrtc/voice_engine/include/voe_audio_processing.h"
using namespace webrtc;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    void creatVoiceEngine();
    int initialVoiceEngine();
    int setDevice();
    void setChannel();
    int getMicrophoneVolumeValue();
    int getSpeakerVolumeValue();
    int unInitialVoiceEngine();

private:
    Ui::MainWindow *ui;

    MyObserver myObserver;
    int error;
    int audioChannel;

    VoiceEngine* ptrVoEngine;
    VoEBase* ptrVoEBase;
    VoEVolumeControl* ptrVoEVolumeControl;
    VoEFile* ptrVoEFile;
    VoEHardware* ptrVoEHardware;
	VoEAudioProcessing* ptrAudioproc;

	QString recordName;
	QTimer*  timerRecordLevel;
	QTimer*  timerPlayLevel;

	
private slots:
    void on_pushButtonRecording_clicked();
    void on_pushButtonPlayout_clicked();
    void slotSetMicrophoneVolumeValue(int value);
    void slotSetSpeakerVolumeValue(int value);
public slots:
	void onTimerRecordOut();
	void onTimerPlayOut();
};

#endif // MAINWINDOW_H
