#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "StartWidget.h"
#include "SelectWidget.h"
#include "GameWidget.h"
#include "ResultWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // void switchToSelect();
    // void switchToGame();
    // void switchToResult();
    // void switchToStart();

private:

    SelectWidget *selectPage;
    StartWidget *startPage;
    GameWidget *gamePage;
    ResultWidget *resultPage;
};
#endif // MAINWINDOW_H
