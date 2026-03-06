#include "GLogApplication.h"
#include "MultiLineDelegate.h"
#include "GCommandLineParser.h"
#include <QVBoxLayout>
#include <QOpenGLWidget>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <fstream>

GLogApplication::GLogApplication(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    tableview = new DropAbleTableView(this);
    setCentralWidget(tableview);
    connect(ui.actionOpen, &QAction::triggered, this, &GLogApplication::openFileAction);
    connect(ui.actionMerge, &QAction::triggered, this, &GLogApplication::mergeFileAction);
    connect(ui.actionSave_As, &QAction::triggered, this, &GLogApplication::saveAsAction);
    model = new AdifModel(this);
    model->getControl()->moveToThread(&modelSub);
    tableview->setModel(model);
    connect(&modelSub, &QThread::finished, &modelSub, &QThread::deleteLater);
    connect(this, &GLogApplication::openFileActionSignal, model->getControl(), &AdifModelC::openFile);
    connect(this, &GLogApplication::mergeFileActionSignal, model->getControl(), &AdifModelC::appendFile);
    connect(this, &GLogApplication::saveAsActionSignal, model->getControl(), &AdifModelC::saveAs);
    //connect(model, &AdifModel::appendFileSignal, model->getControl(), &AdifModelC::appendFile);
    connect(model, &AdifModel::insertFileSignal, model->getControl(), &AdifModelC::insertFile);
    connect(model->getControl(), &AdifModelC::modelUpdated, this, &GLogApplication::modelUpdated);
    connect(model->getControl(), &AdifModelC::saveDone, this, &GLogApplication::saveDone);
    connect(model->getControl(), &AdifModelC::setCilpboard, this, &GLogApplication::setCilpboard);
    connect(tableview, &DropAbleTableView::deleteRowsSignal, model, &AdifModel::deleteRows);
    connect(tableview, &DropAbleTableView::newViewWithRowsSignal, model->getControl(), &AdifModelC::newViewWithRows);
    connect(tableview, &DropAbleTableView::pasteRowsSignal, model->getControl(), &AdifModelC::pasteRows);
    connect(tableview, &DropAbleTableView::copyRowsSignal, model->getControl(), &AdifModelC::copyRows);
    connect(tableview->getHeaderView(), &GHeaderView::sortByColumnSignal, model, &AdifModel::sortSelectedColumn);
    connect(tableview->getHeaderView(), &GHeaderView::removeColumnSignal, model, &AdifModel::removeSelectedColumn);
    
    GCommandLineParser parser;
    parser.process(QCoreApplication::arguments());
    bool remove = parser.isSet("remove");
    if (parser.isSet("i")) {
        auto files = parser.values("i");
        for (auto& file: files) {
            //QMessageBox::information(this, "pre open", file, QMessageBox::StandardButton::Ok);
            mergeFile(file, remove);
        }
    }

    searchBar = new SearchBar(this);
    connect(ui.actionSearch, &QAction::triggered, searchBar, &SearchBar::show);
    connect(searchBar, &SearchBar::findNext, tableview, &DropAbleTableView::findNext);
    connect(searchBar, &SearchBar::selectAll, model->getControl(), &AdifModelC::selectAll);
    connect(searchBar, &SearchBar::deselectAll, model->getControl(), &AdifModelC::deselectAll);
    connect(tableview, &DropAbleTableView::selected, searchBar, &SearchBar::enableButtons);
    connect(tableview, &DropAbleTableView::findNextSignal, model->getControl(), &AdifModelC::findNext);
    connect(model->getControl(), &AdifModelC::foundNext, tableview, &DropAbleTableView::foundNext);
    connect(model->getControl(), &AdifModelC::selectRows, tableview, &DropAbleTableView::selectRows);
    connect(model->getControl(), &AdifModelC::deselectRows, tableview, &DropAbleTableView::deselectRows);

    auto mapWidget = new QMainWindow(this);
    mapWidget->setWindowTitle(tr("Map View"));
    mapView = new MapGraphicsView(mapWidget);
    QOpenGLWidget *gl = new QOpenGLWidget();
    QSurfaceFormat format;
    format.setSamples(4);
    gl->setFormat(format);
    mapView->setViewport(gl);
    mapWidget->setCentralWidget(mapView);
    connect(ui.actionMap_View, &QAction::triggered, [=] {
        mapView->testMarker();
        mapWidget->show();
    });
    connect(mapView, &MapGraphicsView::mouseMoveTo, [=](qreal x, qreal y) {
        mapWidget->setWindowTitle(tr("Map View") + " x:" + QString::number(x) + " y:" + QString::number(y));
    });

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

void GLogApplication::mergeFile(const QString &filename, bool remove)
{
    //tableview->setVisible(false);
    emit mergeFileActionSignal(filename, remove);
}

void GLogApplication::saveAsFile(const QString &filename)
{
    emit saveAsActionSignal(filename);
}

void GLogApplication::mergeFileAction() 
{
    mergeFile(QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Adif Files (*.adi *.adif);;All Files (*.*)")));
}


void GLogApplication::modelUpdated()
{
    //resizeTableView();
    //tableview->setVisible(true);
}

void GLogApplication::saveAsAction()
{
    if (model->rowCount() == 0) {
        return;
    }
    saveAsFile(QFileDialog::getSaveFileName(this, tr("Save As"), "", tr("Adif Files (*.adi *.adif);;Csv Files (*.csv)")));
}

void GLogApplication::saveDone()
{
    QMessageBox::information(this, tr("Save"), tr("Done"), QMessageBox::StandardButton::Ok);
}

void GLogApplication::setCilpboard(QMimeData *mimeData)
{
    QApplication::clipboard()->setMimeData(mimeData);
    QMessageBox::information(this, tr("Copy"), tr("Done"), QMessageBox::StandardButton::Ok);
}
