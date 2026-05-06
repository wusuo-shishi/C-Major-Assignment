#include "SelectWidget.h"
#include <QPixmap>

SelectWidget::SelectWidget(QWidget *parent)
    : QWidget{parent}
{
    setFixedSize(1600, 900);

    bgLabel = new QLabel(this);
    QPixmap pixmap("D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\select xiangjiao.jpg");
    pixmap = pixmap.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    bgLabel->setPixmap(pixmap);
    bgLabel->setGeometry(0, 0, 1600, 900);
    bgLabel->lower();

    AckButton = new QPushButton(this);
    ReturnButton = new QPushButton(this);

    ReturnButton->setText("返回");
    AckButton->setText("确认");

    AckButton->setFixedSize(200,90);
    ReturnButton->setFixedSize(60,60);
    ReturnButton->setStyleSheet("QPushButton {"
                               "    background-color: black;"
                               "    border-radius: 30px;"
                               "    color: white;"
                               "    font-size: 16px;"
                               "}");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(160, 20, 0, 0);

    layout->addWidget(ReturnButton);
    layout->addStretch();
    layout->addWidget(AckButton);
    layout->addStretch();

    this->setLayout(layout);

    returnIconLabel = new QLabel(this);
    QPixmap iconPixmap("D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\fanhuijian.png");
    iconPixmap = iconPixmap.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    returnIconLabel->setPixmap(iconPixmap);
    returnIconLabel->setFixedSize(60, 60);
    returnIconLabel->setStyleSheet("border-radius: 30px;");
    returnIconLabel->move(160, 20);
    returnIconLabel->raise();
    returnIconLabel->installEventFilter(this);

    int radius = 95;
    double startXPercent = 7.41;
    double endXPercent = 87.11;
    double centerYPercent = 77.08;
    double spacingPercent = (endXPercent - startXPercent) / 5;

    for (int i = 0; i < 6; i++) {
    circleButtons[i] = new QPushButton(this);
    circleButtons[i]->setFixedSize(radius * 2, radius * 2);
    circleButtons[i]->setStyleSheet("QPushButton {"
                                    "    background-color: transparent;"
                                    "    border-radius: " + QString::number(radius) + "px;"
                                    "}");
    int x = (startXPercent + i * spacingPercent) / 100.0 * 1600 - radius;
    int y = centerYPercent / 100.0 * 900 - radius;
    circleButtons[i]->move(x, y);
}

connect(circleButtons[0], &QPushButton::clicked, this, [this]() { changeBackground(1); });
connect(circleButtons[1], &QPushButton::clicked, this, [this]() { changeBackground(2); });
connect(circleButtons[2], &QPushButton::clicked, this, [this]() { changeBackground(3); });

    int btnRadius = 35;
    double startX = 2.02;
    double endX = 91.14;
    double yPos = 75.0;
    double spacing = (endX - startX) / 11;

    for (int i = 0; i < 12; i++) {
        selectButtons[i] = new QPushButton(this);
        selectButtons[i]->setFixedSize(btnRadius * 2, btnRadius * 2);
        selectButtons[i]->setStyleSheet("QPushButton {"
                                        "    background-color: transparent;"
                                        "    border: 2px solid white;"
                                        "    border-radius: " + QString::number(btnRadius) + "px;"
                                        "}");
        int x = (startX + i * spacing) / 100.0 * 1600 - btnRadius;
        int y = yPos / 100.0 * 900 - btnRadius;
        selectButtons[i]->move(x, y);

        selectLabels[i] = new QLabel(this);
        selectLabels[i]->setFixedSize(btnRadius * 2, btnRadius * 2);
        selectLabels[i]->setStyleSheet("QLabel {"
                                       "    background-color: transparent;"
                                       "    border-radius: " + QString::number(btnRadius) + "px;"
                                       "    color: " + ((i % 2 == 0) ? "red" : "blue") + ";"
                                       "    font-size: 36px;"
                                       "    font-weight: bold;"
                                       "    text-align: center;"
                                       "}");
        selectLabels[i]->setText((i % 2 == 0) ? "1P" : "2P");
        selectLabels[i]->move(x, y);
        selectLabels[i]->hide();

        connect(selectButtons[i], &QPushButton::clicked, this, [this, i]() {
            if (selectLabels[i]->isVisible()) {
                selectLabels[i]->hide();
            } else {
                selectLabels[i]->show();
            }
        });
    }

    connect(AckButton, &QPushButton::clicked, this, &SelectWidget::OnAckClick);
    connect(ReturnButton, &QPushButton::clicked, this, &SelectWidget::OnReturnClick);
}

void SelectWidget::OnAckClick()
{
    emit goToGameWidget();
}

void SelectWidget::OnReturnClick()
{
    emit goToStartWidget();
}

bool SelectWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == returnIconLabel && event->type() == QEvent::MouseButtonPress) {
        OnReturnClick();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void SelectWidget::changeBackground(int index) {
    QString imagePath;
    switch (index) {
    case 1:
        imagePath = "D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\select xiangjiao.jpg";
        break;
    case 2:
        imagePath = "D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\select tianshi.jpg";
        break;
    case 3:
        imagePath = "D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\select dali.jpg";
        break;
    }
    QPixmap pixmap(imagePath);
    pixmap = pixmap.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    bgLabel->setPixmap(pixmap);
}
