#include "cpp_isomerapi.h"
#include "levelschemewindow.h"
#include "./ui_cpp_isomerapi.h"
#include "./ui_levelschemewindow.h"

#include <QSqlError>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QMessageBox>

cpp_IsomerAPI::cpp_IsomerAPI(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::cpp_IsomerAPI)
    , model(nullptr)
{
    ui->setupUi(this);

    // Database initialization

    // there is a smarter way to write this path with QDir() but I do not know how to escape the builder.

    // dbPath = QCoreApplication::applicationDirPath() + "/database_store/Isomer_DB_WIDGET.sqlite";
    qDebug() << "[cpp_isomerapi BUILD PATH:]" << QDir::currentPath();
    dbPath = "C:/Users/hudso/OneDrive/working/IsomerProject/IsomerAPI/database_store/Isomer_DB_WIDGET.sqlite";
    db = QSqlDatabase::addDatabase("QSQLITE","IsomDB");
    // point to external IsomDb in LISE
    db.setDatabaseName(dbPath);
    if(!db.open()){
        qCritical() << "Failted to open DB:" << db.lastError().text();
    }
    qDebug() << "[cpp_isomerapi DBPATH:] " << dbPath;
    model = new QSqlTableModel(this, db);
    model->setTable("Isomers");
    model->select();
    ui->tableView->setModel(model);

    // Contained utility/attribute declaration
    query = QSqlQuery(db);
    filterBounds = ui->gb_tableFilters->findChildren<QLineEdit*>();

    // Source combo box config
    ui->cb_sourceFilter->addItem("All Sources");
    ui->cb_sourceFilter->addItem("Semicolon;List;Sources");

    if(query.exec("SELECT DISTINCT SOURCE FROM Isomers")) {
        while (query.next()) {
            QString src = query.value(0).toString();
            entrySources.append(src);
        }
    }
    // qDebug() << "[Init l41: Entry source check]: " << entrySources;
    ui->cb_sourceFilter->addItems(entrySources);

    // Scientif notation

    sumStatRefresh();
    connect(ui->pb_clearFilters, &QPushButton::clicked, this, &cpp_IsomerAPI::clearFilters);
    connect(ui->pb_applyFilters, &QPushButton::clicked, this, &cpp_IsomerAPI::applyFilters);
    connect(ui->pb_levelScheme, &QPushButton::clicked, this, &cpp_IsomerAPI::openDrawing);
    // connect(ui->tableView, &QAbstractItemView::scrollToBottom,this,&cpp_IsomerAPI::sumStatRefresh);
}

cpp_IsomerAPI::~cpp_IsomerAPI()
{
    db.close();
    delete ui;
}

void cpp_IsomerAPI::sumStatRefresh() {
    ui -> le_entryCounts->setText(QString::number(model->rowCount()));

    QString queryStr = "SELECT MAX(E_GAMMA) FROM Isomers";
    QVariant val = queryModel(queryStr);

    qDebug() << "[IN sumStatRefresh(): val is] " << val;
    // hellish bug breaks my table refresh
    // ui -> le_maxGAMMA->setText(val.toString());
    // db.close();
}

QVariant cpp_IsomerAPI::queryModel(const QString &queryRequest) {
    QString fullQuery = queryRequest;
    QString filter = model->filter();
    if (!filter.isEmpty()) {
        fullQuery += " WHERE " + filter;
    }
    if (query.exec(fullQuery) && query.next()) {
        qDebug() << "[queryModel -- query value]" << query.value(0);
        return {query.value(0)};
    }
    return {};
}

void cpp_IsomerAPI::sourceFilter() {
    QString text = ui->cb_sourceFilter->currentText();
    qDebug() << "[sourceFilter l70 TEXT]: " << text;
    QStringList reqSources = text.split(';', Qt::SkipEmptyParts);

    if (reqSources.contains("All Sources")) {
        model -> setFilter("");
    } else {
        QStringList quoted;
        for (const QString &src : reqSources) {
            quoted.append("'" + src + "'");

        }
        model->setFilter(QString("SOURCE IN (%1)").arg(quoted.join(',')));
        qDebug() << "[sourceFilter l77: NO ALL SOURCES]: " << quoted;
        qDebug() << "[sourceFilter l78: Check format for SQL]: " << quoted.join(',');
    }
    qDebug() << "\n";

}

