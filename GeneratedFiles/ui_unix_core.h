/********************************************************************************
** Form generated from reading UI file 'unix_core.ui'
**
** Created by: Qt User Interface Compiler version 5.1.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_UNIX_CORE_H
#define UI_UNIX_CORE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_core_analysis
{
public:
    QAction *actionConnect;
    QTabWidget *tabs;
    QWidget *login;
    QWidget *gridLayoutWidget;
    QGridLayout *gridLayout;
    QLabel *label;
    QLineEdit *hostname;
    QLabel *label_3;
    QLineEdit *password;
    QLabel *label_4;
    QLineEdit *command;
    QLabel *label_6;
    QLineEdit *binary;
    QLabel *label_2;
    QLineEdit *username;
    QLabel *label_5;
    QLineEdit *startup;
    QCheckBox *save_password;
    QCheckBox *checkBox;
    QPushButton *connect;
    QLineEdit *core;
    QLabel *label_7;
    QWidget *debug_control;
    QWidget *gridLayoutWidget_2;
    QGridLayout *gridLayout_2;
    QPushButton *stack;
    QPushButton *sharedlibs;
    QSpinBox *framenr;
    QPushButton *registers;
    QPushButton *curr_frame;

    void setupUi(QWidget *core_analysis)
    {
        if (core_analysis->objectName().isEmpty())
            core_analysis->setObjectName(QStringLiteral("core_analysis"));
        core_analysis->resize(403, 227);
        actionConnect = new QAction(core_analysis);
        actionConnect->setObjectName(QStringLiteral("actionConnect"));
        tabs = new QTabWidget(core_analysis);
        tabs->setObjectName(QStringLiteral("tabs"));
        tabs->setGeometry(QRect(10, 10, 381, 221));
        login = new QWidget();
        login->setObjectName(QStringLiteral("login"));
        gridLayoutWidget = new QWidget(login);
        gridLayoutWidget->setObjectName(QStringLiteral("gridLayoutWidget"));
        gridLayoutWidget->setGeometry(QRect(0, 0, 381, 191));
        gridLayout = new QGridLayout(gridLayoutWidget);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(gridLayoutWidget);
        label->setObjectName(QStringLiteral("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        hostname = new QLineEdit(gridLayoutWidget);
        hostname->setObjectName(QStringLiteral("hostname"));

        gridLayout->addWidget(hostname, 0, 2, 1, 1);

        label_3 = new QLabel(gridLayoutWidget);
        label_3->setObjectName(QStringLiteral("label_3"));

        gridLayout->addWidget(label_3, 3, 0, 1, 1);

        password = new QLineEdit(gridLayoutWidget);
        password->setObjectName(QStringLiteral("password"));
        password->setEchoMode(QLineEdit::Password);

        gridLayout->addWidget(password, 3, 2, 1, 1);

        label_4 = new QLabel(gridLayoutWidget);
        label_4->setObjectName(QStringLiteral("label_4"));

        gridLayout->addWidget(label_4, 4, 0, 1, 1);

        command = new QLineEdit(gridLayoutWidget);
        command->setObjectName(QStringLiteral("command"));
        command->setEchoMode(QLineEdit::Normal);

        gridLayout->addWidget(command, 4, 2, 1, 2);

        label_6 = new QLabel(gridLayoutWidget);
        label_6->setObjectName(QStringLiteral("label_6"));

        gridLayout->addWidget(label_6, 7, 0, 1, 1);

        binary = new QLineEdit(gridLayoutWidget);
        binary->setObjectName(QStringLiteral("binary"));
        binary->setEchoMode(QLineEdit::Normal);

        gridLayout->addWidget(binary, 7, 2, 1, 3);

        label_2 = new QLabel(gridLayoutWidget);
        label_2->setObjectName(QStringLiteral("label_2"));

        gridLayout->addWidget(label_2, 0, 3, 1, 1);

        username = new QLineEdit(gridLayoutWidget);
        username->setObjectName(QStringLiteral("username"));

        gridLayout->addWidget(username, 0, 4, 1, 1);

        label_5 = new QLabel(gridLayoutWidget);
        label_5->setObjectName(QStringLiteral("label_5"));

        gridLayout->addWidget(label_5, 5, 0, 1, 1);

        startup = new QLineEdit(gridLayoutWidget);
        startup->setObjectName(QStringLiteral("startup"));
        startup->setEchoMode(QLineEdit::Normal);

        gridLayout->addWidget(startup, 5, 2, 1, 2);

        save_password = new QCheckBox(gridLayoutWidget);
        save_password->setObjectName(QStringLiteral("save_password"));

        gridLayout->addWidget(save_password, 3, 3, 1, 1);

        checkBox = new QCheckBox(gridLayoutWidget);
        checkBox->setObjectName(QStringLiteral("checkBox"));

        gridLayout->addWidget(checkBox, 5, 4, 1, 1);

        connect = new QPushButton(gridLayoutWidget);
        connect->setObjectName(QStringLiteral("connect"));

        gridLayout->addWidget(connect, 4, 4, 1, 1);

        core = new QLineEdit(gridLayoutWidget);
        core->setObjectName(QStringLiteral("core"));
        core->setEchoMode(QLineEdit::Normal);

        gridLayout->addWidget(core, 8, 2, 1, 3);

        label_7 = new QLabel(gridLayoutWidget);
        label_7->setObjectName(QStringLiteral("label_7"));

        gridLayout->addWidget(label_7, 8, 0, 1, 1);

        tabs->addTab(login, QString());
        debug_control = new QWidget();
        debug_control->setObjectName(QStringLiteral("debug_control"));
        debug_control->setEnabled(false);
        gridLayoutWidget_2 = new QWidget(debug_control);
        gridLayoutWidget_2->setObjectName(QStringLiteral("gridLayoutWidget_2"));
        gridLayoutWidget_2->setGeometry(QRect(10, 10, 210, 83));
        gridLayout_2 = new QGridLayout(gridLayoutWidget_2);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        gridLayout_2->setContentsMargins(0, 0, 0, 0);
        stack = new QPushButton(gridLayoutWidget_2);
        stack->setObjectName(QStringLiteral("stack"));

        gridLayout_2->addWidget(stack, 2, 1, 1, 2);

        sharedlibs = new QPushButton(gridLayoutWidget_2);
        sharedlibs->setObjectName(QStringLiteral("sharedlibs"));

        gridLayout_2->addWidget(sharedlibs, 1, 1, 1, 2);

        framenr = new QSpinBox(gridLayoutWidget_2);
        framenr->setObjectName(QStringLiteral("framenr"));
        framenr->setMinimum(1);

        gridLayout_2->addWidget(framenr, 3, 2, 1, 1);

        registers = new QPushButton(gridLayoutWidget_2);
        registers->setObjectName(QStringLiteral("registers"));

        gridLayout_2->addWidget(registers, 3, 3, 1, 1);

        curr_frame = new QPushButton(gridLayoutWidget_2);
        curr_frame->setObjectName(QStringLiteral("curr_frame"));

        gridLayout_2->addWidget(curr_frame, 3, 1, 1, 1);

        tabs->addTab(debug_control, QString());
        QWidget::setTabOrder(tabs, hostname);
        QWidget::setTabOrder(hostname, username);
        QWidget::setTabOrder(username, password);
        QWidget::setTabOrder(password, command);
        QWidget::setTabOrder(command, binary);
        QWidget::setTabOrder(binary, sharedlibs);
        QWidget::setTabOrder(sharedlibs, stack);
        QWidget::setTabOrder(stack, framenr);
        QWidget::setTabOrder(framenr, registers);

        retranslateUi(core_analysis);

        tabs->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(core_analysis);
    } // setupUi

    void retranslateUi(QWidget *core_analysis)
    {
        core_analysis->setWindowTitle(QApplication::translate("core_analysis", "Core File Analysis", 0));
        actionConnect->setText(QApplication::translate("core_analysis", "connect", 0));
#ifndef QT_NO_TOOLTIP
        tabs->setToolTip(QString());
#endif // QT_NO_TOOLTIP
        label->setText(QApplication::translate("core_analysis", "hostname", 0));
#ifndef QT_NO_TOOLTIP
        hostname->setToolTip(QApplication::translate("core_analysis", "name of remote unix host to connet to", 0));
#endif // QT_NO_TOOLTIP
        label_3->setText(QApplication::translate("core_analysis", "password", 0));
#ifndef QT_NO_TOOLTIP
        password->setToolTip(QApplication::translate("core_analysis", "remote unix password", 0));
#endif // QT_NO_TOOLTIP
        label_4->setText(QApplication::translate("core_analysis", "command", 0));
#ifndef QT_NO_TOOLTIP
        command->setToolTip(QApplication::translate("core_analysis", "complete path to debugger", 0));
#endif // QT_NO_TOOLTIP
        label_6->setText(QApplication::translate("core_analysis", "binary", 0));
#ifndef QT_NO_TOOLTIP
        binary->setToolTip(QApplication::translate("core_analysis", "path to remote binary on host", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        binary->setWhatsThis(QApplication::translate("core_analysis", "asdadasd", 0));
#endif // QT_NO_WHATSTHIS
        label_2->setText(QApplication::translate("core_analysis", "username", 0));
#ifndef QT_NO_TOOLTIP
        username->setToolTip(QApplication::translate("core_analysis", "username of remote unix host", 0));
#endif // QT_NO_TOOLTIP
        username->setText(QString());
        label_5->setText(QApplication::translate("core_analysis", "startup", 0));
#ifndef QT_NO_TOOLTIP
        save_password->setToolTip(QApplication::translate("core_analysis", "save password in IDB", 0));
#endif // QT_NO_TOOLTIP
        save_password->setText(QApplication::translate("core_analysis", "save", 0));
        checkBox->setText(QApplication::translate("core_analysis", "debug", 0));
#ifndef QT_NO_TOOLTIP
        connect->setToolTip(QApplication::translate("core_analysis", "connect to/disconnect from remote host", 0));
#endif // QT_NO_TOOLTIP
        connect->setText(QApplication::translate("core_analysis", "connect", 0));
        label_7->setText(QApplication::translate("core_analysis", "core path", 0));
        tabs->setTabText(tabs->indexOf(login), QApplication::translate("core_analysis", "Login", 0));
#ifndef QT_NO_TOOLTIP
        stack->setToolTip(QApplication::translate("core_analysis", "Show stack trace", 0));
#endif // QT_NO_TOOLTIP
        stack->setText(QApplication::translate("core_analysis", "stack trace", 0));
#ifndef QT_NO_TOOLTIP
        sharedlibs->setToolTip(QApplication::translate("core_analysis", "Display all shared libraries.", 0));
#endif // QT_NO_TOOLTIP
        sharedlibs->setText(QApplication::translate("core_analysis", "shared libs", 0));
#ifndef QT_NO_TOOLTIP
        framenr->setToolTip(QApplication::translate("core_analysis", "increase or decrease current frame", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        registers->setToolTip(QApplication::translate("core_analysis", "show registers of current frame", 0));
#endif // QT_NO_TOOLTIP
        registers->setText(QApplication::translate("core_analysis", "registers", 0));
#ifndef QT_NO_TOOLTIP
        curr_frame->setToolTip(QApplication::translate("core_analysis", "show current frame", 0));
#endif // QT_NO_TOOLTIP
        curr_frame->setText(QApplication::translate("core_analysis", "frame", 0));
        tabs->setTabText(tabs->indexOf(debug_control), QApplication::translate("core_analysis", "Debugger", 0));
    } // retranslateUi

};

namespace Ui {
    class core_analysis: public Ui_core_analysis {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_UNIX_CORE_H
