#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <cmath>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(&timer, SIGNAL(timeout()), ui->widget, SLOT(update()));
    connect(ui->radioDodaj, SIGNAL(clicked(bool)), this, SLOT(trybPkt()));
    connect(ui->radioPrzesun, SIGNAL(clicked(bool)), this, SLOT(trybPkt()));
    connect(ui->radioUsun, SIGNAL(clicked(bool)), this, SLOT(trybPkt()));
    connect(ui->verticalSlider, SIGNAL(sliderMoved(int)), this, SLOT(sliderObroc(int)));
    connect(ui->czysc, SIGNAL(clicked(bool)), this, SLOT(reset()));
    timer.start(0);

    ui->verticalSlider->setRange(0, 100);
    ui->verticalSlider->hide();
    ui->comboRysuj->addItem("Krzywa Beziera");
    ui->comboRysuj->addItem("Krzywa B-Sklejana");
    ui->comboRysuj->addItem("Scan Line");
    ui->comboRysuj->addItem("Rysuj koło");
    ui->comboRysuj->addItem("Rysuj elipsę");
    connect(ui->comboRysuj, SIGNAL(activated(QString)), this, SLOT(trybRysowania(QString)));

    rysujWielokat = false;
    trybRysowania("Krzywa Beziera");
    trafionyPkt = NULL;
    trybRys = BEZIER;
    trybEdycja = DODAJ_PKT;
    pixs_w = ui->widget->width();
    pixs_h = ui->widget->height();
    pixs = new uchar[4 * pixs_w * pixs_h];
    czyscEkran();
}

MainWindow::~MainWindow()
{
    delete pixs;
    delete ui;
}

