/********************************************************************************
** Form generated from reading UI file 'highlightGroupDlg.ui'
**
** Created by: Qt User Interface Compiler version 5.9.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_HIGHLIGHTGROUPDLG_H
#define UI_HIGHLIGHTGROUPDLG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_HighlightGroupDlg
{
public:
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QRadioButton *grp3RadioButton;
    QRadioButton *grp5RadioButton;
    QRadioButton *grp4RadioButton;
    QRadioButton *grp6RadioButton;
    QRadioButton *grp1RadioButton;
    QRadioButton *grp8RadioButton;
    QRadioButton *grp2RadioButton;
    QRadioButton *grp7RadioButton;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QDialogButtonBox *buttonBox;
    QSpacerItem *horizontalSpacer_2;

    void setupUi(QDialog *HighlightGroupDlg)
    {
        if (HighlightGroupDlg->objectName().isEmpty())
            HighlightGroupDlg->setObjectName(QStringLiteral("HighlightGroupDlg"));
        HighlightGroupDlg->resize(230, 230);
        HighlightGroupDlg->setMinimumSize(QSize(230, 230));
        gridLayout = new QGridLayout(HighlightGroupDlg);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        groupBox = new QGroupBox(HighlightGroupDlg);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        grp3RadioButton = new QRadioButton(groupBox);
        grp3RadioButton->setObjectName(QStringLiteral("grp3RadioButton"));

        gridLayout_2->addWidget(grp3RadioButton, 2, 0, 1, 1);

        grp5RadioButton = new QRadioButton(groupBox);
        grp5RadioButton->setObjectName(QStringLiteral("grp5RadioButton"));

        gridLayout_2->addWidget(grp5RadioButton, 0, 1, 1, 1);

        grp4RadioButton = new QRadioButton(groupBox);
        grp4RadioButton->setObjectName(QStringLiteral("grp4RadioButton"));

        gridLayout_2->addWidget(grp4RadioButton, 3, 0, 1, 1);

        grp6RadioButton = new QRadioButton(groupBox);
        grp6RadioButton->setObjectName(QStringLiteral("grp6RadioButton"));

        gridLayout_2->addWidget(grp6RadioButton, 1, 1, 1, 1);

        grp1RadioButton = new QRadioButton(groupBox);
        grp1RadioButton->setObjectName(QStringLiteral("grp1RadioButton"));

        gridLayout_2->addWidget(grp1RadioButton, 0, 0, 1, 1);

        grp8RadioButton = new QRadioButton(groupBox);
        grp8RadioButton->setObjectName(QStringLiteral("grp8RadioButton"));

        gridLayout_2->addWidget(grp8RadioButton, 3, 1, 1, 1);

        grp2RadioButton = new QRadioButton(groupBox);
        grp2RadioButton->setObjectName(QStringLiteral("grp2RadioButton"));

        gridLayout_2->addWidget(grp2RadioButton, 1, 0, 1, 1);

        grp7RadioButton = new QRadioButton(groupBox);
        grp7RadioButton->setObjectName(QStringLiteral("grp7RadioButton"));

        gridLayout_2->addWidget(grp7RadioButton, 2, 1, 1, 1);


        verticalLayout->addWidget(groupBox);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        buttonBox = new QDialogButtonBox(HighlightGroupDlg);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(buttonBox->sizePolicy().hasHeightForWidth());
        buttonBox->setSizePolicy(sizePolicy);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        horizontalLayout->addWidget(buttonBox);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout);


        gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);


        retranslateUi(HighlightGroupDlg);
        QObject::connect(buttonBox, SIGNAL(accepted()), HighlightGroupDlg, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), HighlightGroupDlg, SLOT(reject()));

        QMetaObject::connectSlotsByName(HighlightGroupDlg);
    } // setupUi

    void retranslateUi(QDialog *HighlightGroupDlg)
    {
        HighlightGroupDlg->setWindowTitle(QApplication::translate("HighlightGroupDlg", "Highlight Group", Q_NULLPTR));
        groupBox->setTitle(QApplication::translate("HighlightGroupDlg", "Choose Highlight Group :", Q_NULLPTR));
        grp3RadioButton->setText(QApplication::translate("HighlightGroupDlg", "Group3", Q_NULLPTR));
        grp5RadioButton->setText(QApplication::translate("HighlightGroupDlg", "Group 5", Q_NULLPTR));
        grp4RadioButton->setText(QApplication::translate("HighlightGroupDlg", "Group 4", Q_NULLPTR));
        grp6RadioButton->setText(QApplication::translate("HighlightGroupDlg", "Group 6", Q_NULLPTR));
        grp1RadioButton->setText(QApplication::translate("HighlightGroupDlg", "Group 1", Q_NULLPTR));
        grp8RadioButton->setText(QApplication::translate("HighlightGroupDlg", "Group 8", Q_NULLPTR));
        grp2RadioButton->setText(QApplication::translate("HighlightGroupDlg", "Group 2", Q_NULLPTR));
        grp7RadioButton->setText(QApplication::translate("HighlightGroupDlg", "Group 7", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class HighlightGroupDlg: public Ui_HighlightGroupDlg {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_HIGHLIGHTGROUPDLG_H
