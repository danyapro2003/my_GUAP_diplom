#include "stdafx.h"
#include "AiVideo.h"
#include <iostream>
#include "RecognitionProgress.h"


AiVideo::AiVideo(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    videoWidget = new QVideoWidget(ui.videoFrame);

    audioOutput = new QAudioOutput(this);
    audioOutput->setVolume(1.0); // �� 0.0 �� 1.0
    player = new QMediaPlayer(this);
    player->setAudioOutput(audioOutput);

    // ������ layout, ���� ��� ���
    QVBoxLayout* layout = new QVBoxLayout(ui.videoFrame);
    layout->setContentsMargins(0, 0, 0, 0); // ������� �������
    layout->addWidget(videoWidget);

    player->setVideoOutput(videoWidget);
    player->play();

    connect(player, &QMediaPlayer::positionChanged, this, [=](qint64 pos) {
        ui.videoSlider->setValue(static_cast<int>(pos));
        });

    connect(player, &QMediaPlayer::durationChanged, this, [=](qint64 dur) {
        ui.videoSlider->setMaximum(static_cast<int>(dur));
        });


    connect(ui.videoSlider, &QSlider::sliderMoved, this, [=](int value) {
        player->setPosition(static_cast<qint64>(value));
        });

    connect(player, &QMediaPlayer::positionChanged, this, [=](qint64 pos) {
        QTime t(0, 0);
        ui.timeStartLabel->setText(t.addMSecs(pos).toString("mm:ss"));
        });

    connect(player, &QMediaPlayer::durationChanged, this, [=](qint64 dur) {
        QTime t(0, 0);
        ui.timeEndLabel->setText(t.addMSecs(dur).toString("mm:ss"));
        });

    connect(ui.importButton, &QPushButton::clicked, this, &AiVideo::importVideo);

    connect(ui.recognizeButton, &QPushButton::clicked, this, &AiVideo::runSubtitleRecognition);

    connect(ui.runButton, &QPushButton::clicked, this, [=]() {
        QString userText = ui.instructionText->toPlainText();  // �������� ����� �� QTextEdit
        //qDebug() << "User enter: " << userText;

        // ������� � ������ �������
        //this->runLlamaProcess(userText);
        this->runGPTScript(userText);
        });




}

AiVideo::~AiVideo()
{}

void AiVideo::burnSubtitlesIntoVideo() {
    qDebug() << ">>> burnSubtitlesIntoVideo called!";

    if (lastVideoPath.isEmpty()) {
        QMessageBox::warning(this, "warning", "Choose video pls!");
        return;
    }

    QString inputVideo = lastVideoPath;
    QString subtitleFile = lastVideoWavPath + ".srt";
    QString outputVideo = "video_with_subs.mp4";

    qDebug() << "��� ���� ���������:" << subtitleFile;

    if (!QFile::exists(subtitleFile)) {
        qDebug() << "���� �� ������!";
        QMessageBox::warning(this, "warning", "file not found: " + subtitleFile);
        return;
    }

    qDebug() << "���� ������, ��������� ffmpeg!";

    QString ffmpegPath = "ffmpeg";


    QStringList args;
    args << "-y" // ����������
        << "-i" << inputVideo
        << "-vf" << QString("subtitles=%1").arg(QDir::toNativeSeparators(subtitleFile))
        << "-c:a" << "copy"
        << outputVideo;

    QProcess* process = new QProcess(this);
    qDebug() << "FFmpeg path:" << ffmpegPath;
    qDebug() << "FFmpeg args:" << args;
    process->start(ffmpegPath, args);

    connect(process, &QProcess::readyReadStandardError, this, [=]() {
        QByteArray out = process->readAllStandardError();
        qDebug() << QString::fromLocal8Bit(out);
        });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, [=](int code, QProcess::ExitStatus status) {
            qDebug() << "subtitle is done, code:" << code << ", status:" << status;
            if (code == 0) {
                QMessageBox::information(this, "Done!", "Video with subtitle is saved \nvideo_with_subs.mp4 \nOpen it file for you:)");
            }
            process->deleteLater();
        });
}