void cpp_IsomerAPI::applyFilters() {
    sourceFilter();

    QMap<QString, QString> filterMap = {
        {"le_T12", "T12"},
        {"le_GE", "E_GAMMA"},
        {"le_IGAM", "I_GAMMA"},
        {"le_FINE", "LEVEL"},
        {"le_numA", "A_IT"},
        {"le_numZ", "Z_IT"}
    };

    QString filterExpr = model->filter();
    for (QLineEdit* le : std::as_const(filterBounds)) {
        if (le->text().isEmpty()) {
            continue;
        }

        QString objName = le->objectName();
        QString baseName = objName.left(objName.length() - 1);
        QString suffix = objName.right(1);


        qDebug() << "[applyFilters l133: name parse check]" << objName << baseName << suffix;
        qDebug() << "[applyFilters l134: check filterExpr]" << filterExpr;

        if (filterMap.contains(baseName)) {
            QString col = filterMap[baseName];
            QString value = le->text();
            QString condition = (suffix == "1") ? QString("%1 > %2").arg(col,value)
                                                : QString("%1 < %2").arg(col,value);
            qDebug() << "[applyFilters l139: col check]" << col;
            qDebug() << "[applyFilters l140: value check]" << value;
            qDebug() << "[applyFilters l141: conditional check]" << condition;

            if (!filterExpr.isEmpty()) {
                filterExpr += " AND ";
            }
            filterExpr += condition;



        } else if (baseName == "le_num") {
            QString col = (suffix == "A") ? "A_IT" : "Z_IT";
            QString condition = QString("%1 IS %2").arg(col, le->text());


            if (!filterExpr.isEmpty()) {
                filterExpr += " AND ";
            }
            filterExpr += condition;
            qDebug() << "[applyFilters l160: col check]" << col;
            qDebug() << "[applyFilters l161: value check]" << le->text();
        }
        qDebug() << "[applyFilters l151: Check filterExpr]:" << filterExpr;
        qDebug() << "\n";
    }

    model->setFilter(filterExpr);
    model->select();
    ui->tableView->setModel(model);
}

void cpp_IsomerAPI::clearFilters(){
    for (QLineEdit* le : std::as_const(filterBounds)) {
        QString objName = le->objectName();
        qDebug() << "[clearFilters l170: Check object name]" << objName;
        if (le->objectName() != "") {
            le->setText("");
        }
    }
    applyFilters();
}

void cpp_IsomerAPI::openDrawing(){
    // iteration function
    QVector<Level> levels;
    QVector<Transition> transitions;
    std::tie(levels, transitions) = prepData();

    LevelSchemeWindow *mw = new LevelSchemeWindow(levels, transitions, this);
    mw->show();
}

std::tuple<QVector<Level>,QVector<Transition>> cpp_IsomerAPI::prepData(){

    QString fullQuery = "SELECT A_IT,Z_IT,E_GAMMA,D_EG,T12,D_T12,LEVEL,D_LEVEL,JPI FROM Isomers";
    QString filter = model->filter();
    if (!filter.isEmpty()) {
        fullQuery += " WHERE " + filter;
    }

    QVector<Level> prepLevel;
    QVector<Transition> prepTransition;
    query.exec(fullQuery);

    prepLevel.push_back({0,"",""});

    while (query.next()) {

        double tmpLevel = query.value("LEVEL").toDouble();
        double tmpGamma = query.value("E_GAMMA").toDouble();
        double tmpFinal = tmpLevel - tmpGamma;
        QString tmpEmission = QString("%1 keV").arg(tmpGamma);

        prepTransition.push_back({tmpLevel, tmpFinal, tmpEmission});
        prepLevel.push_back({tmpLevel,query.value("JPI").toString(),query.value("T12").toString()});

    }

    for (const Transition &tr : prepTransition) {
        qDebug() << "[prepData: transition check]" << tr.level << tr.emission << tr.label;
    }
    qDebug();
    for (const Level &lvl : prepLevel) {
        qDebug() << "[prepData: level check]" << lvl.lvlEnergy << lvl.spin << lvl.halfLife;
    }

    return std::make_tuple(prepLevel, prepTransition);

}
