#ifndef BASENODE_H
#define BASENODE_H

#include <QGraphicsObject>
#include <QPainter>
#include <QString>
#include <QGraphicsSceneMouseEvent>
class Connection;
class QUndoStack;

//定义锚点的四个方向
enum AnchorPosition { Top, Bottom, Left, Right };

//定义抽象基类 BaseNode
class BaseNode : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit BaseNode(QGraphicsItem *parent = nullptr);
    static QUndoStack *undoStack;
    QString nodeText;
    QColor nodeColor;
    QString nodeId;   // 节点的唯一身份ID
    QString nodeType; // 节点的类型名称（决定存取时是什么形状）
    int width;
    int height;
    QRectF boundingRect() const override; // 告诉框架图元的边框大小，用于碰撞检测和重绘
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override; // 实际的绘制逻辑
    // 为了让具体的图形去实现各自的形状，我们将 paint 定义为可以在子类中重写的虚函数
    // 获取某个锚点在场景中的绝对坐标（以后连线要用）
    QPointF getAnchorScenePos(AnchorPosition pos) const;
    // 专门用来绘制四个小圆点的方法
    void drawAnchors(QPainter *painter);
    void addConnection(Connection *conn);
    void removeConnection(Connection *conn);
    QList<Connection *> connections;

protected:
    // 专门用来监听图元的位置变化
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
        // 鼠标事件重写
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    // 辅助函数：判断鼠标点中了哪个锚点 (-1表示没点中)
    int checkHitAnchor(const QPointF &pos);
    // 拖拽连线状态变量
    bool isDrawingLine = false;
    AnchorPosition currentStartAnchor;
    QGraphicsPathItem *tempLine = nullptr; // 跟随鼠标的临时虚线

};

#endif // BASENODE_H
