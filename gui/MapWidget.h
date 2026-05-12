#ifndef MAPWIDGET
#define MAPWIDGET

#include <vector>
#include "MapGraphicsView.h"
#include "adifdb.h"
#include "app_export.h"
#include "record.h"

class QGraphicsPathItem;

namespace Ui {
class MapWidgetClass;
};

class GLOGKIT_API MapWidget : public QMainWindow {
    Q_OBJECT

  public:
    MapWidget(AdifModel *model, QWidget *parent = nullptr);
    ~MapWidget();
    MapGraphicsView *getMapGraphicsView();
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
    void clearRouteLines();

    Ui::MapWidgetClass *ui;
    QString title;
    QString m_db_hint;
    size_t add_markers_begin = 0;
    std::vector<MarkerItem *> m_markers;
    std::vector<QGraphicsPathItem *> m_route_lines;
    std::vector<int> m_position_count;
    std::atomic<bool> m_markers_ready = false;
    AdifModel *m_model;
};

#endif