bool MainWindow::pointsCmp(QPoint p1, QPoint p2)
{
    return (p1.x() > p2.x());
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter painter(this);
    QImage image(pixs, pixs_w, pixs_h, QImage::Format_ARGB32);
    painter.drawImage(0, 0, image);
    painter.setPen(Qt::black);

    czyscEkran();

    // Rysuj punkty wybrane przez użytkownika
    if(trybRys == BEZIER || trybRys == BSKLEJ || trybRys == SCAN_LINE) {
        for(uint i = 0; i < punkty.size(); ++i)
            rysujKolo((int)punkty[i].x(), (int)punkty[i].y(), 5);
    }

    // Rysuj wybraną figurę

    // Krzywe
    if(punkty.size() == 4) {
        if(trybRys == BEZIER)
            rysujKrzywaBeziera(punkty[0], punkty[1], punkty[2], punkty[3]);
        else if(trybRys == BSKLEJ)
            rysujKrzywaBSklejana(punkty[0], punkty[1], punkty[2], punkty[3]);
    }
    // Koło lub elipsa
    else if (punkty.size() == 2) {
        if(trybRys == KOLO) {
            int r = sqrt(pow((punkty[0].x() - punkty[1].x()), 2) + pow((punkty[0].y() - punkty[1].y()), 2));
            rysujKolo(punkty[0].x(), punkty[0].y(), r);
        }
        else if (trybRys == ELIPSA) {
            // Znajdź długośći promieni elipsy
            int r1, r2;
            if(punkty[0].x() > punkty[1].x())
                r1 = (int)punkty[0].x() - (int)punkty[1].x();
            else
                r1 = (int)punkty[1].x() - (int)punkty[0].x();

            if(punkty[0].y() > punkty[1].y())
                r2 = (int)punkty[0].y() - (int)punkty[1].y();
            else
                r2 = (int)punkty[1].y() - (int)punkty[0].y();

            rysujElipse(punkty[0].x(), punkty[0].y(), r1, r2);
        }
    }
    // Wielokąt
    if(trybRys == SCAN_LINE && punkty.size() > 2) {
        for(uint i = 0; i < punkty.size() - 1; ++i)
            rysujLinie((int)punkty[i].x(), (int)punkty[i].y(), (int)punkty[i + 1].x(), (int)punkty[i + 1].y());

        // Połącz ostatni punkt z pierwszym
        rysujLinie((int)punkty.front().x(), (int)punkty.front().y(), (int)punkty.back().x(), (int)punkty.back().y());
        scanLine(punkty);
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    // Dodaj nowy punkt do tablicy
     if(trybEdycja == DODAJ_PKT) {
         punkty.push_back(QPoint(event->x(), event->y()));
     }

     // Jeśli rysuje krzywe to może być max 4 punkty
     if(trybRys == BEZIER || trybRys == BSKLEJ) {
         if(punkty.size() >= 5) {
             QPoint tmp = punkty.back();
             punkty.clear();
             punkty.push_back(tmp);
         }
     }
     // Jeśli rysuje koło lub elipsę to po kliknięciu wybierany jest punkt środka okręgu
     else if (trybRys == KOLO || trybRys == ELIPSA) {
         reset();
         punkty.push_back(QPoint(event->x(), event->y()));
     }

     // Jeśli przesuwany lub usuwany jest punkt to znajdź trafiony punkt
     if(trybEdycja == PRZESUN_PKT || trybEdycja == USUN_PKT ||
             (trybRys == SCAN_LINE && punkty.size() > 2)) {
         for(uint i = 0; i < punkty.size(); ++i)
         {
             // Trafiono w punkt +- 5
             if(event->x() >= punkty[i].x() - 5 && event->x() <= punkty[i].x() + 5
                     && event->y() >= punkty[i].y() - 5 && event->y() <= punkty[i].y() + 5) {
                 // Zapamiętaj punkt
                 if(trybEdycja == PRZESUN_PKT)
                    trafionyPkt = &punkty[i];
                 // Usuń punkt
                 else if(trybEdycja == USUN_PKT)
                     punkty.erase(punkty.begin() + i);
                 break;
             }
         }
     }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    (void)event;
    // Dla przesuwania punktu
    trafionyPkt = NULL;
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(trafionyPkt != NULL) {
        *trafionyPkt = QPoint(event->x(), event->y());
        czyscEkran();
    }
    // Jeśli rysuje okrąg to przesuwaj drugi punkt
    if(trybRys == KOLO || trybRys == ELIPSA) {
        if(punkty.size() == 1)
            punkty.push_back(QPoint(event->x(), event->y()));
        punkty.back() = QPoint(event->x(), event->y());
    }
}

void MainWindow::rysujPiksel(int x, int y)
{
    // Znajdź położenie piksela w tablicy
    if(x <= pixs_w && y <= pixs_h) {
        int pos = 4 * (y*pixs_w + x);

        // Zmień kolor piksela na czaeny
        if(pos > 0 && pos <= 4 * pixs_w * pixs_h) {
            pixs[pos] = 0;
            pixs[pos + 1] = 0;
            pixs[pos + 2] = 0;
        }
    }
}

void MainWindow::rysujLinie(int x0, int y0, int x1, int y1)
{
    int dx, dy, y, x, endPx, endPy;
        double e, m;

        e = 0;
        x = x0;
        y = y0;

        // Kierunek rysowania linii w poziomie
        if(x1 > x0) {
            dx = x1 - x0;
            endPx = x1;
        }
        else {
            dx = x0 - x1;
            endPx = 2*x0 - x1;
        }

        // Kierunek rysowania linii w pionie
        if(y1 > y0) {
            dy = y1 - y0;
            endPy = y1;
        }
        else {
            dy = y0 - y1;
            endPy = 2*y0 - y1;
        }

        if(dy < dx) {
            m = dy / (double)dx;
            for(; x <= endPx; x++)
            {
                if(x1 > x0) {
                    if(y1 > y0)
                        rysujPiksel(x, y);
                    else
                        rysujPiksel(x, 2*y0 - y);
                }
                else {
                    if(y1 > y0)
                        rysujPiksel(2*x0 - x, y);
                    else
                        rysujPiksel(2*x0 - x, 2*y0 - y);
                }

                e += m;
                if(e >= 0.5) {
                    y++;
                    e -= 1;
                }
            }
        }
        else {
            m = dx / (double)dy;
            for(y = y0; y <= endPy; y++)
            {
                if(x1 > x0) {
                    if(y1 > y0)
                        rysujPiksel(x, y);
                    else
                        rysujPiksel(x, 2*y0 - y);
                }
                else {
                    if(y1 > y0)
                        rysujPiksel(2*x0 - x, y);
                    else
                        rysujPiksel(2*x0 - x, 2*y0 - y);
                }

                e += m;
                if(e >= 0.5) {
                    x++;
                    e -= 1;
                }
            }
        }
}

void MainWindow::rysujKolo(int x0, int y0, int r)
{
    if(r > 0){
        for(int x = 0; x <= r; ++x)
        {
            int y = (int)floor(sqrt(r*r - x*x));

            // Prawy góra
            rysujPiksel(y + x0, -x + y0);
            rysujPiksel(x + x0, -y + y0);

            // Lewy góra
            rysujPiksel(-y + x0, -x + y0);
            rysujPiksel(-x + x0, -y + y0);

            // Prawy dół
            rysujPiksel(y + x0, x + y0);
            rysujPiksel(x + x0, y + y0);

            // Lewy dół
            rysujPiksel(-y + x0, x + y0);
            rysujPiksel(-x + x0, y + y0);
        }
    }
}

void MainWindow::rysujElipse(int x0, int y0, int r1, int r2)
{
    int x=x0, y=y0;
    for(int i = 0; i <= 360; ++i)
    {
        // Kąt obrotu kolejnych punktów w radianach
        double alfa = (i / 360.0) * (2 * M_PI);

        // Zapomiętanie położenia poprzednich punktów
        int oldX = x;
        int oldY = y;

        // Obrót punktów
        x = r1 * cos(alfa);
        y = r2 * sin(alfa);

        // Obrót elipsy
        int tmp = x;
        x = x * cos(beta) - y * sin(beta);
        y = tmp * sin(beta) + y * cos(beta);

        if(i > 0)
            rysujLinie(oldX + x0, oldY + y0, x + x0, y + y0);
    }
}

void MainWindow::rysujKrzywaBeziera(QPoint p0, QPoint p1, QPoint p2, QPoint p3)
{
    QPoint tmp = p0;
    for(int i = 0; i < 100; ++i)
    {
        double t = i / 100.0;
        QPoint pkt = pow((1 - t), 3) * p0 + 3*t * pow((1 - t), 2) * p1 + 3*t*t * (1 - t) * p2 + pow(t, 3) * p3;

        rysujLinie((int)pkt.x(), (int)pkt.y(), (int)tmp.x(), (int)tmp.y());
        tmp = pkt;
    }
}

void MainWindow::rysujKrzywaBSklejana(QPoint p0, QPoint p1, QPoint p2, QPoint p3)
{
    QPoint tmp = p0;
    double step = 1.0 / 100.0;
    double t = 0;
    for(int i = 0; i <= 100; ++i)
    {
        QPoint pkt = ((-p0 + 3*p1 - 3*p2 + p3) * t*t*t + (3*p0 - 6*p1 + 3*p2) * t*t + (-3*p0 + 3*p2) * t + (p0 + 4*p1 + p2)) / 6;

        if( i > 0)
            rysujLinie((int)pkt.x(), (int)pkt.y(), (int)tmp.x(), (int)tmp.y());

        tmp = pkt;
        t += step;
    }
}

void MainWindow::rysujPoziomo(int x0, int y0, int x1)
{
    if(x0 > x1)
        std::swap(x0, x1);
    for(int x = x0; x <= x1; ++x)
        rysujPiksel(x, y0);
}

void MainWindow::scanLine(std::vector<QPoint> punkty)
{
    if(punkty.size() > 2) {
        int startX, endX, startY, endY;

        // Szukaj punktów skrajnych
        startX = endX = punkty.front().x();
        startY = endY = punkty.front().y();
        for(uint i = 1; i < punkty.size(); ++i)
        {
            // Punkty skrajne w poziomie
            if(punkty[i].x() > endX)
                endX = punkty[i].x();
            if(punkty[i].x() < startX)
                startX = punkty[i].x();

            // Punkty skrajne w pionie
            if(punkty[i].y() > endY)
                endY = punkty[i].y();
            if(punkty[i].y() < startY)
                startY = punkty[i].y();
        }

        std::vector <QPoint> punktyWypelnienie;

        // Przechodź po obszarze figury od góry w dół, od lewej do prawej
        for(int y = startY+1; y < endY; ++y)
        {
            punktyWypelnienie.clear();
            // Przechodź po krawędziach figury
            for(uint i = 0; i < punkty.size(); ++i)
            {
                int minX, maxX, minY, maxY, x, j, dx, dy;
                double m;

                j = (i + 1) % punkty.size();
                dx = punkty[i].x() - punkty[j].x();
                dy = punkty[i].y() - punkty[j].y();
                m = dx / (double)dy;

                if(punkty[i].x() > punkty[j].x()) {
                    maxX = punkty[i].x();
                    minX = punkty[j].x();
                }
                else {
                    maxX = punkty[j].x();
                    minX = punkty[i].x();
                }

                if(punkty[i].y() > punkty[j].y()) {
                    maxY = punkty[i].y();
                    minY = punkty[j].y();
                }
                else {
                    maxY = punkty[j].y();
                    minY = punkty[i].y();
                }

                x = floor(m * (y - punkty[i].y()) + punkty[i].x());

                // Sprawdź, czy punkt znajduje się na linii
                if(x >= minX && x <= maxX && y >= minY && y <= maxY) {
                    punktyWypelnienie.push_back(QPoint(x, y));
                }
            }

            // Posortuj punkty w poziomie
            std::sort(punktyWypelnienie.begin(), punktyWypelnienie.end(), pointsCmp);

            // Wypełnij figurę dla par punktów
            for(uint i = 0; i < punktyWypelnienie.size() && !(punktyWypelnienie.size() % 2); i += 2) {
                rysujPoziomo(punktyWypelnienie[i].x(), punktyWypelnienie[i].y(), punktyWypelnienie[i + 1].x());
            }
        }
    }
}

void MainWindow::radioOdslon()
{
    ui->radioDodaj->show();
    ui->radioPrzesun->show();
    ui->radioUsun->show();
}

void MainWindow::radioSchowaj()
{
    ui->radioDodaj->hide();
    ui->radioPrzesun->hide();
    ui->radioUsun->hide();
}

void MainWindow::czyscEkran()
{
    for(int i = 0; i < pixs_w * pixs_h; ++i)
    {
        pixs[4*i] = 255;
        pixs[4*i + 1] = 255;
        pixs[4*i + 2] = 255;
        pixs[4*i + 3] = 255;
    }
}

void MainWindow::reset()
{
    punkty.clear();
    czyscEkran();
}

void MainWindow::trybPkt()
{
    if(ui->radioDodaj->isChecked())
        trybEdycja = DODAJ_PKT;
    else if(ui->radioPrzesun->isChecked())
        trybEdycja = PRZESUN_PKT;
    else if(ui->radioUsun->isChecked())
        trybEdycja = USUN_PKT;
}

void MainWindow::trybRysowania(QString wybor)
{
    if(wybor == "Krzywa Beziera") {
        reset();
        trybRys = BEZIER;
        radioOdslon();
        ui->verticalSlider->hide();
    }
    else if (wybor == "Krzywa B-Sklejana") {
        reset();
        trybRys = BSKLEJ;
        radioOdslon();
        ui->verticalSlider->hide();
    }
    else if(wybor == "Scan Line") {
        reset();
        trybRys = SCAN_LINE;
        radioOdslon();
        ui->wypelnij->show();
        ui->verticalSlider->hide();
    }
    else if(wybor == "Rysuj koło" || wybor == "Rysuj elipsę") {
        reset();

        if(wybor == "Rysuj koło") {
            trybRys = KOLO;
            ui->verticalSlider->hide();
        }
        else {
            trybRys = ELIPSA;
            ui->verticalSlider->show();
        }

        radioSchowaj();
        trybEdycja = DODAJ_PKT;
        ui->radioDodaj->setChecked(true);
    }
}

void MainWindow::sliderObroc(int i)
{
    beta = i / (M_PI * 5.0);
}
