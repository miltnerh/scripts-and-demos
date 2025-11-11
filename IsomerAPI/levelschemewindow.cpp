#include "levelschemewindow.h"
#include "./ui_levelschemewindow.h"
// #include "vectorstructures.h"


#include <QPainter>
#include <QPaintEvent>
#include <QGraphicsScene>

LevelSchemeWindow::LevelSchemeWindow(const QVector<Level>& levels, const QVector<Transition>& transitions,QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::LevelSchemeWindow)

{
    ui->setupUi(this);
    setMinimumSize(500, 500);
    resize(800,400);
    qDebug() << _filterQuery << _path;

    //
    QGraphicsScene *scene = new QGraphicsScene(this);
    paintgraphicsview *item = new paintgraphicsview(levels, transitions);
    scene->addItem(item);

    ui->graphicsView->setScene(scene);

    QAction *action_savePic2 = new QAction("TEST SAVE", this);
    ui->toolBar->addAction(action_savePic2);
    connect(ui->action_savePic, &QAction::triggered,this,&LevelSchemeWindow::saveImage);

}


LevelSchemeWindow::~LevelSchemeWindow()
{
    delete ui;
}

void LevelSchemeWindow::saveImage(){
    qDebug() << "[saveImage: triggered]";
    QPixmap pixMap = ui->graphicsView->viewport()->grab(ui->graphicsView->sceneRect().toRect());
    pixMap.save("lvlSchemeTest.png");

    // QRectF sceneRect(ui->graphicsView->scene()->sceneRect());
    // QImage image(sceneRect.size().toSize(), QImage::Format_ARGB32);
    // image.fill(Qt::white);
    // QPainter painter(&image);
    // painter.setRenderHint(QPainter::Antialiasing);
    // QGraphicsScene currScene = ui->graphicsView->scene();
    // currScene.render(&painter);
    // image.save("lvlSchemeTest.png");
}

// void LevelSchemeWindow::paintEvent(QPaintEvent *)
// {
//     QPainter painter(this);
//     painter.setRenderHint(QPainter::Antialiasing);

//     struct Level {
//         double energy;
//         QString spin;
//         QString halfLife;
//     };

//     QVector<Level> levels = {
//         {0,   "0+", "Stable"},
//         {100, "2+", "5 ns"},
//         {250, "4+", "1 µs"},
//         {500, "6+", "20 µs"}
//     };

//     int left = 150;
//     int right = 300;
//     int spinOffset = 90;
//     int infoOffset = 310;
//     int textVOffset = 5;
//     int arrowHeadSize = 6;
//     // QColor textColor(196, 196, 196);
//     // QColor lineColor(200, 200, 200);
//     QColor lineColor(0,0,0);
//     QColor textColor(0,0,0);
//     QColor arrowColor(34, 139, 34);


//     double yBase = height() - 50;

//     for (const Level &lvl : levels) {
//         int y = static_cast<int>(yBase - lvl.energy);
//         painter.setPen(QPen(lineColor, 2));
//         painter.drawLine(left, y, right, y);
//         painter.setPen(textColor);
//         painter.drawText(spinOffset, y + textVOffset, lvl.spin);
//         QString text = QString("%1 keV, t½ = %2")
//                            .arg(lvl.energy, 0, 'f', 0)
//                            .arg(lvl.halfLife);
//         painter.drawText(infoOffset, y + textVOffset, text);
//     }

//     struct Transition {
//         int fromIdx;
//         int toIdx;
//         QString label;
//         int xOffset;
//     };

//     QVector<Transition> transitions = {
//         {3, 2, "250 keV", 0},
//         {2, 1, "150 keV", -20},
//         {1, 0, "100 keV", 20},
//         {3, 1, "400 keV", 40}
//     };

//     painter.setPen(QPen(arrowColor, 2));
//     painter.setBrush(arrowColor);

//     for (const Transition &tr : transitions) {
//         int y1 = static_cast<int>(yBase - levels[tr.fromIdx].energy);
//         int y2 = static_cast<int>(yBase - levels[tr.toIdx].energy);
//         int x = (left + right) / 2 + tr.xOffset;

//         painter.drawLine(x, y1, x, y2);

//         QPolygon arrowHead;
//         arrowHead << QPoint(x - arrowHeadSize, y2 - arrowHeadSize)
//                   << QPoint(x + arrowHeadSize, y2 - arrowHeadSize)
//                   << QPoint(x, y2);
//         painter.drawPolygon(arrowHead);

//         painter.setPen(textColor);
//         painter.drawText(x + 8, (y1 + y2) / 2, tr.label);

//         painter.setPen(QPen(arrowColor, 2));
//     }
// }
