#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();




    bool LoadFromObjFile(QString objFile, QString mtlFile);

};
#endif // MAINWINDOW_H
