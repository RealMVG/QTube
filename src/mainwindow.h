#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QDebug>
#include <QLabel>
#include <QUrl>
#include <QFile>
#include <QPixmap>
#include <QThread>
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
#include <QTimer>
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
    QString testres;
    QString videoTitle;

private slots:
    void on_searchButton_pressed();
    void fetchVideoDetails(const QString &videoUrl);
    void downloadFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleProgress();
    void loadImage(const QUrl &imageUrl);
    void downloadAndCombine(const QString &videoUrl, const QString &videoId, const QString &audioId);
    void fetchAvailableFormats(const QString &videoUrl);
    void on_downloadButton_pressed();

private:
    Ui::MainWindow *ui;
    QString extractFormatId(const QString &selectedFormat);
    QString getUrlType(const QString &videoUrl);
    QString getYTDLPPath();
};

#endif // MAINWINDOW_H
