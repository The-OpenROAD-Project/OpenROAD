/********************************************************************************
** Form generated from reading UI file 'selectedWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.9.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SELECTEDWIDGET_H
#define UI_SELECTEDWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SelectHighlightWidget
{
public:
    QWidget *dockWidgetContents;
    QGridLayout *gridLayout;
    QTabWidget *tabWidget;
    QWidget *selTab;
    QGridLayout *gridLayout_2;
    QVBoxLayout *verticalLayout_2;
    QFrame *frame_2;
    QGridLayout *gridLayout_5;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *findEditInSel;
    QTableView *selTableView;
    QWidget *hltTab;
    QGridLayout *gridLayout_4;
    QVBoxLayout *verticalLayout;
    QFrame *frame;
    QGridLayout *gridLayout_3;
    QHBoxLayout *horizontalLayout;
    QLineEdit *findEditInHlt;
    QTableView *hltTableView;

    void setupUi(QDockWidget *SelectHighlightWidget)
    {
        if (SelectHighlightWidget->objectName().isEmpty())
            SelectHighlightWidget->setObjectName(QStringLiteral("SelectHighlightWidget"));
        SelectHighlightWidget->resize(878, 540);
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName(QStringLiteral("dockWidgetContents"));
        gridLayout = new QGridLayout(dockWidgetContents);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        tabWidget = new QTabWidget(dockWidgetContents);
        tabWidget->setObjectName(QStringLiteral("tabWidget"));
        selTab = new QWidget();
        selTab->setObjectName(QStringLiteral("selTab"));
        gridLayout_2 = new QGridLayout(selTab);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        frame_2 = new QFrame(selTab);
        frame_2->setObjectName(QStringLiteral("frame_2"));
        frame_2->setFrameShape(QFrame::StyledPanel);
        frame_2->setFrameShadow(QFrame::Raised);
        gridLayout_5 = new QGridLayout(frame_2);
        gridLayout_5->setObjectName(QStringLiteral("gridLayout_5"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        findEditInSel = new QLineEdit(frame_2);
        findEditInSel->setObjectName(QStringLiteral("findEditInSel"));
        findEditInSel->setMinimumSize(QSize(0, 35));

        horizontalLayout_2->addWidget(findEditInSel);


        gridLayout_5->addLayout(horizontalLayout_2, 0, 0, 1, 1);


        verticalLayout_2->addWidget(frame_2);

        selTableView = new QTableView(selTab);
        selTableView->setObjectName(QStringLiteral("selTableView"));
        selTableView->setContextMenuPolicy(Qt::CustomContextMenu);
        selTableView->setSortingEnabled(true);

        verticalLayout_2->addWidget(selTableView);


        gridLayout_2->addLayout(verticalLayout_2, 0, 0, 1, 1);

        tabWidget->addTab(selTab, QString());
        hltTab = new QWidget();
        hltTab->setObjectName(QStringLiteral("hltTab"));
        gridLayout_4 = new QGridLayout(hltTab);
        gridLayout_4->setObjectName(QStringLiteral("gridLayout_4"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        frame = new QFrame(hltTab);
        frame->setObjectName(QStringLiteral("frame"));
        frame->setFrameShape(QFrame::NoFrame);
        frame->setFrameShadow(QFrame::Raised);
        gridLayout_3 = new QGridLayout(frame);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        findEditInHlt = new QLineEdit(frame);
        findEditInHlt->setObjectName(QStringLiteral("findEditInHlt"));
        findEditInHlt->setMinimumSize(QSize(0, 35));

        horizontalLayout->addWidget(findEditInHlt);


        gridLayout_3->addLayout(horizontalLayout, 0, 0, 1, 1);


        verticalLayout->addWidget(frame);

        hltTableView = new QTableView(hltTab);
        hltTableView->setObjectName(QStringLiteral("hltTableView"));
        hltTableView->setContextMenuPolicy(Qt::CustomContextMenu);
        hltTableView->setSortingEnabled(true);
        hltTableView->verticalHeader()->setProperty("showSortIndicator", QVariant(true));

        verticalLayout->addWidget(hltTableView);


        gridLayout_4->addLayout(verticalLayout, 0, 0, 1, 1);

        tabWidget->addTab(hltTab, QString());

        gridLayout->addWidget(tabWidget, 0, 0, 1, 1);

        SelectHighlightWidget->setWidget(dockWidgetContents);

        retranslateUi(SelectHighlightWidget);

        tabWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(SelectHighlightWidget);
    } // setupUi

    void retranslateUi(QDockWidget *SelectHighlightWidget)
    {
        SelectHighlightWidget->setWindowTitle(QApplication::translate("SelectHighlightWidget", "Selected/Highlighted Shapes", Q_NULLPTR));
        findEditInSel->setPlaceholderText(QApplication::translate("SelectHighlightWidget", "Enter Object To Find <Hit Enter>", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(selTab), QApplication::translate("SelectHighlightWidget", "Selected", Q_NULLPTR));
        findEditInHlt->setPlaceholderText(QApplication::translate("SelectHighlightWidget", "Enter Object To Find <Hit Enter>", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(hltTab), QApplication::translate("SelectHighlightWidget", "Highlighted", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class SelectHighlightWidget: public Ui_SelectHighlightWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SELECTEDWIDGET_H
