#include "stdafx.h"
#include <windows.h>
#include "AiVideo.h"
#include <QtWidgets/QApplication>

#ifndef CP_UTF8
#define CP_UTF8 65001
#endif

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(CP_UTF8);
    QApplication a(argc, argv);
    AiVideo w;
    w.show();
    return a.exec();
}
