#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QGraphicsScene>
#include "processnode.h"
#include "startnode.h"
#include "decisionnode.h"
#include "connection.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QToolBar>
#include <QMap>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QImage>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMenu>
#include <QKeyEvent>
#include "commands.h"
#include <QWheelEvent>
#include <QColorDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // 设置窗口启动时的大小
    this->resize(1400, 800); // 设定一个宽敞的默认尺寸 (宽1200, 高800)
    QGraphicsScene *scene = new QGraphicsScene(0, 0, 1000, 1000, this);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    // 设置缩放时以鼠标所在位置为中心
    ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    // 拖拽环境初始化
    ui->listWidget->addItem("处理节点");
    ui->listWidget->addItem("开始节点");
    ui->listWidget->addItem("判断节点");
    ui->listWidget->setDragEnabled(true);

    ui->graphicsView->setAcceptDrops(true);
    ui->graphicsView->viewport()->setAcceptDrops(true);
    ui->graphicsView->viewport()->installEventFilter(this);
    // 文件操作
    QToolBar *toolbar = addToolBar("File");
    QAction *saveAction = toolbar->addAction("💾 保存 JSON");
    QAction *loadAction = toolbar->addAction("📂 打开 JSON");
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveToJson);
    connect(loadAction, &QAction::triggered, this, &MainWindow::loadFromJson);

    // 完善属性面板与导出
    QAction *exportAction = toolbar->addAction("🖼️ 导出 PNG");
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportToPng);
    QWidget *panelWidget = new QWidget();
    QVBoxLayout *vbox = new QVBoxLayout(panelWidget);
    propLineEdit = new QLineEdit();
    propColorBtn =
    new QPushButton("🎨 更改节点颜色");
    QPushButton *checkBtn = new QPushButton("🔍 校验孤立节点");
    vbox->addWidget(new QLabel("修改节点文本:"));
    vbox->addWidget(propLineEdit);
    vbox->addSpacing(10); // 留点空隙
    vbox->addWidget(new QLabel("修改节点颜色:"));
    vbox->addWidget(propColorBtn); //把颜色按钮放进布局
    vbox->addSpacing(20);
    vbox->addWidget(checkBtn);
    vbox->addStretch();
    ui->dockWidget_2->setWidget(panelWidget);
    connect(scene, &QGraphicsScene::selectionChanged, this, &MainWindow::onSelectionChanged);
    connect(propLineEdit, &QLineEdit::textChanged, this, &MainWindow::onNodeTextChanged);
    connect(checkBtn, &QPushButton::clicked, this, &MainWindow::checkGraphLogic);
    connect(propColorBtn, &QPushButton::clicked, this, &MainWindow::onNodeColorChanged);
    // 视图菜单，用于找回停靠窗口
    QMenu *viewMenu = menuBar()->addMenu("👁️ 视图");
    ui->dockWidget->setWindowTitle("组件工具栏");
    viewMenu->addAction(ui->dockWidget->toggleViewAction());
    ui->dockWidget_2->setWindowTitle("属性面板");
    viewMenu->addAction(ui->dockWidget_2->toggleViewAction());
    // 开启画布的右键菜单支持
    ui->graphicsView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->graphicsView, &QWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);
    // 初始化撤销/重做栈
    undoStack = new QUndoStack(this);
    BaseNode::undoStack = this->undoStack;
    //Ctrl+Z 和 Ctrl+Y 的绑定
    QAction *undoAction = undoStack->createUndoAction(this, "撤销 (Ctrl+Z)");
    undoAction->setShortcut(QKeySequence::Undo);
    QAction *redoAction = undoStack->createRedoAction(this, "重做 (Ctrl+Y)");
    redoAction->setShortcut(QKeySequence::Redo);
    viewMenu->addSeparator();
    viewMenu->addAction(undoAction);
    viewMenu->addAction(redoAction);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//拦截拖拽事件与滚轮缩放