void AiVideo::importVideo() {
    QString filePath = QFileDialog::getOpenFileName(this, "Choose a video", "", "����� (*.mp4 *.avi *.mov *.mkv)");
    if (!filePath.isEmpty()) {
        player->setSource(QUrl::fromLocalFile(filePath));
        lastVideoPath = filePath;
        player->play();
    }
}

void AiVideo::convertVidToWav(std::function<void()> onFinished) {
    if (lastVideoPath.isEmpty()) return;

    QStringList args;
    args << "-y" << "-i" << lastVideoPath
        << "-ar" << "16000" << "-ac" << "1"
        << "-c:a" << "pcm_s16le"
        << "output.wav";

    lastVideoWavPath = "output.wav";
    qDebug() << "start convert video to wav";

    QProcess* process = new QProcess(this);
    process->start("ffmpeg", args);

    connect(process, &QProcess::finished, this, [=](int code) {
        qDebug() << "Convert is finished, code:" << code;
        if (code == 0 && onFinished) {
            onFinished();
        }
        process->deleteLater();
        });
}

void AiVideo::runSubtitleRecognition() {
    if (lastVideoPath.isEmpty()) {
        QMessageBox::warning(this, "Error", "Before choose a video!");
        return;
    }

    

    qDebug() << "Recognitionns";

    convertVidToWav([this]() {
        qDebug() << "Start recognition";

        RecognitionProgress* dialog = new RecognitionProgress(this);
        dialog->show();

        recognitionProcess = new QProcess(this);

        QStringList args;
        args << "--model" << "resourcess/ggml-base-q8_0.bin"
            << "--file" << lastVideoWavPath
            << "--language" << "ru"
            << "--output-srt"
            << "--print-progress";


        connect(recognitionProcess, &QProcess::readyReadStandardOutput, this, [this, dialog]() {
            QByteArray output = recognitionProcess->readAllStandardOutput();
            QStringList lines = QString::fromUtf8(output).split('\n', Qt::SkipEmptyParts);

            for (const QString& line : lines) {
                qDebug().noquote() << line;

                QRegularExpression rx(R"(chunk (\d+)/(\d+))");
                QRegularExpressionMatch match = rx.match(line);
                if (match.hasMatch()) {
                    int current = match.captured(1).toInt();
                    int total = match.captured(2).toInt();
                    dialog->updateProgress(current, total);
                }
            }
            });

        


        connect(recognitionProcess, &QProcess::readyReadStandardError, this, [this]() {
            stderrOutput += QString::fromLocal8Bit(recognitionProcess->readAllStandardError());
            });



        connect(recognitionProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, dialog](int code, QProcess::ExitStatus status) {
                qDebug() << "Recognition finished with exit code:" << code;
                dialog->close();
                dialog->deleteLater();

                if (code == 0) {
                    QTimer::singleShot(1000, this, [this]() {
                        burnSubtitlesIntoVideo();
                        });
                }
                else {
                    QMessageBox::warning(this, "Warning",
                        "Recognition is not finished. Exit code: " + QString::number(code) +
                        "\n\nDetails:\n" + stderrOutput);
                }
            });



        stderrOutput.clear();

        recognitionProcess->start("resourcess/whisper-cli", args);
        });
}


