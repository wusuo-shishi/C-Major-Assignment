#ifndef RESULTWIDGET_H
#define RESULTWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QApplication>

class ResultWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ResultWidget(QWidget *parent = nullptr);
    void setWinnerText(bool redWins);
private slots:
    void OnReplayClick();
    void OnReselectClick();

private:
    QLabel *winnerLabel;
    QPushButton *ReplayButton;
    QPushButton *ReselectButton;

signals:
    void goToGameWidget();
    void goToSelectWidget();
};

#endif // RESULTWIDGET_H
