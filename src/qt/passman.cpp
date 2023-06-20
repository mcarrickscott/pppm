#include "passman.h"
#include "./ui_passman.h"
#include "sha3.h"
#include <QDebug>
#include <QClipboard>
#include <QCheckBox>
#include <cctype>

typedef struct server {
    QString username;
    QString service;
    QString domain;
    QString note;
    int policy_type;
} server;

// User configurable from here *************************************************************

QString randstr="gheotibtuwtgwtogvongi4hggigngtgegepegetetqjgjtegjqpughqeugqphqetuhth";

server list[]={
    {"","None","","",0},
    {"youremail@somewhere.com",         "amazon",              "amazon.co.uk",          "",2},
    {"yourotheremail@somewhereelse.ie", "facebook",            "facebook.com",          "",0},
    {"youremail@somewhere.com",         "fitbit",              "fitbit.com",            "",0},
    {"yourotheremail@somewhereelse.ie", "zoom",                "zoom.us",               "",0},
};

// to here ***********************************************************************************

static int SERVICES=sizeof list / sizeof list[0];

static void HASH_again(char *d)
{
    int i;
    sha3 sh;
    SHA3_init(&sh,SHA3_HASH512);
    for (i=0;i<64;i++) SHA3_process(&sh,d[i]);
    SHA3_hash(&sh,d);
}

/* Convert byte array to base64 string */
static void tobase64(int type,char *b,char *w)
{
    int i,j,k,rem,last;
    int c,ch[4],len=PASSLEN;
    unsigned char ptr[3];
    rem=len%3; j=k=0; last=4;
    while (j<len)
    {
        for (i=0;i<3;i++)
        {
            if (j<len) ptr[i]=w[j++];
            else {ptr[i]=0; last--;}
        }
        ch[0]=(ptr[0]>>2)&0x3f;
        ch[1]=((ptr[0]<<4)|(ptr[1]>>4))&0x3f;
        ch[2]=((ptr[1]<<2)|(ptr[2]>>6))&0x3f;
        ch[3]=ptr[2]&0x3f;
        for (i=0;i<last;i++)
        {
            c=ch[i];
            if (c<26) c+=65;
            if (c>=26 && c<52) c+=71;
            if (c>=52 && c<62) c-=4;
            if (c==62) c='+';
            if (c==63) c='/';
            b[k++]=c;
        }
    }
    if (rem>0) for (i=rem;i<3;i++) b[k++]='=';
    b[k]='\0';
    if (type==1) b[10]='\0';
}

// return 1 if password meets password policy requirement, else 0
// at least one special, one upper case, one lower case, one number
// must start with a letter
// User modification may be needed here for new policies **********************************************
static int policy(int type,char *b)
{
    int i,len,isd,isl,isu,gotone=0;
    if (!isupper(b[0]) && !islower(b[0])) return 0;
    isd=isl=isu=0;
    len=(PASSLEN*4)/3;
    if (type==1) len=10;
    for (i=0;i<len;i++)
    {
        if (b[i]=='/') b[i]='!';
        if (type==2 && b[i]=='+') b[i]='$';
        if (isdigit(b[i])) {isd=1; continue;}
        if (islower(b[i])) {isl=1;  continue;}
        if (isupper(b[i])) {isu=1;  continue;}
        gotone=1; // "special" character
    }
    if (isd && isl && isu && gotone) return 1;
    return 0;
}

PassMan::PassMan(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PassMan)
{
    ui->setupUi(this);
    pin=0;
    for (int i=0;i<64;i++) {digest[i]=0; ph[i]=0;}
}


QFont bigFont("Courier New", 16);

void PassMan::pw_entered()
{
    QString password=ui->master->text();
    ui->master->clear();
    ui->master->setDisabled(1);

    sha3 sh;
    SHA3_init(&sh,SHA3_HASH512);

    QByteArray ba=password.toLatin1();
    for (int i=0;i<ba.length();i++)
        SHA3_process(&sh,ba[i]);
    SHA3_hash(&sh,ph);

    for (int i=0;i<HCOUNT;i++) HASH_again(ph);
    ui->service->setEnabled(1);
}

