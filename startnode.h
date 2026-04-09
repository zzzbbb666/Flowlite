#ifndef STARTNODE_H
#define STARTNODE_H

#include "basenode.h"

class StartNode : public BaseNode
{
    Q_OBJECT
public:
    explicit StartNode(QGraphicsItem *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // STARTNODE_H
