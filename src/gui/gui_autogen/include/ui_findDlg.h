/********************************************************************************
** Form generated from reading UI file 'findDlg.ui'
**
** Created by: Qt User Interface Compiler version 5.9.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FINDDLG_H
#define UI_FINDDLG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_FindObjDialog
{
public:
    QGridLayout *gridLayout_2;
    QVBoxLayout *verticalLayout_2;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QCheckBox *matchCaseCheckBox;
    QCheckBox *clearLastSelCheckBox;
    QCheckBox *addToHighlightCheckBox;
    QHBoxLayout *horizontalLayout_6;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *findObjEdit;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_4;
    QComboBox *findObjType;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *FindObjDialog)
    {
        if (FindObjDialog->objectName().isEmpty())
            FindObjDialog->setObjectName(QStringLiteral("FindObjDialog"));
        FindObjDialog->setEnabled(true);
        FindObjDialog->resize(604, 186);
        FindObjDialog->setMaximumSize(QSize(604, 186));
        FindObjDialog->setSizeIncrement(QSize(2, 2));
        FindObjDialog->setSizeGripEnabled(true);
        FindObjDialog->setModal(true);
        gridLayout_2 = new QGridLayout(FindObjDialog);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        groupBox = new QGroupBox(FindObjDialog);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        matchCaseCheckBox = new QCheckBox(groupBox);
        matchCaseCheckBox->setObjectName(QStringLiteral("matchCaseCheckBox"));

        gridLayout->addWidget(matchCaseCheckBox, 0, 0, 1, 1);

        clearLastSelCheckBox = new QCheckBox(groupBox);
        clearLastSelCheckBox->setObjectName(QStringLiteral("clearLastSelCheckBox"));

        gridLayout->addWidget(clearLastSelCheckBox, 1, 0, 1, 2);

        addToHighlightCheckBox = new QCheckBox(groupBox);
        addToHighlightCheckBox->setObjectName(QStringLiteral("addToHighlightCheckBox"));

        gridLayout->addWidget(addToHighlightCheckBox, 1, 2, 1, 1);


        verticalLayout_2->addWidget(groupBox);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QStringLiteral("horizontalLayout_6"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        label = new QLabel(FindObjDialog);
        label->setObjectName(QStringLiteral("label"));

        horizontalLayout->addWidget(label);

        findObjEdit = new QLineEdit(FindObjDialog);
        findObjEdit->setObjectName(QStringLiteral("findObjEdit"));

        horizontalLayout->addWidget(findObjEdit);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        label_4 = new QLabel(FindObjDialog);
        label_4->setObjectName(QStringLiteral("label_4"));

        horizontalLayout_4->addWidget(label_4);

        findObjType = new QComboBox(FindObjDialog);
        findObjType->setObjectName(QStringLiteral("findObjType"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(findObjType->sizePolicy().hasHeightForWidth());
        findObjType->setSizePolicy(sizePolicy);

        horizontalLayout_4->addWidget(findObjType);


        verticalLayout->addLayout(horizontalLayout_4);


        horizontalLayout_6->addLayout(verticalLayout);

        buttonBox = new QDialogButtonBox(FindObjDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(buttonBox->sizePolicy().hasHeightForWidth());
        buttonBox->setSizePolicy(sizePolicy1);
        buttonBox->setOrientation(Qt::Vertical);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        horizontalLayout_6->addWidget(buttonBox);


        verticalLayout_2->addLayout(horizontalLayout_6);


        gridLayout_2->addLayout(verticalLayout_2, 0, 0, 1, 1);


        retranslateUi(FindObjDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), FindObjDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), FindObjDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(FindObjDialog);
    } // setupUi

    void retranslateUi(QDialog *FindObjDialog)
    {
        FindObjDialog->setWindowTitle(QApplication::translate("FindObjDialog", "Find Object", Q_NULLPTR));
        groupBox->setTitle(QApplication::translate("FindObjDialog", "Select Control", Q_NULLPTR));
        matchCaseCheckBox->setText(QApplication::translate("FindObjDialog", "Match Case", Q_NULLPTR));
        clearLastSelCheckBox->setText(QApplication::translate("FindObjDialog", "Clear Last Selection", Q_NULLPTR));
        addToHighlightCheckBox->setText(QApplication::translate("FindObjDialog", "Add Set To Highlight", Q_NULLPTR));
        label->setText(QApplication::translate("FindObjDialog", "Find ", Q_NULLPTR));
        label_4->setText(QApplication::translate("FindObjDialog", "Type", Q_NULLPTR));
        findObjType->clear();
        findObjType->insertItems(0, QStringList()
         << QApplication::translate("FindObjDialog", "Instance", Q_NULLPTR)
         << QApplication::translate("FindObjDialog", "Net", Q_NULLPTR)
         << QApplication::translate("FindObjDialog", "Port", Q_NULLPTR)
        );
    } // retranslateUi

};

namespace Ui {
    class FindObjDialog: public Ui_FindObjDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FINDDLG_H