// 拦截并处理鼠标拖拽、放下以及滚轮缩放的事件
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 如果事件发生在画布上
    if (watched == ui->graphicsView->viewport()) {

        // 滚轮缩放逻辑
        if (event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            // angleDelta().y() > 0 表示滚轮向上滚（放大）
            if (wheelEvent->angleDelta().y() > 0) {
                ui->graphicsView->scale(1.1, 1.1);
            } else { // 向下滚（缩小）
                ui->graphicsView->scale(1.0 / 1.1, 1.0 / 1.1);
            }
            return true; // 截断事件，防止触发默认的滚动条滑动
        }
        // 拖拽进入画布时，同意接收
        else if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
            QDragMoveEvent *dragEvent = static_cast<QDragMoveEvent*>(event);
            dragEvent->acceptProposedAction();
            return true;
        }
        // 关键动作：在画布上松开鼠标（Drop）
        else if (event->type() == QEvent::Drop) {
            QDropEvent *dropEvent = static_cast<QDropEvent*>(event);
            // 获取左侧列表中当前被选中的是哪个文字
            QString itemText = ui->listWidget->currentItem()->text();
            // 把鼠标在屏幕上的物理坐标，转换成画布里的逻辑坐标
            QPointF scenePos = ui->graphicsView->mapToScene(dropEvent->pos());
            // 用命令栈来添加节点
                        if (itemText == "处理节点") {
                            ProcessNode *node = new ProcessNode();
                            node->setPos(scenePos);
                            undoStack->push(new AddNodeCommand(node, ui->graphicsView->scene()));
                        }
                        else if (itemText == "开始节点") {
                            StartNode *node = new StartNode();
                            node->setPos(scenePos);
                            undoStack->push(new AddNodeCommand(node, ui->graphicsView->scene()));
                        }
                        else if (itemText == "判断节点") {
                            DecisionNode *node = new DecisionNode();
                            node->setPos(scenePos);
                            undoStack->push(new AddNodeCommand(node, ui->graphicsView->scene()));
                        }

            dropEvent->acceptProposedAction();
            return true;
        }
    }
    // 其他不关心的事件，还给父类默认处理
    return QMainWindow::eventFilter(watched, event);
}
// 保存为 JSON
void MainWindow::saveToJson()
{
    QJsonArray nodesArray;
    QJsonArray edgesArray;
    // 遍历画布上的所有图元
    for (QGraphicsItem *item : ui->graphicsView->scene()->items()) {
        // 如果是节点 (利用 C++ 的动态类型转换)
        if (BaseNode *node = dynamic_cast<BaseNode*>(item)) {
            QJsonObject nodeObj;
            nodeObj["id"] = node->nodeId;
            nodeObj["type"] = node->nodeType;
            nodeObj["text"] = node->nodeText;
            nodeObj["x"] = node->pos().x();
            nodeObj["y"] = node->pos().y();
            nodesArray.append(nodeObj);
        }
        // 如果是连线
        else if (Connection *conn = dynamic_cast<Connection*>(item)) {
            QJsonObject edgeObj;
            edgeObj["sourceId"] = conn->sourceNode->nodeId;
            edgeObj["sourceAnchor"] = conn->startAnchor;
            edgeObj["targetId"] = conn->targetNode->nodeId;
            edgeObj["targetAnchor"] = conn->endAnchor;
            edgesArray.append(edgeObj);
        }
    }

    QJsonObject mainObj;
    mainObj["nodes"] = nodesArray;
    mainObj["edges"] = edgesArray;
    // 弹出系统保存文件对话框
    QString fileName = QFileDialog::getSaveFileName(this, "保存流程图", "", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(mainObj).toJson());
        file.close();
        QMessageBox::information(this, "成功", "文件保存成功！");
    }
}

