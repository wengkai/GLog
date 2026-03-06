#ifndef MAPGRAPHICSVIEW_H
#define MAPGRAPHICSVIEW_H

#include "record.h"
#include <QSvgRenderer>
#include <QGraphicsSvgItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPen>
#include <QList>
#include <QHash>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>

class MapGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    MapGraphicsView(QWidget *parent = nullptr);

    static void zoomEllipseItem(QGraphicsEllipseItem* ellipse, qreal from, qreal to);
    static void zoomLineItem(QGraphicsLineItem* line, qreal from, qreal to);
    static void zoomTextItem(QGraphicsSimpleTextItem* label, QGraphicsEllipseItem* ellipse, qreal from, qreal to);
    static void mapRecordToPointF(const GRecord& record, qreal& x, qreal& y);

    bool addMarker(const qreal& x, const qreal& y, const qreal& size = 20.0); // 0.0 <= x,y <= 1.0
    void addLine(const qreal& x1, const qreal& y1, const qreal& x2, const qreal& y2, const QPen &pen = QPen()); // 0.0 <= x,y <= 1.0
    qreal maxX() const { return m_scene_bottom_right.x(); }
    qreal maxY() const { return m_scene_bottom_right.y(); }

    void testMarker();

signals:
    void zoomChanged(qreal from, qreal to);
    void mouseMoveTo(qreal x, qreal y);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QGraphicsScene *m_scene;
    qreal m_zoomnum = 1.0;
    QPointF m_scene_bottom_right;
    QList<QGraphicsEllipseItem*> m_markers;
    QHash<QGraphicsEllipseItem*, QGraphicsSimpleTextItem*> m_marker_labels;

};

#endif