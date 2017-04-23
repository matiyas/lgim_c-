#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <vector>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

enum Tryb {BEZIER, BSKLEJ, WIELOKAT, KOLO, ELIPSA, SCAN_LINE, FLOOD_FILL, DODAJ_PKT, USUN_PKT, PRZESUN_PKT};

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    static bool pointsCmp(QPoint p1, QPoint p2);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void rysujPiksel(int x, int y, int r, int g, int b);
    void rysujLinie(int x0, int y0, int x1, int y1);
    void rysujKolo(int x0, int y0, int r, int cR, int cG, int cB);
    void rysujElipse(int x0, int y0, int r1, int r2);
    void rysujKrzywaBeziera(QPoint p0, QPoint p1, QPoint p2, QPoint p3);
    void rysujKrzywaBSklejana(QPoint p0, QPoint p1, QPoint p2, QPoint p3);
    void rysujPoziomo(int x0, int y0, int x1);
    void radioOdslon();
    void radioSchowaj();
    void czyscEkran();
    int *kolor(int x, int y);
    void floodFill(int x0, int y0, int r, int g, int b);

private:
    Ui::MainWindow *ui;
    QTimer timer;
    int x0, y0, x1, y1, pixs_w, pixs_h;
    double beta;
    uchar *pixs;
    Tryb trybEdycja;
    Tryb trybRys;
    QPoint p0, p1, p2, p3, *trafionyPkt, startPointFill;
    std::vector <QPoint> punkty;
    bool rysujWielokat;
    bool wypelnij;

 public slots:
    void reset();
    void trybPkt();
    void trybRysowania(QString wybor);
    void sliderObroc(int i);
    void scanLine(std::vector <QPoint> punkty);
    void slotScanLine(bool);
};

#endif // MAINWINDOW_H
