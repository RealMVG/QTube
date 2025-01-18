#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QDebug>
#include <QLabel>
#include <QUrl>
#include <QFile>
#include <QQuickWidget>
#include <QPixmap>
#include <QDebug>
#include <QUrlQuery>
#include <QProcess>
#include <QRegularExpression>

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
    void on_b_search_pressed();
    void handleImageUrl(const QString &videoUrl);
    void fetchVideoTitle(const QString &videoUrl);

private:
    Ui::MainWindow *ui;

    QString getUrlType(const QString &videoUrl);

};
#endif // MAINWINDOW_H
