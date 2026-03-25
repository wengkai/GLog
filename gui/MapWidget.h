#ifndef MAPWIDGET
#define MAPWIDGET

#include "app_export.h"
#include "record.h"
#include "MapGraphicsView.h"
#include "adifdb.h"
namespace Ui{ class MapWidgetClass; };

class APP_EXPORT MapWidget : public QMainWindow 
{
    Q_OBJECT

public:
    MapWidget(AdifModel * model, QWidget * parent = nullptr);
    ~MapWidget();
    MapGraphicsView * getMapGraphicsView();
    void clearMarkers();

public slots:
    void initCtyMarkers();
    void initCtyMarkersBatch();

    void dataVisualize();

signals:
    void initCtyMarkersBegin();
    void initCtyMarkersCon();
    void initCtyMarkersEnd();
    void dataVisualizeRe();

private:
    Ui::MapWidgetClass * ui;
    QString title;
    QString m_db_hint;
    size_t add_markers_begin = 0;
    QList<MarkerItem *> m_markers;
    QList<int> m_position_count;
    std::atomic<bool> m_markers_ready = false;
    AdifModel * m_model;

};

#endif