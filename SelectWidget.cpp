#include "SelectWidget.h"
#include <QPixmap>
#include <QMap>

SelectWidget::SelectWidget(QWidget *parent)
    : QWidget{parent}
{
    setFixedSize(1600, 900);

    // 背景图（默认展示香蕉海豹）
    bgLabel = new QLabel(this);
    QPixmap pixmap(":/images/select xiangjiao.jpg");
    pixmap = pixmap.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    bgLabel->setPixmap(pixmap);
    bgLabel->setGeometry(0, 0, 1600, 900);
    bgLabel->lower();  // 确保背景在最底层

    AckButton = new QPushButton(this);
    ReturnButton = new QPushButton(this);

    ReturnButton->setText("返回");
    AckButton->setText("PLAY");

    // PLAY按钮样式：蓝色渐变，禁用时灰色
    AckButton->setFixedSize(476, 62);
    AckButton->move(1077, 500);
    AckButton->setStyleSheet("QPushButton {"
                             "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #67d7e8, stop:0.5 #4fc3f7, stop:1 #29b6f6);"
                             "    border-radius: 31px;"
                             "    border: none;"
                             "    color: white;"
                             "    font-size: 28px;"
                             "    font-weight: bold;"
                             "}"
                             "QPushButton:hover {"
                             "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #81e4f8, stop:0.5 #64d3f5, stop:1 #42c5f3);"
                             "}"
                             "QPushButton:pressed {"
                             "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #4fc3f7, stop:0.5 #29b6f6, stop:1 #03a9f4);"
                             "}"
                             "QPushButton:disabled {"
                             "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #808080, stop:0.5 #707070, stop:1 #606060);"
                             "    color: #a0a0a0;"
                             "}");

    // 返回按钮：黑色圆形
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
    layout->addStretch();
    this->setLayout(layout);

    // 返回图标：独立于ReturnButton的视觉层（圆形裁剪）
    returnIconLabel = new QLabel(this);
    QPixmap iconPixmap(":/images/fanhuijian.png");
    iconPixmap = iconPixmap.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    returnIconLabel->setPixmap(iconPixmap);
    returnIconLabel->setFixedSize(60, 60);
    returnIconLabel->setStyleSheet("border-radius: 30px;");
    returnIconLabel->move(160, 20);
    returnIconLabel->raise();
    returnIconLabel->installEventFilter(this);  // 让图标可点击

    // 6个圆形区域按钮（底部，用于切换背景展示豹豹详情）
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
        circleButtons[i]->show();
    }

    // 绑定6个圆形区域按钮到对应背景切换
    connect(circleButtons[0], &QPushButton::clicked, this, [this]() { changeBackground(1); });
    connect(circleButtons[1], &QPushButton::clicked, this, [this]() { changeBackground(2); });
    connect(circleButtons[2], &QPushButton::clicked, this, [this]() { changeBackground(3); });
    connect(circleButtons[3], &QPushButton::clicked, this, [this]() { changeBackground(4); });
    connect(circleButtons[4], &QPushButton::clicked, this, [this]() { changeBackground(5); });
    connect(circleButtons[5], &QPushButton::clicked, this, [this]() { changeBackground(6); });

    // 12个选择圈按钮（顶部横向排列，1P偶数位/2P奇数位）
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

        // 选中标签："1P"(红色)或"2P"(蓝色)，初始隐藏
        selectLabels[i] = new QLabel(this);
        selectLabels[i]->setFixedSize(btnRadius * 2, btnRadius * 2);
        QString color = (i % 2 == 0) ? "red" : "blue";
        selectLabels[i]->setStyleSheet(QString("QLabel {"
                                               "    background-color: transparent;"
                                               "    border: 4px solid %1;"
                                               "    border-radius: %2px;"
                                               "    color: %3;"
                                               "    font-size: 36px;"
                                               "    font-weight: bold;"
                                               "    text-align: center;"
                                               "}").arg(color).arg(btnRadius).arg(color));
        selectLabels[i]->setText((i % 2 == 0) ? "1P" : "2P");
        selectLabels[i]->move(x, y);
        selectLabels[i]->hide();
        selectLabels[i]->setAttribute(Qt::WA_TransparentForMouseEvents, true);  // 穿透点击

        // 选择圈点击逻辑：切换选中状态，1P/2P各限选3个
        connect(selectButtons[i], &QPushButton::clicked, this, [this, i]() {
            if (selectLabels[i]->isVisible()) {
                // 取消选中
                selectLabels[i]->hide();
                if (i % 2 == 0) {
                    p1VisibleList.removeOne(i);
                } else {
                    p2VisibleList.removeOne(i);
                }
            } else {
                // 选中：超出3个时自动取消最早的选择（FIFO）
                if (i % 2 == 0) {
                    if (p1VisibleList.size() >= 3) {
                        int oldest = p1VisibleList.takeFirst();
                        selectLabels[oldest]->hide();
                    }
                    p1VisibleList.append(i);
                } else {
                    if (p2VisibleList.size() >= 3) {
                        int oldest = p2VisibleList.takeFirst();
                        selectLabels[oldest]->hide();
                    }
                    p2VisibleList.append(i);
                }
                selectLabels[i]->show();
            }
            updatePlayButtonState();  // 更新PLAY按钮状态
        });
    }

    connect(AckButton, &QPushButton::clicked, this, &SelectWidget::OnAckClick);
    connect(ReturnButton, &QPushButton::clicked, this, &SelectWidget::OnReturnClick);

    updatePlayButtonState();  // 初始状态：PLAY按钮禁用
}

