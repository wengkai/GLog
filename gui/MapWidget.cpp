#include "MapWidget.h"
#include "ctydb.h"
#include <QGraphicsEllipseItem>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMessageBox>
#include <QBrush>
#include <QFont>
#include <QOpenGLWidget>
#include <QActionGroup>

MapWidget::MapWidget(AdifModel * model, QWidget *parent) : m_model(model) ,  QMainWindow(parent)
{
    ui.setupUi(this);
    title = windowTitle();
    connect(ui.graphicsView, &MapGraphicsView::mouseMoveTo, [=](qreal x, qreal y) {
        QString xt = x > 0 ? "° E" : "° W";
        QString yt = y > 0 ? "° N" : "° S";
        setWindowTitle(title + "   " + QString::number(qAbs(x)) + xt + ",   " + QString::number(qAbs(y)) + yt);
    });

    connect(this, &MapWidget::initCtyMarkersCon, this, &MapWidget::initCtyMarkersBatch, Qt::QueuedConnection);

    connect(this, &MapWidget::dataVisualizeRe, this, &MapWidget::dataVisualize, Qt::QueuedConnection);

    connect(ui.actionDrag_Mode, &QAction::triggered, [=](){
        ui.graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    });
    connect(ui.actionSelect_Mode, &QAction::triggered, [=](){
        ui.graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
    });

    auto viewDragModeGroup = new QActionGroup(this);
    viewDragModeGroup->addAction(ui.actionDrag_Mode);
    viewDragModeGroup->addAction(ui.actionSelect_Mode);
    viewDragModeGroup->setExclusive(true);

    ui.graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    ui.actionDrag_Mode->setChecked(true);
}

void MapWidget::clearMarkers()
{
    qDeleteAll(m_markers);
    m_markers.clear();
    m_position_count.clear();
}

void MapWidget::initCtyMarkersBatch()
{
    auto ctydb = CtyDB::instance();

    while (ctydb->mutex.try_lock_shared()) {
        if (m_db_hint != ctydb->getDBHint()) {
            add_markers_begin = 0;
            m_db_hint = ctydb->getDBHint();
        }
        auto & entdb = ctydb->getVEnts();
        auto add_markers_end = std::min(add_markers_begin + 50, entdb.size());
        if (add_markers_begin >= add_markers_end) {
            m_markers_ready = true;
            emit initCtyMarkersEnd();
            return;
        }

        if (add_markers_begin == 0) {
            m_markers.resize(entdb.size());
            m_markers.assign(entdb.size(), nullptr);
            m_position_count.resize(entdb.size());
            m_position_count.assign(entdb.size(), 0);
        }

        for (auto i = add_markers_begin; i < add_markers_end; ++i) {
            auto & ent = entdb[i];
            auto maker = ui.graphicsView->createMarker(ent->name, ent->lon, ent->lat);
            Q_ASSERT(i == ent->location_id);
            m_markers[ent->location_id] = maker;
            maker->setVisible(false);
            ui.graphicsView->addItem(maker);
        }
        add_markers_begin = add_markers_end;
        ctydb->mutex.unlock_shared();
        break;
    }

    emit initCtyMarkersCon(); // connect to initCtyMarkersBatch (Qt::QueuedConnection)
}

void MapWidget::initCtyMarkers() // connect(ctydb, &CtyDB::dbHintChanged, mapWidget, &MapWidget::initCtyMarkers);
{
    emit initCtyMarkersBegin();
    m_markers_ready = false;
    clearMarkers();
    add_markers_begin = 0;
    initCtyMarkersBatch();
}

void MapWidget::dataVisualize()
{
    if (!m_markers_ready) {
        emit dataVisualizeRe();
        return;
    }

    m_position_count.assign(m_position_count.size(), 0);

    auto ctydb = CtyDB::instance();
    std::shared_lock<decltype(ctydb->mutex)> lock0(ctydb->mutex, std::defer_lock);
    if (!lock0.try_lock()) {
        emit dataVisualizeRe();
        return;
    }
    std::shared_lock<decltype(m_model->mutex)> lock1(m_model->mutex, std::defer_lock);
    if (!lock1.try_lock()) {
        emit dataVisualizeRe();
        return;
    }

    Q_ASSERT(ctydb->getVEnts().size() == m_position_count.size());
    QString buf;
    for (auto& record : m_model->records) {
        auto & call = record["call"];
        buf = QString::fromUtf8(call.data(), call.size());
        ctydb->normalizeCallSign(buf);
        auto result = ctydb->lookUpCallSign(QStringView(buf));
        if (result.first->vaild) {
            ++m_position_count[result.first->location_id];
        }
    }

    for (int i = 0; i < m_markers.size(); ++i) {
        if (auto* marker = m_markers[i]) {
            int count = m_position_count[i];
            marker->setVisible(count > 0);
            if (count > 0) marker->setPointOpacity(std::min(0.2 + count * 0.05, 1.0));
        }
    }
}
