/********************************************************************************
** Form generated from reading UI file 'dialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE

class Ui_Dialog
{
public:
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout_2;
    QComboBox *comboBox;
    QPushButton *OpenFile;
    QPushButton *pbSend;
    QVBoxLayout *verticalLayout;
    QLineEdit *leHost;
    QSpinBox *sbPort;
    QPushButton *pbConnect;
    QPushButton *pbDisconnect;
    QPlainTextEdit *pteMessage;
    QListWidget *lwLog;
    QCustomPlot *widget;

    void setupUi(QDialog *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName(QString::fromUtf8("Dialog"));
        Dialog->resize(529, 387);
        gridLayout = new QGridLayout(Dialog);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        comboBox = new QComboBox(Dialog);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName(QString::fromUtf8("comboBox"));

        verticalLayout_2->addWidget(comboBox);

        OpenFile = new QPushButton(Dialog);
        OpenFile->setObjectName(QString::fromUtf8("OpenFile"));

        verticalLayout_2->addWidget(OpenFile);

        pbSend = new QPushButton(Dialog);
        pbSend->setObjectName(QString::fromUtf8("pbSend"));
        pbSend->setEnabled(false);

        verticalLayout_2->addWidget(pbSend);


        gridLayout->addLayout(verticalLayout_2, 2, 1, 1, 1);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(6);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        leHost = new QLineEdit(Dialog);
        leHost->setObjectName(QString::fromUtf8("leHost"));

        verticalLayout->addWidget(leHost);

        sbPort = new QSpinBox(Dialog);
        sbPort->setObjectName(QString::fromUtf8("sbPort"));
        sbPort->setMaximum(65536);
        sbPort->setValue(5000);

        verticalLayout->addWidget(sbPort);

        pbConnect = new QPushButton(Dialog);
        pbConnect->setObjectName(QString::fromUtf8("pbConnect"));

        verticalLayout->addWidget(pbConnect);

        pbDisconnect = new QPushButton(Dialog);
        pbDisconnect->setObjectName(QString::fromUtf8("pbDisconnect"));
        pbDisconnect->setEnabled(false);

        verticalLayout->addWidget(pbDisconnect);


        gridLayout->addLayout(verticalLayout, 2, 2, 1, 1);

        pteMessage = new QPlainTextEdit(Dialog);
        pteMessage->setObjectName(QString::fromUtf8("pteMessage"));

        gridLayout->addWidget(pteMessage, 2, 0, 1, 1);

        lwLog = new QListWidget(Dialog);
        lwLog->setObjectName(QString::fromUtf8("lwLog"));

        gridLayout->addWidget(lwLog, 0, 0, 2, 1);

        widget = new QCustomPlot(Dialog);
        widget->setObjectName(QString::fromUtf8("widget"));

        gridLayout->addWidget(widget, 0, 1, 2, 2);


        retranslateUi(Dialog);

        QMetaObject::connectSlotsByName(Dialog);
    } // setupUi

    void retranslateUi(QDialog *Dialog)
    {
        Dialog->setWindowTitle(QApplication::translate("Dialog", "Lab2 client", nullptr));
        comboBox->setItemText(0, QApplication::translate("Dialog", "frequence", nullptr));
        comboBox->setItemText(1, QApplication::translate("Dialog", "EXT ADC1", nullptr));
        comboBox->setItemText(2, QApplication::translate("Dialog", "EXT ADC2", nullptr));
        comboBox->setItemText(3, QApplication::translate("Dialog", "ADC1 1kHz", nullptr));
        comboBox->setItemText(4, QApplication::translate("Dialog", "ADC1 10kHz", nullptr));
        comboBox->setItemText(5, QApplication::translate("Dialog", "ADC1 50kHz", nullptr));
        comboBox->setItemText(6, QApplication::translate("Dialog", "ADC2 1kHz", nullptr));
        comboBox->setItemText(7, QApplication::translate("Dialog", "ADC2 10kHz", nullptr));
        comboBox->setItemText(8, QApplication::translate("Dialog", "ADC2 50kHz", nullptr));

        OpenFile->setText(QApplication::translate("Dialog", "Open File", nullptr));
        pbSend->setText(QApplication::translate("Dialog", "Send", nullptr));
        leHost->setText(QApplication::translate("Dialog", "169.254.153.204", nullptr));
        pbConnect->setText(QApplication::translate("Dialog", "Connect", nullptr));
        pbDisconnect->setText(QApplication::translate("Dialog", "Disconnect", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIALOG_H
