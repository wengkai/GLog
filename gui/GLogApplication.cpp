#include "GLogApplication.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <fstream>

GLogApplication::GLogApplication(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    tableview = new QTableView(this);
    setCentralWidget(tableview);
    connect(ui.actionOpen, &QAction::triggered, this, &GLogApplication::openFileAction);
    connect(ui.actionOpen_Append, &QAction::triggered, this, &GLogApplication::openFileAppendAction);
    model = new AdifModel(this);
    tableview->setModel(model);
    auto work = new ParserThread;
    work->moveToThread(&parserSub);
    connect(&parserSub, &QThread::finished, &parserSub, &QThread::deleteLater);
    connect(this, &GLogApplication::openFileActionSignal, work, &ParserThread::openFile);
    connect(this, &GLogApplication::openFileAppendActionSignal, work, &ParserThread::appendFile);
    connect(work, &ParserThread::done, this, &GLogApplication::modelUpdated);
    parserSub.start();
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
    tableview->setVisible(false);
    emit openFileActionSignal(filename, model);
}

void GLogApplication::openFileAction() 
{
    openFile(QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Adif Files (*.adi *.adif);;All Files (*.*)")));
}

void GLogApplication::openFileAppend(const QString &filename)
{
    tableview->setVisible(false);
    emit openFileAppendActionSignal(filename, model);
}

void GLogApplication::openFileAppendAction() 
{
    openFileAppend(QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Adif Files (*.adi *.adif);;All Files (*.*)")));
}


void GLogApplication::modelUpdated()
{
    //resizeTableView();
    tableview->setVisible(true);
}

ParserThread::ParserThread(QObject *parent) : QObject(parent)
{}

void ParserThread::openFile(QString filename, AdifModel* model)
{
    QFileInfo file(filename);
    std::ifstream in(file.filesystemAbsoluteFilePath());
    if ( in ) {
        driver.switch_streams(in, std::cerr);
        parser.parse();
        model->setRecords(driver.data.rbegin(), driver.data.rend());
    }
    emit done();
    in.close();
}

void ParserThread::appendFile(QString filename, AdifModel* model)
{
    QFileInfo file(filename);
    std::ifstream in(file.filesystemAbsoluteFilePath());
    if ( in ) {
        driver.switch_streams(in, std::cerr);
        parser.parse();
        model->addRecords(driver.data.rbegin(), driver.data.rend());  
    }
    emit done();
    in.close();
}
