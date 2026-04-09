#include "processnode.h"

ProcessNode::ProcessNode(QGraphicsItem *parent) : BaseNode(parent)
{
    nodeType = "Process";
    // 设置矩形节点的默认属性
    nodeText = "处理过程";
    nodeColor = QColor(200, 230, 255); // 浅蓝色背景
}

void ProcessNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // 1. 绘制矩形本身
    painter->setBrush(nodeColor);
    // 实现选中状态的视觉反馈 (如绘制蓝色边框)
    if (isSelected()) {
        painter->setPen(QPen(Qt::blue, 2, Qt::DashLine)); // 选中时显示蓝色虚线边框
    } else {
        painter->setPen(QPen(Qt::black, 1)); // 未选中时显示普通黑边
    }
    //只画实际大小，不画包围盒大小
    QRectF r(-width/2, -height/2, width, height);
    painter->drawRect(r);
    // 2. 确保图形居中绘制文本
    painter->setPen(Qt::black);
    painter->drawText(boundingRect(), Qt::AlignCenter, nodeText);
    //在图形的最上层绘制四个锚点
    drawAnchors(painter);
}
