#ifndef SELECTWIDGET_H
#define SELECTWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QEvent>
#include "BaoBaoType.h"

class SelectWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SelectWidget(QWidget *parent = nullptr);
    bool eventFilter(QObject *obj, QEvent *event) override;
    QList<BaoBaoType> getP1SelectedTypes() const;
    QList<BaoBaoType> getP2SelectedTypes() const;

private slots:
    void OnAckClick();
    void OnReturnClick();

private:
    QPushButton *AckButton;
    QPushButton *ReturnButton;
    QLabel *returnIconLabel;
    QLabel *bgLabel;
    QPushButton *circleButtons[6];
    QPushButton *selectButtons[12];
    QLabel *selectLabels[12];
    QList<int> p1VisibleList;
    QList<int> p2VisibleList;
    void changeBackground(int index);
    void updatePlayButtonState();
signals:
    void goToGameWidget();
    void goToStartWidget();
};

#endif // SELECTWIDGET_H