//void AiVideo::startLlamaAI(const QString prompt = "������ �������� ����� �� ����� �������� ") {
//    QString srtText = getTextFromSrt();
//
//    QString systemPrompt = R"(�� � ����� �������� �� ������������. �� ���� �� ���������:
//1. �������� � ������� .srt (�� �������� � �������).
//2. ��������� ������������ (��������: ������� ������� �� ����� ������� ����).
//
//�� ������ �� ������ ������������� ���� ��� ��������� ������ ffmpeg, ������� ������� ������ ���������, ������� ������� (slow motion, �����-����� � �. �.) � �������� ���������.
//
//������ ������ � ������ ffmpeg-�������, ��� ���������.
//
//������:
//
//���������: �������� ������ � ������ � ������ �����-�����
//��������:
//00:00:15,000 --> 00:00:20,000
//�� ������ �� �����.
//
//�����:
//ffmpeg -i input.mp4 -ss 00:00:15 -to 00:00:20 -vf "setpts=2.0*PTS,format=gray" output.mp4
//)";
//
//    QString finalPrompt = systemPrompt + "\n\nUser request:\n" + prompt + "\n\nSubtitles:\n" + srtText;
//    
//    if (finalPrompt.length() > 18000) {
//        qDebug() << "Video is too big, please use another video. " << finalPrompt.length();
//        return;
//    }
//
//    llama_model_params model_params = llama_model_default_params();
//
//    llama_model* model = llama_model_load_from_file("mistral-7bq4.gguf"/*pathToModel.toUtf8().constData()*/, model_params);
//    if (!model) {
//        qDebug() << "������ �������� ������\n";
//        return;
//    }
//
//    llama_context_params ctx_params = llama_context_default_params();
//    ctx_params.n_ctx = 8191;
//
//    llama_context* ctx = llama_init_from_model(model, ctx_params);
//
//    std::vector<llama_token> tokens;
//    tokens.resize(4096);
//
//
//
//    QByteArray utf8 = finalPrompt.toUtf8();
//    const char* rawPrompt = utf8.constData();
//
//    const llama_vocab* vocab = llama_model_get_vocab(model);
//
//
//    int n_tokens = llama_tokenize(vocab, rawPrompt, strlen(rawPrompt), tokens.data(), ctx_params.n_ctx, true, false);
//    if (n_tokens <= 0) {
//        qDebug() << "Error tokenization!";
//        llama_free(ctx);
//        llama_model_free(model);
//        return;
//    }
//
//    llama_batch batch = llama_batch_get_one(tokens.data(), n_tokens);
//    llama_decode(ctx, batch);
//
//
//    QString output;
//
//    for (int i = 0; i < 256; ++i) {
//        // �������� ������ ��� ���������� ������
//        float* logits = llama_get_logits(ctx);
//        int n_vocab = llama_vocab_n_tokens(vocab);
//
//        // �������� ����� � ������������ ��������� ������ (greedy sampling)
//        int best_token = 0;
//        float best_logit = logits[0];
//        for (int t = 1; t < n_vocab; ++t) {
//            if (logits[t] > best_logit) {
//                best_token = t;
//                best_logit = logits[t];
//            }
//        }
//
//        if (best_token == llama_vocab_eos(vocab)) break;
//
//        // �������� ����� ������
//        char buffer[64] = {};
//        llama_token_to_piece(vocab, best_token, buffer, sizeof(buffer), 0, false);
//        output += QString::fromUtf8(buffer);
//
//        // ���������� ��������� ��������
//        llama_token next = best_token;
//        llama_batch next_batch = llama_batch_get_one(&next, 1);
//        llama_decode(ctx, next_batch);
//    }
//
//    // ����������� �������
//    llama_free(ctx);
//    llama_model_free(model);
//    llama_backend_free();
//
//    qDebug() << "������������� � ������� LLm:";
//    qDebug() << output;
//}

QString AiVideo::getTextFromSrt() {
    QString srtFilePath = "output.wav.srt";
    QFile file(srtFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "������ ��� �������� ����� ���������";
        return "";
    }

    QByteArray bytes = file.readAll();
    file.close();

    QString srtContent = QString::fromUtf8(bytes); // <--- �������� � Qt 6
    return srtContent;
}

