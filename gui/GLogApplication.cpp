#include "GLogApplication.h"
#include "MultiLineDelegate.h"
#include <QVBoxLayout>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <fstream>

GLogApplication::GLogApplication(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    //setAcceptDrops(true);
    tableview = new DropAbleTableView(this);
    setCentralWidget(tableview);
    connect(ui.actionOpen, &QAction::triggered, this, &GLogApplication::openFileAction);
    connect(ui.actionOpen_Append, &QAction::triggered, this, &GLogApplication::openFileAppendAction);
    model = new AdifModel(this);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(model);
    model->getControl()->moveToThread(&modelSub);
    proxyModel->setSourceModel(model);
    tableview->setModel(model);
    tableview->setItemDelegate(new MultiLineDelegate(tableview));
    connect(&modelSub, &QThread::finished, &modelSub, &QThread::deleteLater);
    connect(this, &GLogApplication::openFileActionSignal, model->getControl(), &AdifModelC::openFile);
    connect(this, &GLogApplication::openFileAppendActionSignal, model->getControl(), &AdifModelC::appendFile);
    connect(model, &AdifModel::appendFileSignal, model->getControl(), &AdifModelC::appendFile);
    connect(model, &AdifModel::insertFileSignal, model->getControl(), &AdifModelC::insertFile);
    connect(model->getControl(), &AdifModelC::modelUpdated, this, &GLogApplication::modelUpdated);
    connect(tableview, &DropAbleTableView::deleteRows, model, &AdifModel::deleteRows);
    modelSub.start();
}

GLogApplication::~GLogApplication()
{}

void GLogApplication::resizeTableView()
{
    tableview->setVisible(false);
    tableview->resizeColumnsToContents();
    tableview->resizeRowsToContents();
    tableview->setVisible(true);
}

void GLogApplication::openFile(const QString &filename)
{
    //tableview->setVisible(false);
    emit openFileActionSignal(filename);
}

void GLogApplication::openFileAction() 
{
    openFile(QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Adif Files (*.adi *.adif);;All Files (*.*)")));
}

void GLogApplication::openFileAppend(const QString &filename)
{
    //tableview->setVisible(false);
    emit openFileAppendActionSignal(filename);
}

void GLogApplication::openFileAppendAction() 
{
    openFileAppend(QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Adif Files (*.adi *.adif);;All Files (*.*)")));
}


void GLogApplication::modelUpdated()
{
    //resizeTableView();
    //tableview->setVisible(true);
}
