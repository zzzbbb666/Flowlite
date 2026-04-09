#include "commands.h"


AddNodeCommand::AddNodeCommand(BaseNode *node, QGraphicsScene *scene)
    : myNode(node), myScene(scene) {
    setText("添加节点");
}

AddNodeCommand::~AddNodeCommand() {
    // 如果这个历史记录被清除了，且节点确实不在画布上，那就可以安全释放内存了
    if (!myNode->scene()) delete myNode;
}

void AddNodeCommand::undo() {
    myScene->removeItem(myNode); // 撤销：摘下
}

void AddNodeCommand::redo() {
    myScene->addItem(myNode);    // 重做（或首次执行）：贴上
}

DeleteCommand::DeleteCommand(QList<QGraphicsItem*> items, QGraphicsScene *scene)
    : myScene(scene) {
    setText("删除图元");
    // 核心算法：找出被选中的节点，以及挂在它们身上的连线，一起打包！
    for (QGraphicsItem *item : items) {
        if (!itemsToDelete.contains(item)) itemsToDelete.append(item);

        // 如果删的是节点，把它的线也一并揪出来
        if (BaseNode *node = dynamic_cast<BaseNode*>(item)) {
            for (Connection *conn : node->connections) {
                if (!itemsToDelete.contains(conn)) itemsToDelete.append(conn);
            }
        }
    }
}

DeleteCommand::~DeleteCommand() {
    // 历史记录被清空时，真正清理那些被删掉的对象的内存
    for (QGraphicsItem *item : itemsToDelete) {
        if (!item->scene()) delete item;
    }
}

void DeleteCommand::undo() {
    // 撤销删除：把刚刚摘下来的节点和线，原封不动地还给画布！
    for (QGraphicsItem *item : itemsToDelete) {
        myScene->addItem(item);
    }
}

void DeleteCommand::redo() {
    // 执行删除：不要 delete！只要把它们移出视线即可
    for (QGraphicsItem *item : itemsToDelete) {
        myScene->removeItem(item);
    }
}

AddConnectionCommand::AddConnectionCommand(Connection *conn, QGraphicsScene *scene)
    : myConn(conn), myScene(scene) {
    setText("添加连线");
}

AddConnectionCommand::~AddConnectionCommand() {
    // 垃圾回收：如果历史记录被清空且线不在画布上，彻底销毁
    if (!myConn->scene()) delete myConn;
}

void AddConnectionCommand::undo() {
    // 撤销连线：从画布拿掉，并通知两头节点断开绑定
    if (myConn->sourceNode) myConn->sourceNode->removeConnection(myConn);
    if (myConn->targetNode) myConn->targetNode->removeConnection(myConn);
    myScene->removeItem(myConn);
}

void AddConnectionCommand::redo() {
    // 重做连线：重新通知两头节点绑定，并贴回画布
    if (myConn->sourceNode && !myConn->sourceNode->connections.contains(myConn))
        myConn->sourceNode->addConnection(myConn);
    if (myConn->targetNode && !myConn->targetNode->connections.contains(myConn))
        myConn->targetNode->addConnection(myConn);

    if (!myConn->scene()) myScene->addItem(myConn);
}
