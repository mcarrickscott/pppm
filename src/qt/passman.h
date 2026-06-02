#ifndef PASSMAN_H
#define PASSMAN_H

#include <QDialog>

#define HCOUNT 10000
#define PINLEN 4

QT_BEGIN_NAMESPACE
namespace Ui { class PassMan; }
QT_END_NAMESPACE

class PassMan : public QDialog
{
    Q_OBJECT

public:
    PassMan(QWidget *parent = nullptr);
    ~PassMan();

private:
    int pin,services;
    char digest[64],ph[64];
    Ui::PassMan *ui;
public:
    void initialise();
    void startup();
public slots:
    void pw_entered();
    void rand_entered();
    void pin_entered(QString);
    void service_chosen(int);
    void checkchange(int);
    void clean();
    void create();
    void add();
    void remove();
    void sure();
    void reset();
};
#endif // PASSMAN_H
