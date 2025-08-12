#pragma once

#include "D:\Qt\6.9.1\msvc2022_64\include\QtWidgets\qdialog.h"
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>

class RecognitionProgress :
    public QDialog
{
    Q_OBJECT
public:
    RecognitionProgress(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle("Recognition..");
        setModal(true);
        setFixedSize(300, 100);

        QVBoxLayout* layout = new QVBoxLayout(this);
        label = new QLabel("Start...");
        progressBar = new QProgressBar();

        progressBar->setMinimum(0);
        progressBar->setMaximum(100);

        layout->addWidget(label);
        layout->addWidget(progressBar);
    }

    void updateProgress(int current, int total) {
        label->setText(QString("Processing %1 from %2...").arg(current).arg(total));
        progressBar->setValue((100 * current) / total);
    }

private:
    QLabel* label;
    QProgressBar* progressBar;
};

