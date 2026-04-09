#include "connection.h"
#include <QPainter>
#include <cmath>

Connection::Connection(BaseNode *source, AnchorPosition sourceAnchor,
                       BaseNode *target, AnchorPosition targetAnchor,
                       QGraphicsItem *parent)
    : QGraphicsPathItem(parent), sourceNode(source), targetNode(target),
      startAnchor(sourceAnchor), endAnchor(targetAnchor)
{
    // Qt 神技：把连线的 Z 层级设为 -1，让它永远压在图形的下面，视觉效果最好！
    setZValue(-1.0);

    // 允许连线被选中（方便以后加删除功能）
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    // 把自己添加到起点和终点的关联列表中
    if (sourceNode) sourceNode->addConnection(this);
    if (targetNode) targetNode->addConnection(this);
    // 初始化时就计算一次曲线路径
    updatePosition();
}

void Connection::updatePosition()
{
    if (!sourceNode || !targetNode) return;

    QPointF startP = sourceNode->getAnchorScenePos(startAnchor);
    QPointF endP = targetNode->getAnchorScenePos(endAnchor);

    QPainterPath path;
    path.moveTo(startP);

    // 核心优化：根据连线的实际距离，动态决定控制点要拉伸多远（让短线不生硬，长线不夸张）
    qreal offset = QLineF(startP, endP).length() * 0.4;
    if (offset < 40) offset = 40; // 设置一个最小拉伸值

    // 计算起点控制点：顺着起点锚点的法线方向往外推
    QPointF ctrl1 = startP;
    if (startAnchor == Top) ctrl1.setY(ctrl1.y() - offset);
    else if (startAnchor == Bottom) ctrl1.setY(ctrl1.y() + offset);
    else if (startAnchor == Left) ctrl1.setX(ctrl1.x() - offset);
    else if (startAnchor == Right) ctrl1.setX(ctrl1.x() + offset);

    // 计算终点控制点：顺着终点锚点的法线方向往外推
    QPointF ctrl2 = endP;
    if (endAnchor == Top) ctrl2.setY(ctrl2.y() - offset);
    else if (endAnchor == Bottom) ctrl2.setY(ctrl2.y() + offset);
    else if (endAnchor == Left) ctrl2.setX(ctrl2.x() - offset);
    else if (endAnchor == Right) ctrl2.setX(ctrl2.x() + offset);

    path.cubicTo(ctrl1, ctrl2, endP);
    setPath(path);
}

void Connection::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // 1. 设置并绘制基础线条
    QPen p(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    if (isSelected()) p.setColor(Qt::blue);
    painter->setPen(p);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path());

    // 2. 如果路径为空就不画箭头
    if (path().isEmpty()) return;

    QPointF endPoint = path().pointAtPercent(1.0);
    qreal angle = path().angleAtPercent(1.0);

    qreal arrowSize = 12.0;
    QPolygonF arrowHead;
    // 核心优化：画一个带凹陷的“燕尾”形状，而不是粗笨的三角形
    arrowHead << QPointF(0, 0)
              << QPointF(-arrowSize, arrowSize / 2.5)
              << QPointF(-arrowSize * 0.7, 0) // 尾部的凹陷点
              << QPointF(-arrowSize, -arrowSize / 2.5);

    painter->save();
    painter->translate(endPoint);
    painter->rotate(-angle);
    painter->setPen(Qt::NoPen);
    painter->setBrush(p.color());

    painter->drawPolygon(arrowHead);
    painter->restore();
}