void SelectWidget::OnAckClick()
{
    emit goToGameWidget();
}

void SelectWidget::OnReturnClick()
{
    emit goToStartWidget();
}

// 返回图标点击 → 触发返回操作
bool SelectWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == returnIconLabel && event->type() == QEvent::MouseButtonPress) {
        OnReturnClick();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

// 切换背景：index 1=橡胶 2=天使 3=大力 4=红温 5=绷带 6=嘭嘭
void SelectWidget::changeBackground(int index) {
    QString imagePath;
    switch (index) {
    case 1: imagePath = ":/images/select xiangjiao.jpg";  break;
    case 2: imagePath = ":/images/select tianshi.jpg";    break;
    case 3: imagePath = ":/images/select dali.jpg";       break;
    case 4: imagePath = ":/images/select_hongwen.jpg";    break;
    case 5: imagePath = ":/images/select_bengdai.jpg";    break;
    case 6: imagePath = ":/images/select_pengpeng.jpg";   break;
    }
    QPixmap pixmap(imagePath);
    pixmap = pixmap.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    bgLabel->setPixmap(pixmap);
}

// 获取1P选择的豹豹类型（按选择顺序排列）
// 偶数索引对应关系：0=橡胶, 2=天使, 4=大力, 6=红温, 8=绷带, 10=嘭嘭
QList<BaoBaoType> SelectWidget::getP1SelectedTypes() const {
    QList<BaoBaoType> types;
    QMap<int, BaoBaoType> indexToType;
    indexToType[0] = BaoBaoType::Xiangjiao;
    indexToType[2] = BaoBaoType::Tianshi;
    indexToType[4] = BaoBaoType::Dali;
    indexToType[6] = BaoBaoType::Hongwen;
    indexToType[8] = BaoBaoType::Bengdai;
    indexToType[10] = BaoBaoType::Pengpeng;

    QList<int> sortedList = p1VisibleList;
    std::sort(sortedList.begin(), sortedList.end());

    for (int index : sortedList) {
        if (indexToType.contains(index)) {
            types.append(indexToType[index]);
        }
    }
    return types;
}

QList<BaoBaoType> SelectWidget::getP2SelectedTypes() const {
    QList<BaoBaoType> types;
    QMap<int, BaoBaoType> indexToType;
    indexToType[1] = BaoBaoType::Xiangjiao;
    indexToType[3] = BaoBaoType::Tianshi;
    indexToType[5] = BaoBaoType::Dali;
    indexToType[7] = BaoBaoType::Hongwen;
    indexToType[9] = BaoBaoType::Bengdai;
    indexToType[11] = BaoBaoType::Pengpeng;

    QList<int> sortedList = p2VisibleList;
    std::sort(sortedList.begin(), sortedList.end());

    for (int index : sortedList) {
        if (indexToType.contains(index)) {
            types.append(indexToType[index]);
        }
    }
    return types;
}

void SelectWidget::updatePlayButtonState() {
    bool canPlay = (p1VisibleList.size() == 3 && p2VisibleList.size() == 3);
    AckButton->setEnabled(canPlay);
}
