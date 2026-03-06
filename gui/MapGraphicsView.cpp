#include "MapGraphicsView.h"
#include <QGraphicsEllipseItem>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMessageBox>
#include <QBrush>
#include <QFont>

#include <random>

MapGraphicsView::MapGraphicsView(QWidget *parent) : QGraphicsView(parent), m_scene(new QGraphicsScene(this))
{
    auto worldSvg = new QGraphicsSvgItem(QStringLiteral(":/assets/world.svg"));
    m_scene->addItem(worldSvg);
    m_scene_bottom_right = worldSvg->boundingRect().bottomRight();

    setScene(m_scene);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
}

void MapGraphicsView::zoomEllipseItem(QGraphicsEllipseItem *ellipse, qreal from, qreal to)
{
    auto m_rect = ellipse->rect();
    auto center = m_rect.center();
    auto new_width = m_rect.width() * from / to;
    auto new_height = m_rect.height() * from / to;
    ellipse->setRect(center.x() - new_width / 2.0, center.y() - new_height / 2.0, new_width, new_height);
    auto pen = ellipse->pen();
    auto new_pen_width = pen.widthF() * from / to;
    pen.setWidthF(new_pen_width);
    ellipse->setPen(pen);
}

void MapGraphicsView::zoomLineItem(QGraphicsLineItem *line, qreal from, qreal to)
{
    auto pen = line->pen();
    auto new_width = pen.widthF() * from / to;
    pen.setWidthF(new_width);
    line->setPen(pen);
}

void MapGraphicsView::zoomTextItem(QGraphicsSimpleTextItem *label, QGraphicsEllipseItem* ellipse, qreal from, qreal to)
{
    auto center = ellipse->boundingRect().center();
    auto font = label->font();
    auto new_font_size = font.pointSizeF() * from / to;
    font.setPointSizeF(new_font_size);
    label->setFont(font);
    auto new_label_rect = label->boundingRect();
    label->setPos(center.x() - new_label_rect.width() / 2.0, center.y() - new_label_rect.height() / 2.0);
}

void MapGraphicsView::mapRecordToPointF(const GRecord &record, qreal &x, qreal &y)
{
    // TODO: imp
}

bool MapGraphicsView::addMarker(const qreal& x, const qreal& y, const qreal& size)
{
    QPointF topLeft(maxX() * x - size / 2.0 , maxY() * y - size / 2.0);
    auto ellipse = new QGraphicsEllipseItem;
    ellipse->setZValue(qreal(1.0));
    ellipse->setBrush(QBrush(Qt::black));
    ellipse->setRect(topLeft.x(), topLeft.y(), size, size);
    zoomEllipseItem(ellipse, 1.0, m_zoomnum);
    
    decltype(ellipse) colliding_marker = nullptr;
    for(auto marker : m_markers) {
        auto c1 = ellipse->rect().center();
        auto c2 = marker->rect().center();
        auto distance = QLineF(c1, c2).length();
        if ((ellipse->rect().width() / 2 + marker->rect().width() / 2) * m_zoomnum > distance) {
            colliding_marker = marker;
            break;
        }
    }
    if (colliding_marker) {
        delete ellipse;
        auto label = m_marker_labels[colliding_marker];
        int count = label->text().toInt();
        label->setText(QString::number(count + 1));
        zoomTextItem(label, colliding_marker, 1.0, 1.0);
        return false;
    }

    m_markers.append(ellipse);
    m_scene->addItem(ellipse);
    
    auto label = new QGraphicsSimpleTextItem("1");
    label->setZValue(2.0);
    label->setBrush(QBrush(Qt::white));
    auto labelRect = label->boundingRect();
    label->setPos(ellipse->rect().center().x() - labelRect.width() / 2.0, ellipse->rect().center().y() - labelRect.height() / 2.0);
    zoomTextItem(label, ellipse, 1.0, m_zoomnum);
    m_marker_labels[ellipse] = label;
    m_scene->addItem(label);

    connect(this, &MapGraphicsView::zoomChanged, [=](qreal from, qreal to) {
        zoomEllipseItem(ellipse, from, to);
        zoomTextItem(label, ellipse, from, to);
    });

    return true;
}

void MapGraphicsView::addLine(const qreal &x1, const qreal &y1, const qreal &x2, const qreal &y2, const QPen &pen)
{
    auto line = m_scene->addLine(x1 * maxX() , y1 * maxY() , x2 * maxX() , y2 * maxY(), pen);
    zoomLineItem(line, 1.0, m_zoomnum);
    connect(this, &MapGraphicsView::zoomChanged, [=](qreal from, qreal to) {
        zoomLineItem(line, from, to);
    });
}

void MapGraphicsView::testMarker()
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<qreal> dist(0.0, 1.0);
    addMarker(0.5, 0.5);
    // we might not use more than 400 markers
    for (int i = 0; i < 400; ++i) {
        auto x = dist(mt);
        auto y = dist(mt);
        if (!addMarker(x, y)) {
            continue;
        }
        addLine(0.5, 0.5, x, y);
    }
    // after almost all the possible positions are used, add more markers to test counting performance
    for (int i = 0; i < 400; ++i) {
        if (!addMarker(0.9, 0.9)) {
            continue;
        }
        addLine(0.5, 0.5, 0.9, 0.9);
    }
}

void MapGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
    auto scenePos = mapToScene(event->pos());
    auto x = scenePos.x() / maxX();
    auto y = scenePos.y() / maxY();
    emit mouseMoveTo(x, y);
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
    m_zoomnum = qBound(1.0, m_zoomnum, 20.0);

    emit zoomChanged(pre_zoom, m_zoomnum);

    // 
    setTransform(QTransform::fromScale(m_zoomnum, m_zoomnum));
}

void MapGraphicsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QGraphicsView::mouseDoubleClickEvent(event);
}
