#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)

{

    setFixedSize(1600, 900);    // 固定窗口大小
    //创建几个界面
    startPage = new StartWidget(this);
    selectPage = new SelectWidget(this);
    gamePage = new GameWidget(this);
    resultPage = new ResultWidget(this);

    QStackedWidget *stack = new QStackedWidget(this);
    stack->addWidget(startPage);   // index 0
    stack->addWidget(selectPage);  // index 1
    stack->addWidget(gamePage);    // index 2
    stack->addWidget(resultPage); // index 3
    //初始化时隐藏其余界面
    // selectPage->hide();
    // gamePage->hide();
    // resultPage->hide();
    //默认开始界面
    setCentralWidget(stack);
    //绑定信号和槽
    connect(startPage, &StartWidget::goToSelectWidget, [stack](){ stack->setCurrentIndex(1);});
    connect(selectPage, &SelectWidget::goToGameWidget, [stack](){stack->setCurrentIndex(2);});
    connect(selectPage, &SelectWidget::goToStartWidget, [stack](){stack->setCurrentIndex(0);});
    connect(gamePage, &GameWidget::goToResultWidget, [stack](){stack->setCurrentIndex(3);});
    connect(resultPage, &ResultWidget::goToStartWidget, [stack](){stack->setCurrentIndex(0);});
}

// //跳转实现
// void MainWindow::switchToStart()
// {
//     // selectPage->hide();
//     // gamePage->hide();
//     // resultPage->hide();
//     // //切换到豹豹首页
//     // startPage->show();
//     setCentralWidget(startPage);
// }

// void MainWindow::switchToSelect()
// {
//     // startPage->hide();
//     // gamePage->hide();
//     // resultPage->hide();
//     // //切换到豹豹选择界面
//     // selectPage->show();
//     setCentralWidget(selectPage);
// }

// void MainWindow::switchToGame()
// {
//     // startPage->hide();
//     // selectPage->hide();
//     // resultPage->hide();
//     // //切换到豹豹游戏界面
//     // gamePage->show();
//     setCentralWidget(gamePage);
// }

// void MainWindow::switchToResult()
// {
//     // startPage->hide();
//     // selectPage->hide();
//     // gamePage->hide();
//     // //切换到豹豹结算界面
//     // resultPage->show();
//     setCentralWidget(resultPage);
// }


MainWindow::~MainWindow()
{
    delete startPage;
    delete selectPage;
    delete gamePage;
    delete resultPage;
}
