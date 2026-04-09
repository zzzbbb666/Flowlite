#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>
#include <QGraphicsScene>
#include "basenode.h"
#include "connection.h"

//添加节点
class AddNodeCommand : public QUndoCommand
{
public:
    AddNodeCommand(BaseNode *node, QGraphicsScene *scene);
    ~AddNodeCommand(); // 析构函数：负责最终的垃圾回收
    void undo() override; // 撤销：把节点摘下来
    void redo() override; // 重做：把节点贴上去

private:
    BaseNode *myNode;
    QGraphicsScene *myScene;
};

//删除图元
class DeleteCommand : public QUndoCommand
{
public:
    DeleteCommand(QList<QGraphicsItem*> items, QGraphicsScene *scene);
    ~DeleteCommand();
    void undo() override; // 撤销删除：重新贴回画布
    void redo() override; // 执行删除：从画布摘下（但不立刻销毁）

private:
    QGraphicsScene *myScene;
    QList<QGraphicsItem*> itemsToDelete;
};

//添加连线
class AddConnectionCommand : public QUndoCommand
{
public:
    AddConnectionCommand(Connection *conn, QGraphicsScene *scene);
    ~AddConnectionCommand();
    void undo() override;
    void redo() override;

private:
    Connection *myConn;
    QGraphicsScene *myScene;
};
#endif // COMMANDS_H
