#include "MapGraphicsView.h"
#include <QGraphicsEllipseItem>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMessageBox>
#include <QBrush>
#include <QFont>
#include <QOpenGLWidget>
#include <QGraphicsSceneEvent>

MarkerPointItem::MarkerPointItem(qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent) : QGraphicsEllipseItem(x, y, w, h, parent)
{
    setAcceptHoverEvents(true);
    setBrush(QColor(255, 0, 0, 180));
    setZValue(3.0);
}

void MarkerPointItem::setLabel(QGraphicsItem *label)
{
    m_label = label;
}

void MarkerPointItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsEllipseItem::hoverEnterEvent(event);
}

void MarkerPointItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsEllipseItem::hoverLeaveEvent(event);
}

MarkerItem::MarkerItem(const QString& text, qreal size, QGraphicsItem *parent) : QGraphicsItemGroup(parent)
{
    point = new MarkerPointItem(-size / 2, -size / 2, size, size);
    addToGroup(point);

    label = new QGraphicsSimpleTextItem(text);

    QRectF tr = label->boundingRect();
    label->setPos(-tr.width() / 2, -size / 2 - tr.height());
    label->setZValue(4.0);
    label->setVisible(false);
    label->setAcceptedMouseButtons(Qt::NoButton);
    addToGroup(label);

    point->setLabel(label);

    setFlags(QGraphicsItem::ItemIgnoresTransformations);
    //setHandlesChildEvents(false);
    setAcceptHoverEvents(true);
}

void MarkerItem::setPointOpacity(qreal opacity)
{
    point->setOpacity(opacity);
}

void MarkerItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsItemGroup::hoverEnterEvent(event);
}

void MarkerItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    label->setVisible(false);
    QGraphicsItemGroup::hoverLeaveEvent(event);
}

bool MarkerItem::sceneEvent(QEvent *event)
{
    if (event->type() == QEvent::GraphicsSceneHoverMove) {
        auto *mouseEvent = static_cast<QGraphicsSceneHoverEvent*>(event);
        bool posInPoint = point->contains(point->mapFromScene(mouseEvent->scenePos()));
        if (label->isVisible() != posInPoint) {
            label->setVisible(posInPoint);
        }
    }
    return QGraphicsItemGroup::sceneEvent(event);
}

MapGraphicsView::MapGraphicsView(QWidget *parent) : QGraphicsView(parent), m_scene(new QGraphicsScene(this))
{
    auto worldSvg = new QGraphicsSvgItem(QStringLiteral(":/assets/world.svg"));
    worldSvg->setPos(0, 0);
    m_scene->addItem(worldSvg);
    m_scene->setSceneRect(worldSvg->boundingRect());
    m_scene_bottom_right = worldSvg->boundingRect().bottomRight();

    setScene(m_scene);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setMouseTracking(true);

    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);

    QOpenGLWidget *gl = new QOpenGLWidget();
    QSurfaceFormat format;
    format.setSamples(4);
    gl->setFormat(format);
    setViewport(gl);
}

QPointF MapGraphicsView::mapLonAndLat(const qreal &lon, const qreal &lat) const
{
    qreal x = (180.0 + lon) * (maxX() / 360.0);
    qreal y = (90.0 - lat) * (maxY() / 180.0);
    return QPointF(x, y);
}

MarkerItem *MapGraphicsView::createMarker(const QString &title, const qreal &lon, const qreal &lat, const qreal &size) const
{
    auto marker_center = mapLonAndLat(lon, lat);
    auto sx = marker_center.x(), sy = marker_center.y();
    auto marker = new MarkerItem(title, size);
    marker->setPos(sx, sy);
    marker->setZValue(2.0); 
    return marker;
}

QGraphicsLineItem *MapGraphicsView::createLine(const qreal &lon1, const qreal &lat1, const qreal &lon2, const qreal &lat2, const QPen &pen) const
{
    QPointF p1 = mapLonAndLat(lon1, lat1);
    QPointF p2 = mapLonAndLat(lon2, lat2);

    auto lineItem = new QGraphicsLineItem(QLineF(p1, p2));
    
    auto m_pen = pen;
    m_pen.setWidthF(pen.widthF() / m_zoomnum);
    lineItem->setPen(m_pen);

    lineItem->setZValue(1.5);

    connect(this, &MapGraphicsView::zoomChanged, [=](qreal from, qreal to) {
        auto p = lineItem->pen();
        p.setWidthF(pen.widthF() * from / to);
        lineItem->setPen(p);
    });

    return lineItem;
}

void MapGraphicsView::addItem(QGraphicsItem *item)
{
    m_scene->addItem(item);
}

void MapGraphicsView::removeItem(QGraphicsItem *item)
{
    m_scene->removeItem(item);
}

void MapGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
    
    QPointF scenePos = mapToScene(event->pos());
    
    qreal w = m_scene_bottom_right.x();
    qreal h = m_scene_bottom_right.y();

    if (w <= 0 || h <= 0) return;

    qreal percentX = qBound(0.0, scenePos.x() / maxX(), 1.0);
    qreal percentY = qBound(0.0, scenePos.y() / maxY(), 1.0);

    double lon = percentX * 360.0 - 180.0;
    double lat = 90.0 - percentY * 180.0;

    //qDebug() << itemAt(event->pos());

    emit mouseMoveTo(lon, lat);
}

void MapGraphicsView::wheelEvent(QWheelEvent *event)
{
    event->accept();

    qreal pre_zoom = m_zoomnum;

    QPoint angleDelta = event->angleDelta();
    int deltaY = angleDelta.y();

    qreal scaleFactor = 1.15; // 
    if (deltaY > 0) {
        m_zoomnum *= scaleFactor; // 
    } else {
        m_zoomnum /= scaleFactor; // 
    }
    // 
    m_zoomnum = qBound(0.1, m_zoomnum, 20.0);

    emit zoomChanged(pre_zoom, m_zoomnum);

    // 
    setTransform(QTransform::fromScale(m_zoomnum, m_zoomnum));
}

void MapGraphicsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QGraphicsView::mouseDoubleClickEvent(event);
}
