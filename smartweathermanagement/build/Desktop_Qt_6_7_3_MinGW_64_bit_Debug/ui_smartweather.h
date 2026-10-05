/********************************************************************************
** Form generated from reading UI file 'smartweather.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SMARTWEATHER_H
#define UI_SMARTWEATHER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_smartweather
{
public:
    QWidget *centralwidget;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *smartweather)
    {
        if (smartweather->objectName().isEmpty())
            smartweather->setObjectName("smartweather");
        smartweather->resize(800, 600);
        centralwidget = new QWidget(smartweather);
        centralwidget->setObjectName("centralwidget");
        smartweather->setCentralWidget(centralwidget);
        menubar = new QMenuBar(smartweather);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 25));
        smartweather->setMenuBar(menubar);
        statusbar = new QStatusBar(smartweather);
        statusbar->setObjectName("statusbar");
        smartweather->setStatusBar(statusbar);

        retranslateUi(smartweather);

        QMetaObject::connectSlotsByName(smartweather);
    } // setupUi

    void retranslateUi(QMainWindow *smartweather)
    {
        smartweather->setWindowTitle(QCoreApplication::translate("smartweather", "smartweather", nullptr));
    } // retranslateUi

};

namespace Ui {
    class smartweather: public Ui_smartweather {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SMARTWEATHER_H
