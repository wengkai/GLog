#ifndef MAPGRAPHICSVIEW_H
#define MAPGRAPHICSVIEW_H

#include <QSvgRenderer>
#include <QGraphicsSvgItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPen>
#include <QList>
#include <QHash>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QMainWindow>
#include <QThread>

class MarkerPointItem : public QGraphicsEllipseItem
{
public:
    MarkerPointItem(qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent = nullptr);
    void setLabel(QGraphicsItem * label);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    QGraphicsItem * m_label = nullptr;

};

class MarkerItem : public QGraphicsItemGroup {
public:
    MarkerItem(const QString& text, qreal size, QGraphicsItem *parent = nullptr);
    void setPointOpacity(qreal opacity);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    bool sceneEvent(QEvent *event) override;

private:
    MarkerPointItem * point;
    QGraphicsSimpleTextItem * label;
};

class MapGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    MapGraphicsView(QWidget *parent = nullptr);

    QPointF mapLonAndLat(const qreal& lon, const qreal& lat) const;

    qreal maxX() const { return m_scene_bottom_right.x(); }
    qreal maxY() const { return m_scene_bottom_right.y(); }

    MarkerItem * createMarker(const QString& title, const qreal& lon, const qreal& lat, const qreal& size = 10.0) const;
    QGraphicsLineItem * createLine(const qreal& lon1, const qreal& lat1, const qreal& lon2, const qreal& lat2, const QPen &pen = QPen()) const;

    void addItem(QGraphicsItem * item);
    void removeItem(QGraphicsItem * item);
    

signals:
    void zoomChanged(qreal from, qreal to);
    void mouseMoveTo(qreal lon, qreal lat);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QGraphicsScene *m_scene;
    qreal m_zoomnum = 1.0;
    QPointF m_scene_bottom_right;

};

#endif