// 读取 JSON
void MainWindow::loadFromJson()
{
    QString fileName = QFileDialog::getOpenFileName(this, "打开流程图", "", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray data = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject mainObj = doc.object();
    // 清空当前画布
    ui->graphicsView->scene()->clear();
    undoStack->clear(); // 清空历史记录栈，重新开始
    // 准备一个字典，用来在恢复连线时，通过 ID 快速找到刚刚复活的节点
    QMap<QString, BaseNode*> nodeMap;
    //先复活所有的节点
    QJsonArray nodesArray = mainObj["nodes"].toArray();
    for (int i = 0; i < nodesArray.size(); ++i) {
        QJsonObject nObj = nodesArray[i].toObject();
        QString type = nObj["type"].toString();
        // 简单工厂模式：根据字符串类型创建对应的对象
        BaseNode *newNode = nullptr;
        if (type == "Process") newNode = new ProcessNode();
        else if (type == "Start") newNode = new StartNode();
        else if (type == "Decision") newNode = new DecisionNode();
        if (newNode) {
            newNode->nodeId = nObj["id"].toString(); // 继承原来的ID
            newNode->nodeText = nObj["text"].toString();
            newNode->setPos(nObj["x"].toDouble(), nObj["y"].toDouble());
            ui->graphicsView->scene()->addItem(newNode);
            // 记入字典备用
            nodeMap[newNode->nodeId] = newNode;
        }
    }
    // 2. 再复活所有的连线
    QJsonArray edgesArray = mainObj["edges"].toArray();
    for (int i = 0;i < edgesArray.size(); ++i) {
        QJsonObject eObj = edgesArray[i].toObject();
        QString sourceId = eObj["sourceId"].toString();
        QString targetId = eObj["targetId"].toString();
        AnchorPosition sAnchor = static_cast<AnchorPosition>(eObj["sourceAnchor"].toInt());
        AnchorPosition tAnchor = static_cast<AnchorPosition>(eObj["targetAnchor"].toInt());
        // 如果连线的两端节点都能在字典里找到
        if (nodeMap.contains(sourceId) && nodeMap.contains(targetId)) {
            Connection *conn = new Connection(nodeMap[sourceId], sAnchor, nodeMap[targetId], tAnchor);
            ui->graphicsView->scene()->addItem(conn);
        }
    }
}

//点击图元，属性回显
void MainWindow::onSelectionChanged()
{
    // 获取场景中当前被选中的所有物品
    QList<QGraphicsItem*> items = ui->graphicsView->scene()->selectedItems();

    if (items.size() == 1) {
        // 如果刚好选中了一个，并且它是 BaseNode 节点
        if (BaseNode *node = dynamic_cast<BaseNode*>(items.first())) {
            currentNode = node;
            // 暂时阻断输入框的信号，防止 setText 时触发死循环
            propLineEdit->blockSignals(true);
            propLineEdit->setText(node->nodeText);
            propLineEdit->blockSignals(false);
            QString style = QString("background-color: %1; border: 1px solid gray; border-radius: 4px; padding: 5px;")
                                            .arg(node->nodeColor.name());
            propColorBtn->setStyleSheet(style);
            return;
        }
    }
    // 如果啥都没选中，或者选了一根线，清空面板
    currentNode = nullptr;
    propLineEdit->clear();
    propColorBtn->setStyleSheet("");
}

// 修改输入框，图元实时刷新
void MainWindow::onNodeTextChanged(const QString &text)
{
    if (currentNode) {
        currentNode->nodeText = text;
        currentNode->update(); // 通知 Qt 引擎去调用 paint 重新绘制这段新文字
    }
}

// 图论算法检测孤立节点
void MainWindow::checkGraphLogic()
{
    QStringList isolatedNodes;
    // 遍历画布上的每一个图元
    for (QGraphicsItem *item : ui->graphicsView->scene()->items()) {
        if (BaseNode *node = dynamic_cast<BaseNode*>(item)) {
            // 题目要求：不是开始/结束节点才去检测
            if (node->nodeType != "Start") {
                int inDegree = 0;
                int outDegree = 0;
                // 统计入度和出度
                for (Connection *conn : node->connections) {
                    if (conn->sourceNode == node) outDegree++;
                    if (conn->targetNode == node) inDegree++;
                }
                // 既没有进来也没有出去，就是孤立的
                if (inDegree == 0 && outDegree == 0) {
                    isolatedNodes.append(node->nodeText);
                }
            }
        }
    }

    if (isolatedNodes.isEmpty()) {
        QMessageBox::information(this, "校验通过", "✅ 完美！当前流程图中没有任何孤立节点。");
    } else {
        QMessageBox::warning(this, "警告", "❌ 发现以下孤立节点，请检查连线：\n\n" + isolatedNodes.join("\n"));
    }
}

// 导出画布为 PNG
void MainWindow::exportToPng()
{
    QGraphicsScene *scene = ui->graphicsView->scene();
    // 导出前先清空选择状态，把那些蓝色的虚线框和小圆点锚点藏起来
    scene->clearSelection();
    // 获取囊括了所有图形的最小包围矩形
    QRectF rect = scene->itemsBoundingRect();
    if (rect.isEmpty()) {
        QMessageBox::warning(this, "空画布", "画布上啥也没有，导出失败！");
        return;
    }
    // 创建一张透明背景的高清图片
    QImage image(rect.size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    // 把 scene 里的画面“印”到这张图片上
    scene->render(&painter, QRectF(), rect);
    // 呼出保存文件对话框
    QString fileName = QFileDialog::getSaveFileName(this, "导出高清 PNG", "flowchart.png", "PNG Images (*.png)");
    if (!fileName.isEmpty()) {
        image.save(fileName);
        QMessageBox::information(this, "大功告成", "图片已成功导出到本地！");
    }
}
// 弹出右键菜单的逻辑
void MainWindow::showContextMenu(const QPoint &pos)
{
    // 如果当前画布上什么都没选中，就不弹出菜单
    if (ui->graphicsView->scene()->selectedItems().isEmpty()) {
        return;
    }
    // 创建一个菜单对象
    QMenu contextMenu(this);
    // 添加一个删除动作，并加个小图标 emoji
    QAction *deleteAction = contextMenu.addAction("🗑️ 删除选中项");
    // 将点击这个动作的信号，连接到我们的删除函数上
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteSelectedItems);
    // 在鼠标当前所在的屏幕全局坐标位置弹出菜单
    contextMenu.exec(ui->graphicsView->mapToGlobal(pos));
}

// 安全删除逻辑
void MainWindow::deleteSelectedItems()
{
    QList<QGraphicsItem*> selectedItems = ui->graphicsView->scene()->selectedItems();
    if (selectedItems.isEmpty()) return;
    // 直接把要删的东西打包成一个命令，丢进历史栈
    undoStack->push(new DeleteCommand(selectedItems, ui->graphicsView->scene()));
    // 清空右侧属性面板
    currentNode = nullptr;
    propLineEdit->clear();
}

// 绑定键盘的 Delete 键
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // 如果用户按下了 Delete 键或 Backspace 键
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        deleteSelectedItems(); // 直接调用我们写好的核心删除逻辑
    }
    QMainWindow::keyPressEvent(event);
}

// 调出调色板并修改颜色
void MainWindow::onNodeColorChanged()
{
    // 如果当前有选中的节点
    if
 (currentNode) {
        // 弹出 Qt 自带的神级调色板，初始颜色设为节点当前的颜色
        QColor newColor = QColorDialog::getColor(currentNode->nodeColor,
this, "选择节点颜色"
);

        // 如果用户选了颜色并且点了确定 (没点取消)
        if
 (newColor.isValid()) {
            currentNode->nodeColor = newColor;
// 改变节点颜色数据
            currentNode->update();
// 通知画布重绘该节点

            // 同步更新右侧按钮的背景色
            QString style = QString(
"background-color: %1; border: 1px solid gray; border-radius: 4px; padding: 5px;"
)
                                .arg(newColor.name());
            propColorBtn->setStyleSheet(style);
        }
    }
}
