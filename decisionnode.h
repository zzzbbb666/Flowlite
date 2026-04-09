#ifndef DECISIONNODE_H
#define DECISIONNODE_H

#include "basenode.h"

class DecisionNode : public BaseNode
{
    Q_OBJECT
public:
    explicit DecisionNode(QGraphicsItem *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // DECISIONNODE_H
