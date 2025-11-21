/********************************************************************************
** Form generated from reading UI file 'gotoDlg.ui'
**
** Created by: Qt User Interface Compiler version 5.15.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GOTODLG_H
#define UI_GOTODLG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_GotoLocDialog
{
public:
    QGridLayout *gridLayout_2;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_2;
    QVBoxLayout *verticalLayout_9;
    QLabel *xLabel;
    QVBoxLayout *verticalLayout_10;
    QLineEdit *xEdit;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout_7;
    QLabel *yLabel;
    QVBoxLayout *verticalLayout_8;
    QLineEdit *yEdit;
    QHBoxLayout *horizontalLayout_3;
    QVBoxLayout *verticalLayout_3;
    QLabel *slabel;
    QVBoxLayout *verticalLayout;
    QLineEdit *sEdit;

    void setupUi(QDialog *GotoLocDialog)
    {
        if (GotoLocDialog->objectName().isEmpty())
            GotoLocDialog->setObjectName(QString::fromUtf8("GotoLocDialog"));
        GotoLocDialog->setWindowModality(Qt::ApplicationModal);
        GotoLocDialog->setEnabled(true);
        GotoLocDialog->resize(150, 100);
        GotoLocDialog->setMinimumSize(QSize(150, 100));
        GotoLocDialog->setSizeIncrement(QSize(2, 2));
        GotoLocDialog->setBaseSize(QSize(150, 100));
        GotoLocDialog->setFocusPolicy(Qt::ClickFocus);
        GotoLocDialog->setAcceptDrops(true);
        GotoLocDialog->setSizeGripEnabled(true);
        GotoLocDialog->setModal(false);
        gridLayout_2 = new QGridLayout(GotoLocDialog);
        gridLayout_2->setSpacing(3);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        gridLayout_2->setSizeConstraint(QLayout::SetDefaultConstraint);
        gridLayout_2->setContentsMargins(2, 2, 2, 2);
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(3);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(2, 2, 2, 2);
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(3);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(2, 2, 2, 2);
        verticalLayout_9 = new QVBoxLayout();
        verticalLayout_9->setObjectName(QString::fromUtf8("verticalLayout_9"));
        xLabel = new QLabel(GotoLocDialog);
        xLabel->setObjectName(QString::fromUtf8("xLabel"));

        verticalLayout_9->addWidget(xLabel, 0, Qt::AlignHCenter);


        horizontalLayout_2->addLayout(verticalLayout_9);

        verticalLayout_10 = new QVBoxLayout();
        verticalLayout_10->setObjectName(QString::fromUtf8("verticalLayout_10"));
        xEdit = new QLineEdit(GotoLocDialog);
        xEdit->setObjectName(QString::fromUtf8("xEdit"));
        xEdit->setInputMethodHints(Qt::ImhDigitsOnly);

        verticalLayout_10->addWidget(xEdit);


        horizontalLayout_2->addLayout(verticalLayout_10);

        horizontalLayout_2->setStretch(0, 1);
        horizontalLayout_2->setStretch(1, 4);

        verticalLayout_2->addLayout(horizontalLayout_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(3);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(2, 2, 2, 2);
        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        yLabel = new QLabel(GotoLocDialog);
        yLabel->setObjectName(QString::fromUtf8("yLabel"));

        verticalLayout_7->addWidget(yLabel, 0, Qt::AlignHCenter);


        horizontalLayout->addLayout(verticalLayout_7);

        verticalLayout_8 = new QVBoxLayout();
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        yEdit = new QLineEdit(GotoLocDialog);
        yEdit->setObjectName(QString::fromUtf8("yEdit"));
        yEdit->setInputMethodHints(Qt::ImhDigitsOnly);

        verticalLayout_8->addWidget(yEdit);


        horizontalLayout->addLayout(verticalLayout_8);

        horizontalLayout->setStretch(0, 1);
        horizontalLayout->setStretch(1, 4);

        verticalLayout_2->addLayout(horizontalLayout);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(3);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(2, 2, 2, 2);
        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        slabel = new QLabel(GotoLocDialog);
        slabel->setObjectName(QString::fromUtf8("slabel"));
        slabel->setLayoutDirection(Qt::LeftToRight);
        slabel->setMidLineWidth(0);
        slabel->setAlignment(Qt::AlignCenter);

        verticalLayout_3->addWidget(slabel);


        horizontalLayout_3->addLayout(verticalLayout_3);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        sEdit = new QLineEdit(GotoLocDialog);
        sEdit->setObjectName(QString::fromUtf8("sEdit"));

        verticalLayout->addWidget(sEdit);


        horizontalLayout_3->addLayout(verticalLayout);

        horizontalLayout_3->setStretch(0, 1);
        horizontalLayout_3->setStretch(1, 4);

        verticalLayout_2->addLayout(horizontalLayout_3);

        verticalLayout_2->setStretch(0, 1);
        verticalLayout_2->setStretch(1, 1);
        verticalLayout_2->setStretch(2, 1);

        gridLayout_2->addLayout(verticalLayout_2, 0, 0, 1, 1);

#if QT_CONFIG(shortcut)
        xLabel->setBuddy(xEdit);
        yLabel->setBuddy(yEdit);
        slabel->setBuddy(sEdit);
#endif // QT_CONFIG(shortcut)

        retranslateUi(GotoLocDialog);
        QObject::connect(xEdit, SIGNAL(textEdited(QString)), GotoLocDialog, SLOT(accept()));
        QObject::connect(yEdit, SIGNAL(textEdited(QString)), GotoLocDialog, SLOT(accept()));
        QObject::connect(GotoLocDialog, SIGNAL(objectNameChanged(QString)), xEdit, SLOT(setText(QString)));
        QObject::connect(GotoLocDialog, SIGNAL(objectNameChanged(QString)), yEdit, SLOT(setText(QString)));
        QObject::connect(sEdit, SIGNAL(textEdited(QString)), GotoLocDialog, SLOT(accept()));
        QObject::connect(GotoLocDialog, SIGNAL(objectNameChanged(QString)), sEdit, SLOT(setText(QString)));

        QMetaObject::connectSlotsByName(GotoLocDialog);
    } // setupUi

    void retranslateUi(QDialog *GotoLocDialog)
    {
        GotoLocDialog->setWindowTitle(QCoreApplication::translate("GotoLocDialog", "Goto Position", nullptr));
        xLabel->setText(QCoreApplication::translate("GotoLocDialog", "X", nullptr));
        yLabel->setText(QCoreApplication::translate("GotoLocDialog", "Y", nullptr));
        slabel->setText(QCoreApplication::translate("GotoLocDialog", "Size", nullptr));
    } // retranslateUi

};

namespace Ui {
    class GotoLocDialog: public Ui_GotoLocDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GOTODLG_H
