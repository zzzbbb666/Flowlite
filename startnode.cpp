#include "startnode.h"

StartNode::StartNode(QGraphicsItem *parent) : BaseNode(parent)
{
    nodeType = "Start";
    nodeText = "开始/结束";
    nodeColor = QColor(200, 255, 200); // 浅绿色背景
}

void StartNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setBrush(nodeColor);

    if (isSelected()) {
        painter->setPen(QPen(Qt::blue, 2, Qt::DashLine));
    } else {
        painter->setPen(QPen(Qt::black, 1));
    }

    QRectF r(-width/2, -height/2, width, height);
    painter->drawRoundedRect(r, 20, 20);
    painter->setPen(Qt::black);
    painter->drawText(r, Qt::AlignCenter, nodeText);
    drawAnchors(painter);
}
