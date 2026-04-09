#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include "basenode.h"
#include <QUndoStack>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private slots:
    void saveToJson();
    void loadFromJson();
    void onSelectionChanged();                  // 监听画布选中变化
    void onNodeTextChanged(const QString &text);// 监听输入框文字变化
    void checkGraphLogic();                     // 校验孤立节点
    void exportToPng();                         // 导出图片
    void showContextMenu(const QPoint &pos); // 弹出右键菜单
    void deleteSelectedItems();              // 核心的删除逻辑
    void onNodeColorChanged();
private:
    Ui::MainWindow *ui;
    BaseNode *currentNode = nullptr; // 当前正在被选中的节点
    QLineEdit *propLineEdit = nullptr; // 右侧面板的输入框
    void keyPressEvent(QKeyEvent *event) override;
    QUndoStack *undoStack;
    QPushButton *propColorBtn = nullptr;
};
#endif // MAINWINDOW_H
