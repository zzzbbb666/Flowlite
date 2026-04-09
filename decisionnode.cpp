#include "decisionnode.h"

DecisionNode::DecisionNode(QGraphicsItem *parent) : BaseNode(parent)
{
    nodeType = "Decision";
    nodeText = "条件判断";
    nodeColor = QColor(255, 255, 200); // 浅黄色背景

    // 菱形通常需要稍微宽一点才能容纳下文字，这里可以直接覆盖父类的默认宽高
    width = 120;
    height = 80;
}

void DecisionNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setBrush(nodeColor);

    if (isSelected()) {
        painter->setPen(QPen(Qt::blue, 2, Qt::DashLine));
    } else {
        painter->setPen(QPen(Qt::black, 1));
    }

    QRectF r(-width/2, -height/2, width, height);
    QPolygonF diamond;
    diamond << QPointF(r.center().x(), r.top())
            << QPointF(r.right(), r.center().y())
            << QPointF(r.center().x(), r.bottom())
            << QPointF(r.left(), r.center().y());

    painter->drawPolygon(diamond);
    painter->setPen(Qt::black);
    painter->drawText(r, Qt::AlignCenter, nodeText);
    drawAnchors(painter);
}
