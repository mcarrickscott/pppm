#include "passman.h"
#include "./ui_passman.h"
#include "sha3.h"
#include <QDebug>
#include <QClipboard>
#include <QCheckBox>
#include <cctype>
#include <QFile>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QStandardPaths>

#define MAX 200

typedef struct server {
    QString username;
    QString service;
    QString domain;
    QString note;
    int policy_type;
} server;


static bool entered=false;
static QString randstr="";  // long random secret
//static QString extra="giso562994laaa;;i";  // enter your own randomness here....
static QString email="";

static server list[MAX];

static void bubblesort(server list[], int number)
{
    bool swapped;
    for (int i = 1; i < number; i++) {
        swapped = false;
        for (int j = 1; j < number - i; j++) {
            if (list[j].service.toLower().compare(list[j+1].service.toLower())>0) {
                server t=list[j]; list[j]=list[j+1]; list[j+1]=t;
                swapped=true;
            }
        }
        if (!swapped) break;
    }
}

static int getlist()
{
    QFile file("sites.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    { // create the file and initialise it
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return 0;
        QTextStream stream(&file);
        stream << ",None,,,0\n";
        file.close();
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return 0;  // open it again
    }

    int number=0;
    while (!file.atEnd() && number<MAX) {
        QString line = file.readLine();
//qDebug() << "getlist " << line;
        QStringList tokens = line.split(",");
        list[number].username = tokens.at(0);
        list[number].service = tokens.at(1);
        list[number].domain = tokens.at(2);
        list[number].note = tokens.at(3);
        list[number].policy_type = tokens.at(4).toInt();
        number++;
    }
    bubblesort(list,number);
    return number;
}

static void HASH_again(char *d)
{
    int i;
    sha3 sh;
    SHA3_init(&sh,SHA3_HASH512);
    for (i=0;i<64;i++) SHA3_process(&sh,d[i]);
    SHA3_hash(&sh,d);
}

/* Convert byte array w to base64 string b */
static void tobase64(char *w,char *b)
{
    int i,j,k,rem,last;
    int c,ch[4],len=32;   // sufficient random bytes for any conceivable password
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
}

// return 1 if password does not contain triple repeat
static int notriple(char *b) {
    int i=0;
    while (b[i+2]!='\0') {
        if (b[i]==b[i+1] && b[i]==b[i+2]) return 0;
        i++;
    }
    return 1;
}

// return 1 if password meets password policy requirement, else 0
// at least one special, one upper case, one lower case, one number
// must start with a letter
// must not contain triple repeat like aaa
// User modification may be needed here for new policies **********************************************

// Policy 0 - as above, length 12
// Policy 1 - as above, length 10
// Policy 2 - as above, length 12 except special character + not allowed
// Policy 3 - as above, length 16 except special character + not allowed
// Policy 4 - as above, Convert I to i and l to L