void PassMan::service_chosen(int n)
{
    int i;
    if (n==0) return;
    //    qDebug() << services[n];
    sha3 sh;
    char b64[50];
    QString chosen=list[n].service;
    ui->url->setText(list[n].domain);
    ui->note->setText(list[n].note);
    // push password onto Clipboard
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(list[n].username);//usernames[n]);

    QByteArray ba=chosen.toLatin1();
    SHA3_init(&sh,SHA3_HASH512);
    for (i=0;i<ba.length();i++)
        SHA3_process(&sh,ba[i]);
    for (i=0;i<64;i++) SHA3_process(&sh,ph[i]);
    SHA3_hash(&sh,digest);
    ui->pin->setEnabled(1); // enable PIN entry
    ui->pin->setFocus();
    ui->pin->clear();
    ui->pword->clear();
    ui->service->setDisabled(1);
    ui->username->setText(list[n].username);//usernames[n]);
    ui->show->setEnabled(1);
}

void PassMan::pin_entered(QString text)
{
    int pt=0;  // policy type
    if (text.length()!=4) return;

    const int ipt=ui->service->currentIndex();
    pt=list[ipt].policy_type;//   policy_type[ipt];
    ui->pin->setDisabled(1);
    ui->service->setEnabled(1);
    pin=text.toInt();

    sha3 sh;
    SHA3_init(&sh,SHA3_HASH512);
    SHA3_process(&sh,pin%100);
    SHA3_process(&sh,(pin/100)%100);
    for (int i=0;i<64;i++) SHA3_process(&sh,digest[i]);
    SHA3_hash(&sh,digest);

    int i;
    char b64[50];
    QByteArray ba=randstr.toLatin1();
    SHA3_init(&sh,SHA3_HASH512);
    for (i=0;i<ba.length();i++)
        SHA3_process(&sh,ba[i]);
    for (i=0;i<64;i++) SHA3_process(&sh,digest[i]);
    SHA3_hash(&sh,digest);

    tobase64(pt,b64,digest);  // Convert to base64

    while (!policy(pt,b64)) {
        HASH_again(digest);
        tobase64(pt,b64,digest);
    }

    if (ui->show->isChecked()) {
        ui->pword->setText(b64);
    }

    // push password onto Clipboard
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(b64);

    ui->service->setCurrentIndex(0);
    ui->url->clear();
    ui->note->clear();
    ui->username->clear();
    ui->service->setEnabled(1);
    ui->pin->clear();
    ui->show->setEnabled(0);
    pin=0;
    for (i=0;i<64;i++) {digest[i]=0;}
    i=0;
    while (b64[i]!=0) {
        b64[i++]=0;
    }
}

void PassMan::checkchange(int state)
{
    if (!state) {
        ui->show->setChecked(false);
        //ui->username->clear();
        ui->pword->clear();
    }
    ui->pin->setFocus();
}

void PassMan::clear()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();
    ui->pword->clear();
    ui->show->setChecked(false);

    ui->service->setCurrentIndex(0);
    ui->url->clear();
    ui->note->clear();
    ui->username->clear();
    ui->service->setEnabled(1);
    ui->pin->clear();
}

void PassMan::reset()
{
    clear();
    for (int i=0;i<64;i++) {digest[i]=0; ph[i]=0;}
    ui->master->setEnabled(1);
    ui->service->setCurrentIndex(0);
    ui->url->clear();
    ui->note->clear();
    ui->username->clear();
    ui->service->setEnabled(0);
    ui->pin->clear();
    ui->show->setEnabled(0);
    ui->master->setFocus();
}

void PassMan::initialise()
{
    ui->pin->setEchoMode(QLineEdit::Password);
    ui->pin->setFont(bigFont);
    ui->pin->setMaxLength(4);
    ui->pin->setStyleSheet("border: 1px solid red");
    for (int i=0;i<SERVICES;i++)
        ui->service->addItem(list[i].service);//usernames[i]);
    connect(ui->master, &QLineEdit::returnPressed, this, &PassMan::pw_entered);
    connect(ui->pin,&QLineEdit::textEdited,this, &PassMan::pin_entered);  // Signal parameter is passed to slot
    connect(ui->service,&QComboBox::currentIndexChanged,this,&PassMan::service_chosen);
    connect(ui->show,&QCheckBox::stateChanged,this,&PassMan::checkchange);
    connect(ui->clear,&QPushButton::clicked,this,&PassMan::clear);
    connect(ui->reset,&QPushButton::clicked,this,&PassMan::reset);
}

PassMan::~PassMan()
{
    for (int i=0;i<64;i++) {digest[i]=0; ph[i]=0;}
    delete ui;
}

