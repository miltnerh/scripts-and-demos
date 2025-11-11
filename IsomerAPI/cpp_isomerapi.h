#ifndef CPP_ISOMERAPI_H
#define CPP_ISOMERAPI_H

#include <QWidget>
#include <QApplication>
#include <QWidget>
#include <QApplication>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QList>
#include <QVector>

#include "vectorstructures.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class cpp_IsomerAPI;
}
QT_END_NAMESPACE

class cpp_IsomerAPI : public QWidget
{
    Q_OBJECT

public:
    cpp_IsomerAPI(QWidget *parent = nullptr);
    ~cpp_IsomerAPI();

    std::tuple<QVector<Level>,QVector<Transition>> prepData();


private slots:
    void applyFilters();
    void openDrawing();
    void sumStatRefresh();
    void clearFilters();

private:
    void sourceFilter();
    QVariant queryModel(const QString &queryRequest);
    Ui::cpp_IsomerAPI *ui;
    QSqlDatabase db;
    QSqlQuery query;
    QSqlTableModel *model;
    QList<QLineEdit*> filterBounds;
    QString dbPath;
    QStringList entrySources;
};
#endif // CPP_ISOMERAPI_H