static int policy(int type,char *b)
{
    int i,len,isd,isl,isu,gotone=0;
    if (!isupper(b[0]) && !islower(b[0])) return 0;
    isd=isl=isu=0;
    len=12;
    if (type==1) len=10;
    if (type==3) len=16;
    b[len]='\0';
    for (i=0;i<len;i++)
    {
        if (b[i]=='/') b[i]='!';
        if (type>1 && b[i]=='+') b[i]='$';
        if (type>3 && b[i]=='I') b[i]='i';
        if (type>3 && b[i]=='l') b[i]='L';
        if (isdigit(b[i])) {isd=1; continue;}
        if (islower(b[i])) {isl=1;  continue;}
        if (isupper(b[i])) {isu=1;  continue;}
        gotone=1; // it must be a "special" character
    }
    if (isd && isl && isu && gotone && notriple(b)) return 1;
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

// set label normal or bold
void bold(QLabel *f,bool doit)
{
    QFont font = f->font();
    if (doit) font.setBold(true);
    else font.setBold(false);
    f->setFont(font);
}

// super random secret
void PassMan::rand_entered()
{
    randstr=ui->master->text();//+extra;
    QFile file("rand.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream op(&file);
        op <<  ui->master->text() << "\n";
        op << ui->username->text();
        file.close();
        exit(0);
    }
}

void PassMan::pw_entered()
{
    QString password=ui->master->text();
    //qDebug() << "password entered";

    sha3 sh;
    SHA3_init(&sh,SHA3_HASH512);

    QByteArray ba=password.toLatin1();
    for (int i=0;i<ba.length();i++)
        SHA3_process(&sh,ba[i]);
    SHA3_hash(&sh,ph);

    for (int i=0;i<HCOUNT;i++) HASH_again(ph);

    ui->master->clear();
    bold(ui->secret,false);
    ui->master->setDisabled(1);

    bold(ui->label,true);
 //   qDebug() << "made bold";
    ui->service->setCurrentIndex(0);
    ui->service->setEnabled(1);
    ui->service->setFocus();      // important to shift focus

    ui->create->setDisabled(1);
}

void PassMan::service_chosen(int n)
{
    int i;
    if (n==0) return;

    sha3 sh;
    char b64[50];
    QString chosen=list[n].service;
    ui->url->setText(list[n].domain);
    ui->note->setText(list[n].note);
    // push password onto Clipboard
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(list[n].username);//usernames[n]);

    bold(ui->label,false);

    QByteArray ba=chosen.toLatin1();
    SHA3_init(&sh,SHA3_HASH512);
    for (i=0;i<ba.length();i++)
        SHA3_process(&sh,ba[i]);
    for (i=0;i<64;i++) SHA3_process(&sh,ph[i]);
    SHA3_hash(&sh,digest);
    ui->pin->setEnabled(1); // enable PIN entry
    ui->pin->setFocus();
    ui->pin->clear();
    ui->pin->setToolTip("Recommend same PIN for all sites");
    ui->pword->clear();
    ui->service->setDisabled(1);
    ui->username->setText(list[n].username);//usernames[n]);
    ui->show->setEnabled(1);
    ui->remove->setEnabled(1);

    bold(ui->label_pin,true);
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
    char b64[100];
    QByteArray ba=randstr.toLatin1();
    SHA3_init(&sh,SHA3_HASH512);
    for (i=0;i<ba.length();i++)
        SHA3_process(&sh,ba[i]);
    for (i=0;i<64;i++) SHA3_process(&sh,digest[i]);
    SHA3_hash(&sh,digest);

    tobase64(digest,b64);  // Convert to base64

    while (!policy(pt,b64)) {
        HASH_again(digest);
        tobase64(digest,b64);
    }

    if (ui->show->isChecked()) {
        ui->pword->setText(b64);
    }

    // push password onto Clipboard
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(b64);

    bold(ui->label_pin,false);
    bold(ui->label,true);

    ui->service->setCurrentIndex(0);
    ui->url->clear();
    ui->note->clear();
    ui->username->clear();
    ui->service->setEnabled(1);
    ui->pin->clear();
    ui->show->setEnabled(0);
    ui->remove->setEnabled(0);
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

void PassMan::clean()
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
    ui->remove->setEnabled(0);
    ui->pin->clear();
    ui->pin->setEnabled(0);
    ui->sure->setEnabled(0);
}

void PassMan::create()
{
    ui->newservice->setEnabled(1); ui->newservice->setToolTip("Your unique name to identify service");
    ui->newservice->setFocus();
    ui->url->setEnabled(1); ui->url->setToolTip("Legit URL of service - don't go to phishing site!");
    ui->note->setEnabled(1);
    ui->username->setEnabled(1); ui->username->setToolTip("Enter Username (maybe email?)");
    ui->username->setText(email);
    ui->policy->setEnabled(1);

    ui->policy->setText("4");  ui->policy->setToolTip("Select password policy to conform to");
    ui->add->setEnabled(1); ui->add->setToolTip("Confirm service to be added or deleted");
    ui->master->setEnabled(0);
    ui->pin->setEnabled(0);
    ui->create->setEnabled(0);
    bold(ui->label_user,true);
    bold(ui->label_url,true);
    bold(ui->label_new,true);
    bold(ui->label,false);
    bold(ui->secret,false);
}

void PassMan::add()
{
    if (ui->newservice->text().isEmpty() || ui->username->text().isEmpty()) return;

    bool notnew=false;
    for (int i=0;i<services;i++) {
        if (list[i].service==ui->newservice->text()) {
            notnew=true;
            break;
        }
    }
    if (notnew) return;

    server newone;
    newone.domain=ui->url->text();
    newone.note=ui->note->text();
    newone.service=ui->newservice->text();
    newone.username=ui->username->text();
    newone.policy_type=ui->policy->text().toInt();
    if (newone.policy_type>4 || newone.policy_type<0) {
        ui->policy->clear();
        return;
    }

    ui->newservice->setEnabled(0);
    // add to file
    QFile file("sites.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream op(&file);
        op << newone.username << "," << newone.service << "," << newone.domain << "," << newone.note << "," << ui->policy->text() << "\n";
    }

 // add to list in memory
    list[services]=newone;
 // add to drop-down list
    ui->service->addItem(ui->newservice->text());
    this->services+=1;
    ui->url->setEnabled(0);
    ui->note->setEnabled(0);
    ui->username->setEnabled(0);
    ui->policy->setEnabled(0);
    ui->create->setEnabled(0);
    ui->url->clear();
    ui->note->clear();
    ui->username->clear();
    ui->policy->clear();
    ui->add->setEnabled(0);
    bold(ui->label_user,false);
    bold(ui->label_url,false);
    bold(ui->label_new,false);
    //qDebug() << "Add pressed reset";
    reset();
}

void PassMan::remove()
{
    ui->newservice->setText(ui->service->currentText());
    ui->sure->setEnabled(1);
    ui->remove->setEnabled(0);
    ui->pin->setEnabled(0);
    bold(ui->label_pin,false);
}

void PassMan::sure()
{
    int i,j;
    QFile file("sites.txt");
    QFile newfile("newsites.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    if (!newfile.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream out(&newfile);
// remove server from file
    int number=0;
    while (!file.atEnd() && number<MAX) {
        QString line = file.readLine();
        QStringList tokens = line.split(",");
        if ((QString)tokens.at(1)==ui->newservice->text()) continue;
        out << line;
        number++;
    }
    file.close();
    newfile.close();
    file.remove();
    newfile.rename("sites.txt");
// remove from memory
    for (i=j=1;i<services;i++) {
        if (list[i].service==ui->newservice->text()) continue;
        list[j++]=list[i];
    }

// remove from drop-down list
    for (i=1;i<services;i++) {
        if (ui->service->itemText(i)==ui->newservice->text())
            ui->service->removeItem(i);
    }
    //qDebug() << "Sure pressed reset";
    reset();
    services=number;
}

void PassMan::reset()
{
    clean();
    for (int i=0;i<64;i++) {digest[i]=0; ph[i]=0;}
    //qDebug() << "Somebody pressed reset";
    ui->url->clear();
    ui->note->clear();
    ui->username->clear();
    ui->service->setCurrentIndex(0);
    ui->service->setEnabled(0);
    ui->pin->clear();
    ui->show->setEnabled(0);

    ui->create->setEnabled(1);
    ui->sure->setEnabled(0);
    ui->newservice->clear();
    ui->newservice->setEnabled(0);
    ui->add->setEnabled(0);
    ui->policy->clear();
    ui->policy->setEnabled(0);
    ui->pin->setEnabled(0);
    ui->url->setEnabled(0);
    ui->username->setEnabled(0);
    ui->note->setEnabled(0);
    ui->master->setEnabled(1);
    ui->master->setFocus();

    bold(ui->label,false);
    bold(ui->secret,true);
    bold(ui->label_pin,false);
    bold(ui->label_user,false);
    bold(ui->label_url,false);
    bold(ui->label_new,false);
}

void PassMan::startup()
{
// first set path to writeable storage
    QString path = QStandardPaths::standardLocations( QStandardPaths::AppConfigLocation ).value(0);  // Somewhere local - NOT in the cloud!

    QDir myDir(path);
    if (!myDir.exists()) {
        myDir.mkpath(path);
    }
    QDir::setCurrent(path);
    //ui->note->setText(path);
    //qDebug() << path;
    QFile file("rand.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        randstr = file.readLine().trimmed();
        //qDebug() << randstr;
        email = file.readLine().trimmed();
        ui->secret->setText("Master");
        bold(ui->secret,true);
        ui->master->setFocus();
        ui->master->setToolTip("Enter your Master Secret (and don't write it down anywhere!)");
        entered=true;
        file.close();
    } else {  // application has not been initialised - enter fixed long secret
        entered=false;
        ui->username->setEnabled(1);
        ui->secret->setText("Random");
        bold(ui->secret,true);
        bold(ui->label_user,true);
        ui->master->setToolTip("Enter long random string (and write it down somewhere!). Then restart program");
        connect(ui->master, &QLineEdit::returnPressed, this, &PassMan::rand_entered);
    }
}

void PassMan::initialise()
{
    if (!entered) return;
    this->services=getlist();
    ui->username->setEnabled(0);
    ui->pin->setEchoMode(QLineEdit::Password);
    ui->pin->setFont(bigFont);
    ui->pin->setMaxLength(4);
    ui->pin->setStyleSheet("border: 1px solid red");
    for (int i=0;i<services;i++)
        ui->service->addItem(list[i].service);

    connect(ui->master, &QLineEdit::returnPressed, this, &PassMan::pw_entered);
    connect(ui->pin,&QLineEdit::textEdited,this, &PassMan::pin_entered);  // Signal parameter is passed to slot
    connect(ui->service,&QComboBox::currentIndexChanged,this,&PassMan::service_chosen);
    connect(ui->show,&QCheckBox::checkStateChanged,this,&PassMan::checkchange);
    connect(ui->clean,&QPushButton::clicked,this,&PassMan::clean);
    connect(ui->reset,&QPushButton::clicked,this,&PassMan::reset);
    connect(ui->create,&QPushButton::clicked,this,&PassMan::create);
    connect(ui->add,&QPushButton::clicked,this,&PassMan::add);
    connect(ui->remove,&QPushButton::clicked,this,&PassMan::remove);
    connect(ui->sure,&QPushButton::clicked,this,&PassMan::sure);

    ui->master->setFocus();
    ui->clean->setToolTip("Clear clip-board and all secret data");
    ui->create->setEnabled(1);
    ui->create->setToolTip("Create password for Username/URL/Service");
}

PassMan::~PassMan()
{
    for (int i=0;i<64;i++) {digest[i]=0; ph[i]=0;}
    delete ui;
}

