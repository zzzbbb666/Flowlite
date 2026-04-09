#include "basenode.h"
#include "connection.h"
#include <QGraphicsScene>
#include <QUuid>
#include "commands.h"
#include <QUndoStack>

QUndoStack* BaseNode::undoStack = nullptr; //初始化为空

BaseNode::BaseNode(QGraphicsItem *parent) : QGraphicsObject(parent)
{
    //自动生成不重复的ID
    nodeId = QUuid::createUuid().toString();
    // 初始化默认属性
    width = 100;
    height = 60;
    nodeColor = Qt::white;
    nodeText = "Base";
    //直接开启图元的可选中、可移动标志位
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

QRectF BaseNode::boundingRect() const
{
    // 包围盒固定比真实图形宽高大 20 像素，留足空间给锚点和粗边框
    return QRectF(-width/2 - 10, -height/2 - 10, width + 20, height + 20);
}


void BaseNode::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
}

// 获取锚点的场景坐标
QPointF BaseNode::getAnchorScenePos(AnchorPosition pos) const
{
    QRectF r(-width/2, -height/2, width, height);
    QPointF localPos;
    switch (pos) {
        case Top:    localPos = QPointF(0, r.top()); break;
        case Bottom: localPos = QPointF(0, r.bottom()); break;
        case Left:   localPos = QPointF(r.left(), 0); break;
        case Right:  localPos = QPointF(r.right(), 0); break;
    }
    return mapToScene(localPos);
}

// 绘制锚点小圆圈
void BaseNode::drawAnchors(QPainter *painter)
{
    if (!isSelected()) return;

    painter->setBrush(Qt::blue);
    painter->setPen(Qt::black);
    int radius = 4;
    QRectF r(-width/2, -height/2, width, height);
    painter->drawEllipse(QPointF(0, r.top()), radius, radius);
    painter->drawEllipse(QPointF(0, r.bottom()), radius, radius);
    painter->drawEllipse(QPointF(r.left(), 0), radius, radius);
    painter->drawEllipse(QPointF(r.right(), 0), radius, radius);
}

void BaseNode::addConnection(Connection *conn)
{
    connections.append(conn);
}

void BaseNode::removeConnection(Connection *conn)
{
    connections.removeAll(conn);
}

// 当节点发生任何变化时，Qt 都会调用这个函数
QVariant BaseNode::itemChange(GraphicsItemChange change, const QVariant &value)
{
    // 如果是“位置发生改变”
    if (change == ItemPositionHasChanged) {
        // 遍历挂在这个节点上的所有连线，强制它们重新计算起点和终点
        for (Connection *conn : connections) {
            conn->updatePosition();
        }
    }
    return QGraphicsObject::itemChange(change, value);
}

//碰撞检测：判断鼠标是否点中了某个小圆点
int BaseNode::checkHitAnchor(const QPointF &pos)
{
    QRectF r(-width/2, -height/2, width, height);
    int tolerance = 8; // 增加一点容错范围，不需要点得像素级精准
    // 计算鼠标位置与四个锚点的距离 (QLineF::length 可以方便地算距离)
    if (QLineF(pos, QPointF(0, r.top())).length() <= tolerance) return Top;
    if (QLineF(pos, QPointF(0, r.bottom())).length() <= tolerance) return Bottom;
    if (QLineF(pos, QPointF(r.left(), 0)).length() <= tolerance) return Left;
    if (QLineF(pos, QPointF(r.right(), 0)).length() <= tolerance) return Right;
    return -1; // 没点中任何锚点
}

//鼠标按下：如果点中锚点，开始拉线；如果点中节点本体，保持默认的拖拽移动
void BaseNode::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    //只有当前节点已经被选中（锚点亮着），才允许从它身上拉线
    if (isSelected()) {
        int hitAnchor = checkHitAnchor(event->pos());
        if (hitAnchor != -1) {
            // 开启画线模式
            isDrawingLine = true;
            currentStartAnchor = static_cast<AnchorPosition>(hitAnchor);
            tempLine = new QGraphicsPathItem();
            tempLine->setPen(QPen(Qt::gray, 2, Qt::DashLine));
            tempLine->setZValue(-1.0);
            scene()->addItem(tempLine);
            event->accept();
            return; // 结束处理
        }
    }
    // 如果没被选中，或者点的是节点本体不是锚点，执行正常的选中/拖拽逻辑
    QGraphicsObject::mousePressEvent(event);
}

//鼠标移动：更新临时虚线的终点
void BaseNode::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (isDrawingLine && tempLine) {
        QPointF startP = getAnchorScenePos(currentStartAnchor);
        QPointF endP = event->scenePos();
        QPainterPath path;
        path.moveTo(startP);
        qreal offset = QLineF(startP, endP).length() * 0.4;
        if (offset < 40) offset = 40;
        // 起点同样根据锚点方向推
        QPointF ctrl1 = startP;
        if (currentStartAnchor == Top) ctrl1.setY(ctrl1.y() - offset);
        else if (currentStartAnchor == Bottom) ctrl1.setY(ctrl1.y() + offset);
        else if (currentStartAnchor == Left) ctrl1.setX(ctrl1.x() - offset);
        else if (currentStartAnchor == Right) ctrl1.setX(ctrl1.x() + offset);
        // 临时线因为还没连上目标，终点没有方向，我们就让它默认顺着鼠标当前的移动方向或者直接自然垂下
        QPointF ctrl2 = endP;
        path.cubicTo(ctrl1, ctrl2, endP);
        tempLine->setPath(path);
    } else {
        QGraphicsObject::mouseMoveEvent(event);
    }
}

//鼠标松开：寻找目标，决定是否生成正式连线
void BaseNode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (isDrawingLine) {
        isDrawingLine = false;
        // 临时线完成使命，从场景中移除并销毁
        if (tempLine) {
            scene()->removeItem(tempLine);
            delete tempLine;
            tempLine = nullptr;
        }
        // 核心：利用场景引擎，找出鼠标松开位置下面的图元
        QGraphicsItem *itemUnderMouse = scene()->itemAt(event->scenePos(), QTransform());
        // 尝试将其转换为 BaseNode
        BaseNode *targetNode = dynamic_cast<BaseNode*>(itemUnderMouse);
        // 如果松开的地方确实是一个节点，并且不是自己连自己
        if (targetNode && targetNode != this) {
        // 将鼠标的全局坐标转换成目标节点的局部坐标
            QPointF localPos = targetNode->mapFromScene(event->scenePos());
        // 判断松开的位置是否刚好在目标节点的某个锚点上
            int targetAnchor = targetNode->checkHitAnchor(localPos);
            if (targetAnchor != -1) {
                Connection *realConn = new Connection(this, currentStartAnchor,targetNode, static_cast<AnchorPosition>(targetAnchor));
                if (BaseNode::undoStack) {
                BaseNode::undoStack->push(new AddConnectionCommand(realConn, scene()));
                                         }
                else {
                    scene()->addItem(realConn);
                     }
                                    }
                                               }
            }
    else {
        QGraphicsObject::mouseReleaseEvent(event);
         }
}
