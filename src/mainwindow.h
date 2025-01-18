#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QDebug>
#include <QLabel>
#include <QUrl>
#include <QFile>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QProcess>
#include <QRegularExpression>
#include <QMap>
#include <QFileDialog>
#include <QSlider>
#include <QMessageBox>
#include <QProgressBar>
#include <QStatusBar>


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
    QMap<QString, QString> formatMap;
    QString videoTitle;

private slots:
    void on_searchButton_pressed();
    void fetchVideoDetails(const QString &videoUrl);
    void loadImage(const QUrl &imageUrl);
    void fetchAvailableFormats(const QString &videoUrl);
    void on_b_download_pressed();


private:
    Ui::MainWindow *ui;
    QString extractFormatId(const QString &selectedFormat);
    QString getUrlType(const QString &videoUrl);
    QProgressBar *progressBar;
};
#endif // MAINWINDOW_H