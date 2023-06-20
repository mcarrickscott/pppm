#ifndef PASSMAN_H
#define PASSMAN_H

#include <QDialog>

#define HCOUNT 10000
#define PASSLEN 9 // default length in random bytes
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
    int pin;
    char digest[64],ph[64];
    Ui::PassMan *ui;
public:
    void initialise();
public slots:
    void pw_entered();
    void pin_entered(QString);
    void service_chosen(int);
    void checkchange(int);
    void clear();
    void reset();
};
#endif // PASSMAN_H
