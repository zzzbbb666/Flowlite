#ifndef CONNECTION_H
#define CONNECTION_H

#include <QGraphicsPathItem>
#include "basenode.h"

class Connection : public QGraphicsPathItem
{
public:
    // 构造函数：需要知道从哪个节点的哪个锚点，连到哪个节点的哪个锚点
    Connection(BaseNode *source, AnchorPosition sourceAnchor,
               BaseNode *target, AnchorPosition targetAnchor,
               QGraphicsItem *parent = nullptr);
    BaseNode *sourceNode;
    BaseNode *targetNode;
    AnchorPosition startAnchor;
    AnchorPosition endAnchor;
    // 核心计算方法：根据起点和终点，重新计算并生成贝塞尔曲线
    void updatePosition();
    // 重写绘制方法，用于给线路上色
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // CONNECTION_H