void AiVideo::runLlamaProcess(QString prompt = "������ �������� ����� �� ����� �������� ") {
    QString srtText = getTextFromSrt();
    qDebug() << "Start llm";

    QString systemPrompt = R"(�� � ����� �������� �� ������������. �� ���� �� ���������:
1. �������� � ������� .srt (�� �������� � �������).
2. ��������� ������������ (��������: ������� ������� �� ����� ������� ����).

�� ������ �� ������ ������������� ���� ��� ��������� ������ ffmpeg, ������� ������� ������ ���������, ������� ������� (slow motion, �����-����� � �. �.) � �������� ���������.

������ ������ � ������ ffmpeg-�������, ��� ���������.

������:

���������: �������� ������ � ������ � ������ �����-�����
��������:
00:00:15,000 --> 00:00:20,000
�� ������ �� �����.

�����:
ffmpeg -i input.mp4 -ss 00:00:15 -to 00:00:20 -vf "setpts=2.0*PTS,format=gray" output.mp4
)";

    QString finalPrompt = systemPrompt + "\n\nUser request:\n" + prompt + "\n\nSubtitles:\n" + srtText;

    if (finalPrompt.length() > 300000) {
        qDebug() << "Video is too big, please use another video. " << finalPrompt.length();
        return;
    }

    //finalPrompt = "### Instruction:\nWhat is 1 * 5?\n\n### Response:\n";

    QProcess process;
    QString program = "C:\\llama_fresh\\build\\bin\\Release\\llama-cli.exe";
    QStringList args;
    args << "-m" << "mistral-7bq4.gguf" // modelPath
        << "-p" << finalPrompt
        << "-c" << "8192" // ���������� ������� ���������
        << "--simple-io"; // ��������� ���������, ����� ��� ������ ������ �����

    process.setProgram(program);
    process.setArguments(args);
    process.setProcessChannelMode(QProcess::MergedChannels); // ���������� stdout � stderr

    process.start();

    if (!process.waitForStarted()) {
        qDebug() << "ne udalos run model";
        return;
    }

    process.waitForFinished(-1); // ��� ����������

    QByteArray result = process.readAll(); // ������ stdout+stderr
    QString output = QString::fromUtf8(result);

    qDebug() << "ans from LLaMA:";
    qDebug().noquote() << output;

    //qDebug() << "otvet sohranen v llama_output.txt";

}

void AiVideo::runGPTScript(QString prompt) {
    qDebug().noquote() << "User enter:" << prompt;

    QString srtText = getTextFromSrt();
    QString finalPrompt = lastVideoPath + "\n������ ������������:\n" + prompt +
        "\n\n�������� �����:\n" + srtText;

    QString promptFilePath = "prompt_input.txt";
    QFile file(promptFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << finalPrompt;
        file.close();
    }
    else {
        qDebug() << "�� ������� �������� �� ��������� ����.";
        return;
    }

    QProcess* process = new QProcess(this);

    process->setProgram("python");
    process->setArguments(QStringList() << "gpt_access.py" << promptFilePath);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONIOENCODING", "utf-8");
    process->setProcessEnvironment(env);

    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start();

    

    // 1. ������ stdout
    connect(process, &QProcess::readyReadStandardOutput, this, [=]() {
        QByteArray output = process->readAllStandardOutput();
        qDebug().noquote() << QString::fromUtf8(output);
        QMessageBox::information(this, "Editing video...", "Open LLM... DONE!\n Write commands for editing...\n");
    });

    // 2. ����������
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, [=](int code, QProcess::ExitStatus status) {
            qDebug() << "GPT Python script ����������. ���: " << code;
            QMessageBox::information(this, "Video DONE!", "video saved to file: ai_rendered_video\n");
            process->deleteLater();
        });

}


void AiVideo::runFFmpegFromFile() {
    QFile file("ffmpeg_cmd.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "�� ������� ������� ffmpeg_cmd.txt";
        return;
    }

    QTextStream in(&file);
    QString commandLine = in.readAll().trimmed();
    file.close();

    // ��������: ����� ������� ������ � �����, ���� ����
    commandLine.replace("\r", "").replace("\n", "");

    // DEBUG
    qDebug().noquote() << "������� ��� cmd:";
    qDebug().noquote() << commandLine;

    QProcess process;
    process.setProgram("cmd.exe");
    process.setArguments(QStringList() << "/C" << commandLine);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start();

    if (!process.waitForStarted()) {
        qDebug() << "������ ������� ffmpeg ����� cmd";
        return;
    }

    process.waitForFinished(-1);

    QString output = QString::fromUtf8(process.readAll());
    qDebug().noquote() << "��������� ffmpeg:\n" << output;
}




