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
private slots:
    void OnBackClick();
    void OnExitClick();

private:
    QPushButton *BackButton;
    QPushButton *ExitButton;

signals:
    void goToStartWidget();
};

#endif // RESULTWIDGET_H
