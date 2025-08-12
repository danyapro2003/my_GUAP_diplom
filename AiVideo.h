#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_AiVideo.h"
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QTime>
#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>
#include <functional>
#include <QRegularExpression>
//#include "llama.h"
#include <vector>

class AiVideo : public QMainWindow
{
    Q_OBJECT

public:
    explicit AiVideo(QWidget* parent = nullptr);
    ~AiVideo();

    void importVideo();
    void runSubtitleRecognition();
    void convertVidToWav(std::function<void()> onFinished);
    //void startLlamaAI(const QString prompt);
    void runLlamaProcess(QString prompt);
    QString getTextFromSrt();
    void burnSubtitlesIntoVideo();
    void runGPTScript(QString);
    void runFFmpegFromFile();


private:
    Ui::AiVideo ui;
    QMediaPlayer* player;
    QVideoWidget* videoWidget;
    QAudioOutput* audioOutput;
    QString lastVideoPath;
    QString lastVideoWavPath;
    QProcess* recognitionProcess;
    QString pathToModel;
    QString stderrOutput;
    //QFileDialog* fileDialog;

};
