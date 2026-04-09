#ifndef PROCESSNODE_H
#define PROCESSNODE_H

#include "basenode.h"

class ProcessNode : public BaseNode
{
    Q_OBJECT
public:
    explicit ProcessNode(QGraphicsItem *parent = nullptr);
    // 重写父类的 paint 方法，编写具体的绘制逻辑
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // PROCESSNODE_H
