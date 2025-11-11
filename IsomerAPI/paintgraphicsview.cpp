#include "paintgraphicsview.h"
#include "vectorstructures.h"

#include <QPainter>
#include <QPen>
#include <QPolygon>

paintgraphicsview::paintgraphicsview(const QVector<Level>& levels, const QVector<Transition>& transitions, QGraphicsItem *parent)
    : QGraphicsItem(parent),
    L_Levels(levels),
    L_Transitions(transitions)
{}



void paintgraphicsview::paint(QPainter *painter,
                              const QStyleOptionGraphicsItem *,
                              QWidget *)
{
    painter->setRenderHint(QPainter::Antialiasing);

    int trHOffset = 20;

    int lineLeft = 0;
    int trCount = L_Transitions.size();
    qDebug() << "[paintgraphicsview: transition count]: "<< trCount;
    int lineRight = trCount*trHOffset + 25;
    qDebug() << "[paintgraphicsview: line left/right]" << lineLeft << lineRight;
    int indentation = 25;


    int titleOffset = 20;
    int spinOffset = -40;
    int infoHOffset = 10;
    int textVOffset = 5;
    int arrowHeadSize = 6;

    QColor lineColor(0,0,0);
    QColor levelTextColor(0,0,0);
    QColor transitionColor(34, 139, 34);

    double yBase = boundingRect().height() - 50;

    for (const Level &lvl : L_Levels) {
        int y = static_cast<int>(yBase - lvl.lvlEnergy);
        painter->setPen(QPen(lineColor, 2));
        painter->drawLine(lineLeft, y, lineRight, y);
        painter->setPen(levelTextColor);
        painter->drawText(spinOffset, y + textVOffset, lvl.spin);
        QString text = QString("%1 keV, T<sub>1/2</sub> = %2 \u03BCs")
                           .arg(lvl.lvlEnergy, 0, 'f', 0)
                           .arg(lvl.halfLife);




        painter->drawText(lineRight + infoHOffset, y + textVOffset, text);
    }

    //  draw title
    painter->drawText((lineLeft+lineRight)/2,yBase + titleOffset,QString("##El"));

    painter->setPen(QPen(transitionColor, 2));
    painter->setBrush(transitionColor);

    int xOffset = 0;
    for (const Transition &tr : L_Transitions) {
        int y1 = static_cast<int>(yBase - tr.level);
        int y2 = static_cast<int>(yBase - tr.emission);

        int x = lineLeft + indentation + xOffset;

        painter->drawLine(x, y1, x, y2);

        QPolygon arrowHead;
        arrowHead << QPoint(x - arrowHeadSize, y2 - arrowHeadSize)
                  << QPoint(x + arrowHeadSize, y2 - arrowHeadSize)
                  << QPoint(x, y2);
        painter->drawPolygon(arrowHead);

        painter->setPen(transitionColor);

        painter->drawText(x + 8, (y1 + y2) / 2, tr.label);
        painter->setPen(QPen(transitionColor, 2));

        xOffset += 20;
    }
}

QRectF paintgraphicsview::boundingRect() const
{
    return QRectF(-105-50, 0, 500, 600);
}
