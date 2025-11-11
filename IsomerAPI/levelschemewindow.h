#ifndef LEVELSCHEMEWINDOW_H
#define LEVELSCHEMEWINDOW_H

#include "paintgraphicsview.h"
#include "vectorstructures.h"

#include <QMainWindow>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui {
class LevelSchemeWindow;
}
QT_END_NAMESPACE

class LevelSchemeWindow : public QMainWindow
{
    Q_OBJECT

public:
    LevelSchemeWindow(const QVector<Level>& level, const QVector<Transition>& transition,QWidget *parent = nullptr);
    ~LevelSchemeWindow();    

private:
    QString _filterQuery;
    QString _path;
    Ui::LevelSchemeWindow *ui;

    void saveImage();
    // void paintEvent(QPaintEvent *event) override;
};
#endif // LEVELSCHEMEWINDOW_H
