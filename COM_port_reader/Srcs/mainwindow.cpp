#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _portCount(0)
{
    ui->setupUi(this);
    
    this->putWindowOnScreen(700, 616);
    this->_buttonCheck = this->createButton("Check connected ports", 20, 30, 380, 30, std::bind(&MainWindow::buttonCheckAction, this), this);
    this->addLoadingAnimation(this->_buttonCheck, 21, 150, 370, 370);
    this->createGroupBox(20, 70, 380, 515);
    this->createLiftVertical(379, 71, 20, 513);
    this->_buttonSaveTo = this->createButton("Save to", 560, 555, 100, 30, std::bind(&MainWindow::buttonSaveToAction, this), this);
    
    _baudRateItems = {"1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200"};
    _dataBitsItems = {"5", "6", "7", "8"};
    _parityItems = {"None", "Even", "Odd", "Space", "Mark"};
    _stopBitsItems = {"1", "1.5", "2"};
    _flowControlItems = {"None", "Hardware", "Xon / Xoff"};
}

MainWindow::~MainWindow()
{
    for (QVector<ComPort *>::iterator it = _comPorts.begin(); it < _comPorts.end(); ++it)
        delete *it;
    this->_comPorts.clear();
    delete _buttonCheck;
    delete _gifLabel;
    delete _gifMovie;
    delete _liftVertical;
    delete ui;
}

void    MainWindow::putWindowOnScreen(int windowWidth, int windowHeight)
{
    /* ------ put window to the center of the screen ------ */
    QScreen *screen = QApplication::primaryScreen();
    QSize screenSize = screen->size();
    int screenWidth = screenSize.width();
    int screenHeight = screenSize.height();
    this->setGeometry((screenWidth - windowWidth) / 2, \
                      (screenHeight - windowHeight) / 2, \
                      windowWidth, windowHeight);
    this->setMinimumSize(windowWidth, windowHeight);
    this->setMaximumSize(windowWidth, windowHeight);

    /* ------------- background, icon, and title ---------- */
    this->setWindowTitle("OQNI: COM port reader");
    this->setWindowIcon(QIcon(":/Imgs/oqni.ico"));
    this->setWindowFilePath(":/Imgs/oqni.ico");
    this->setStyleSheet("background-image: url(:/Imgs/background.png); font-size: 20px");
}

QPushButton    *MainWindow::createButton(const QString &name, int x, int y, int width, \
                                         int height, std::function<void(void)> onPressAction, QWidget *box)
{
    QPushButton *button;
   
    /* ------------------- Button design ------------------ */
    button = new QPushButton(name, box);
    button->setGeometry(x, y, width, height);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(MY_DEFINED_DEFAULT_BUTTON);
    connect(button, &QPushButton::released, button,
        [=](void)
        {
            button->setStyleSheet(MY_DEFINED_RELEASED_BUTTON);
        });
    connect(button, &QPushButton::clicked, button,
        [=](void)
        {
            button->setStyleSheet(MY_DEFINED_DEFAULT_BUTTON);
        });
    connect(button, &QPushButton::pressed, button,
        [=](void)
        {
            button->setStyleSheet(MY_DEFINED_PRESSED_BUTTON);
            if (onPressAction != nullptr)
                onPressAction();
        });

    return (button);
}
    
void    MainWindow::addLoadingAnimation(QPushButton *button, int x, int y, int width, int height)
{
    /* ---------------- Button functional ----------------- */
    this->_gifLabel = new QLabel(this);
    this->_gifLabel->setGeometry(x, y, width, height);
    this->_gifLabel->stackUnder(button);
    this->_gifLabel->hide();
    this->_gifMovie = new QMovie(":/Imgs/loading.gif");
    this->_gifMovie->setScaledSize(this->_gifLabel->size());
    this->_gifLabel->setMovie(this->_gifMovie);
    this->_gifLabel->setStyleSheet("background: #e6e6e6;");
}

void    MainWindow::createGroupBox(int x, int y, int width, int height)
{
    /* ---------------- adding GroupBox ------------------- */
    this->_groupBox = new QGroupBox("Connected COM ports:", this);
    this->_groupBox->setGeometry(x, y, width, height);
    this->_groupBox->stackUnder(this->_gifLabel);
    this->_groupBox->setStyleSheet("border: 1px solid gray; background: #e6e6e6;");
}

void    MainWindow::createLiftVertical(int x, int y, int width, int height)
{
    /* ----------- adding Vertical ScrollBar -------------- */
    this->_liftVertical = new QScrollBar(Qt::Vertical, this);
    this->_liftVertical->setGeometry(x, y, width, height);
    this->_liftVertical->hide();
    connect(this->_liftVertical, &QScrollBar::valueChanged, this->_groupBox,
        [=](void)
        {
            int liftRatio;
        
            for (QVector<ComPort *>::iterator it = _comPorts.begin(); it < _comPorts.end(); ++it)
            {
                liftRatio = 40 * (1 + (it - _comPorts.begin()) - this->_liftVertical->value());
                (*it)->getCheckBox()->setGeometry(40, liftRatio, 285, 20);
                (*it)->getCheckBox()->raise();

                (*it)->getToolButton()->setGeometry(5, liftRatio - 5, 30, 30);
                (*it)->getToolButton()->raise();
                if (liftRatio >= 40)
                {
                    (*it)->getCheckBox()->show();
                    if ((*it)->getCheckBox()->isChecked() == true )
                    (*it)->getToolButton()->show();
                }
                else
                {
                    (*it)->getCheckBox()->hide();
                    (*it)->getToolButton()->hide();
                }
                (*it)->getCheckBox()->setStyleSheet("border: 0px solid gray;");
            }
        });
}

void    MainWindow::buttonCheckAction(void)
{
    /* ----------- show animation and update checkboxes' list -------------- */

    this->_previewsCheckBox = nullptr;
    this->_gifLabel->show();
    this->_gifMovie->start();
    this->_liftVertical->hide();
    for (QVector<ComPort *>::iterator it = _comPorts.begin(); it < _comPorts.end(); ++it)
    {
        (*it)->getCheckBox()->hide();
        (*it)->getToolButton()->hide();
    }
    QTimer::singleShot(1000, this->_gifLabel, &QLabel::hide);
    QTimer::singleShot(1000, this,
        [=](void)
        {
            for (QVector<ComPort *>::iterator it = _comPorts.begin(); it < _comPorts.end(); ++it)
                delete (*it);
            this->_comPorts.clear();
                
            QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
            for (const QSerialPortInfo& port : portList)
                this->_comPorts.push_back(new ComPort(port, this->_groupBox));

            this->_portCount = this->_comPorts.size();
            if (this->_portCount > 12)
            {
                this->_liftVertical->setValue(0);
                this->_liftVertical->show();
                this->_liftVertical->setMinimum(0);
                this->_liftVertical->setMaximum(this->_portCount - 12);
            }
            for (QVector<ComPort *>::iterator it = _comPorts.begin(); it < _comPorts.end(); ++it)
            {
                (*it)->getCheckBox()->setGeometry(40, 40 * (1 + (it - _comPorts.begin())), 285, 20);
                (*it)->getCheckBox()->raise();
                (*it)->getCheckBox()->show();
                (*it)->getCheckBox()->setStyleSheet("border: 0px solid gray;");

                (*it)->getToolButton()->setGeometry(5, 40 * (1 + (it - _comPorts.begin())) - 5, 30, 30);

                connect((*it)->getCheckBox(), &QRadioButton::clicked, (*it)->getToolButton(),
                    [=](void)
                    {
                        (*it)->getToolButton()->raise();
                        (*it)->getToolButton()->show();
                        if (this->_previewsCheckBox && this->_previewsCheckBox != *it)
                            this->_previewsCheckBox->getToolButton()->hide();
                        this->_previewsCheckBox = *it;
                    });
                if ((*it)->getCheckBox()->isChecked() == false)
                    (*it)->getToolButton()->hide();
                
                connect((*it)->getToolButton(), &QToolButton::clicked, this,
					[=](void)
					{
						this->buttonToolAction(*it);
					});
            }
        });
}

void    MainWindow::setParametersDesign(QLabel *showReadingPort1, QLabel *showReadingPort2, \
										QLabel *showSelectedDir1, QLabel *showSelectedDir2, \
										QLabel *setTimer1, QLabel *setTimer2, \
                                    		QLineEdit *lineEdit, QString &selectedDirectory)
{
    showReadingPort1->setGeometry(10, 10, 100, 30);
    showReadingPort2->setGeometry(120, 10, 480, 30);
    showReadingPort2->setStyleSheet("font-size: 14px; color: blue;");
    
    showSelectedDir1->setGeometry(10, 40, 100, 30);
    showSelectedDir2->setGeometry(120, 40, 480, 30);
    showSelectedDir2->setToolTip(selectedDirectory);
    showSelectedDir2->setStyleSheet("font-size: 14px; color: blue;");

    setTimer1->setGeometry(10, 70, 100, 30);
    setTimer2->setGeometry(210, 70, 100, 30);
    setTimer2->setStyleSheet("font-size: 14px; color: blue;");

    lineEdit->setPlaceholderText("enter here");
    lineEdit->setGeometry(120, 70, 83, 30);
    lineEdit->setStyleSheet("background: white; font-size: 14px; padding: 0 5px; color: blue;");
    lineEdit->setToolTip("Please enter only numeric values.");
    lineEdit->setMaxLength(4);
    lineEdit->setAlignment(Qt::AlignCenter);
    this->_durationTimerValue = 0;

    /* --- If the text contains a non-numeric character, show warrnig msg --- */
    connect(lineEdit, &QLineEdit::textChanged, this->_windowSaveTo,
        [=](void)
        {
        	if (lineEdit->text().length() == 0)
            {
                lineEdit->setStyleSheet("background: white; font-size: 14px; padding: 0 5px; color: blue;");
                this->_durationTimerValue = 0;
                return ;
			}
            QString text = lineEdit->text();
            bool hasOnlyDigits = true;
            for (int i = 0; i < text.length(); i++)
            {
                if (text[i].isDigit() == false)
                {
                    hasOnlyDigits = false;
                    lineEdit->setStyleSheet("background-color: red; padding: 0 5px; color: blue;");
                    this->_durationTimerValue = 0;
                    QMessageBox::warning(this->_windowSaveTo, tr("Invalid Input"),
                                        tr("Please enter a numeric value."), QMessageBox::Ok);
                    break ;
                }
            }
            if (hasOnlyDigits == true)
            {
                lineEdit->setStyleSheet("background-color: white; padding: 0 5px; color: blue;");
                this->_durationTimerValue = text.toInt();
            }
        });
}

void    MainWindow::windowSaveToButtonsFunctionality(QPushButton *start, QPushButton *stop, QPushButton *close, QLineEdit *lineEdit)
{
    stop->setEnabled(false);
    stop->setStyleSheet("border-radius: 6px; background-color: #D3D3D3;");
    connect(close, &QPushButton::clicked, this->_windowSaveTo,
		[=](void)
		{
            this->_windowSaveTo->close();
		});
    connect(start, &QPushButton::clicked, this->_windowSaveTo,
		[=](void)
		{
            if (this->_durationTimerValue == 0)
                return ;
            close->setEnabled(false);
            close->setStyleSheet("border-radius: 6px; background-color: #D3D3D3;");
            start->setEnabled(false);
			start->setStyleSheet("border-radius: 6px; background-color: #D3D3D3;");
			stop->setEnabled(true);
            stop->setStyleSheet(MY_DEFINED_DEFAULT_BUTTON);
            lineEdit->setEnabled(false);
            lineEdit->setStyleSheet("background-color: #D3D3D3; padding: 0 5px; color: blue;");
            this->_threadDisplayTimer = new ThreadDisplayTimer(this->_durationTimerValue, this->_windowSaveTo);
            this->_threadDisplayTimer->start();
		});
    connect(stop, &QPushButton::clicked, this->_windowSaveTo,
		[=](void)
		{
            close->setEnabled(true);
            close->setStyleSheet(MY_DEFINED_DEFAULT_BUTTON);
            start->setEnabled(true);
            start->setStyleSheet(MY_DEFINED_DEFAULT_BUTTON);
			stop->setEnabled(false);
			stop->setStyleSheet("border-radius: 6px; background-color: #D3D3D3;");
            lineEdit->setEnabled(true);
            lineEdit->setStyleSheet("background-color: white; padding: 0 5px; color: blue;");
            this->_threadDisplayTimer->requestInterruption();
            this->_threadDisplayTimer->wait();
            delete this->_threadDisplayTimer;
		});
//    connect(this->_threadDisplayTimer, &ThreadDisplayTimer::finished, this, &MainWindow::onThreadDisplayTimerFinished);
}

void    MainWindow::buttonSaveToAction()
{
    ComPort     *comPort = nullptr;
    QFileDialog dialog;
    QString     selectedDirectory;
    QString     fileName;
    
    this->_windowSaveTo = new QDialog(this);
    this->_windowSaveTo->setModal(true);
    this->_windowSaveTo->setMinimumSize(500, 700);
    this->_windowSaveTo->setMaximumSize(500, 700);
    this->_windowSaveTo->setWindowTitle("OQNI: Drawer");
    this->_windowSaveTo->setWindowIcon(QIcon(":/Imgs/oqni.ico"));
    this->_windowSaveTo->setWindowFilePath(":/Imgs/oqni.ico");
    this->_windowSaveTo->setStyleSheet("background: #e6e6e6;");
    this->_windowSaveTo->setWindowFlag(Qt::WindowCloseButtonHint, false); // Remove the closing button

    for (QVector<ComPort *>::iterator it = _comPorts.begin(); it != _comPorts.end(); ++it)
    {
        if ((*it)->getCheckBox()->isChecked() == true )
        {
            comPort = *it;
            break ;
        }
    }
    if (comPort == nullptr)
    {
        delete this->_windowSaveTo;
        return ;
    }
    
    dialog.setOption(QFileDialog::ShowDirsOnly);
    selectedDirectory = dialog.getExistingDirectory(this->_windowSaveTo, tr("Save to"), \
                                            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    if (selectedDirectory == "")
    {
        delete this->_windowSaveTo;
        this->_buttonSaveTo->setStyleSheet(MY_DEFINED_RELEASED_BUTTON);
        return ;
    }
    fileName = selectedDirectory + "/" + createFileName(comPort->getPortName());

    QPushButton	*close = this->createButton("Close", 10, 110, 100, 30, nullptr, this->_windowSaveTo);
    QPushButton	*start = this->createButton("Start", 120, 110, 100, 30, nullptr, this->_windowSaveTo);
    QPushButton	*stop = this->createButton("Stop", 230, 110, 100, 30, nullptr, this->_windowSaveTo);

    QLabel *showReadingPort1 = new QLabel("Read from:", this->_windowSaveTo);
    QLabel *showReadingPort2 = new QLabel(comPort->getPortName(), this->_windowSaveTo);
    QLabel *showSelectedDir1 = new QLabel("Save to:", this->_windowSaveTo);
    QLabel *showSelectedDir2 = new QLabel(selectedDirectory, this->_windowSaveTo);
    QLabel *setTimer1 = new QLabel("Duration:", this->_windowSaveTo);
    QLabel *setTimer2 = new QLabel("seconds  ", this->_windowSaveTo);
    QLineEdit *lineEdit = new QLineEdit(this->_windowSaveTo);
    
    this->setParametersDesign(showReadingPort1, showReadingPort2, \
                        showSelectedDir1, showSelectedDir2, \
                        setTimer1, setTimer2, lineEdit, selectedDirectory);
    
    this->windowSaveToButtonsFunctionality(start, stop, close, lineEdit);
    
//	ThreadRuner *threadReader = new ThreadRuner(comPort, fileName.toStdString());
//	threadReader->start();
    
    this->_windowSaveTo->exec();
    this->_buttonSaveTo->setStyleSheet(MY_DEFINED_RELEASED_BUTTON);
    delete showReadingPort1;
    delete showReadingPort2;
    delete showSelectedDir1;
    delete showSelectedDir2;
    delete setTimer1;
    delete setTimer2;
    delete lineEdit;
    delete close;
    delete start;
    delete stop;
    delete this->_windowSaveTo;
}

void    MainWindow::buttonToolAction(ComPort *comPort)
{
    comPort->_windowProperty = new QDialog(this);
    comPort->_windowProperty->setModal(true);

    comPort->_windowProperty->setMinimumSize(360, 300);
    comPort->_windowProperty->setMaximumSize(360, 300);
    comPort->_windowProperty->setWindowTitle("Properties");
    comPort->_windowProperty->setWindowIcon(QIcon(":/Imgs/oqni.ico"));
    comPort->_windowProperty->setWindowFilePath(":/Imgs/oqni.ico");
    comPort->_windowProperty->setStyleSheet("background: #e6e6e6;");
    
    QLabel *portName = new QLabel("Port name:         " + comPort->getPortName(), comPort->_windowProperty);
    portName->setGeometry(10, 10, 430, 30);
    QLabel *baudRate = new QLabel("Baud Rate:" , comPort->_windowProperty);
    baudRate->setGeometry(10, 50, 130, 30);
    QLabel *dataBits = new QLabel("Data Bits:", comPort->_windowProperty);
    dataBits->setGeometry(10, 90, 130, 30);
    QLabel *parity = new QLabel("Parity:", comPort->_windowProperty);
    parity->setGeometry(10, 130, 130, 30);
    QLabel *stopBits = new QLabel("Stop Bits:", comPort->_windowProperty);
    stopBits->setGeometry(10, 170, 130, 30);
    QLabel *flowControl = new QLabel("Flow Control:", comPort->_windowProperty);
    flowControl->setGeometry(10, 210, 130, 30);
    
    
    QComboBox *baudComboBox = new QComboBox(comPort->_windowProperty);
    baudComboBox->addItems(this->_baudRateItems);
    baudComboBox->setGeometry(150, 50, 200, 30);
    
    QComboBox *dataComboBox = new QComboBox(comPort->_windowProperty);
    dataComboBox->addItems(this->_dataBitsItems);
    dataComboBox->setGeometry(150, 90, 200, 30);
    
    QComboBox *parityComboBox = new QComboBox(comPort->_windowProperty);
    parityComboBox->addItems(this->_parityItems);
    parityComboBox->setGeometry(150, 130, 200, 30);
    
    QComboBox *stopComboBox = new QComboBox(comPort->_windowProperty);
    stopComboBox->addItems(this->_stopBitsItems);
    stopComboBox->setGeometry(150, 170, 200, 30);
    
    QComboBox *flowComboBox = new QComboBox(comPort->_windowProperty);
    flowComboBox->addItems(this->_flowControlItems);
    flowComboBox->setGeometry(150, 210, 200, 30);
    
    comPort->_cancelProperties = this->createButton("Cancel", 10, 255, 100, 30, nullptr, comPort->_windowProperty);
    comPort->_setDefaultProperties = this->createButton("Default", 130, 255, 100, 30, nullptr, comPort->_windowProperty);
    comPort->_saveProperies = this->createButton("Save", 250, 255, 100, 30, nullptr, comPort->_windowProperty);

    baudComboBox->setCurrentIndex(comPort->getBaudRateIndex());
    dataComboBox->setCurrentIndex(comPort->getDataBitsIndex());
    parityComboBox->setCurrentIndex(comPort->getParityIndex());
    stopComboBox->setCurrentIndex(comPort->getStopBitsIndex());
    flowComboBox->setCurrentIndex(comPort->getFlowControlIndex());
    
    connect(comPort->_cancelProperties, &QPushButton::clicked, comPort->_windowProperty,
		[=](void)
		{
            comPort->_windowProperty->close();
		});
    connect(comPort->_setDefaultProperties, &QPushButton::clicked, comPort->_windowProperty,
		[=](void)
		{
            baudComboBox->setCurrentIndex(7);
            dataComboBox->setCurrentIndex(3);
            parityComboBox->setCurrentIndex(0);
            stopComboBox->setCurrentIndex(0);
            flowComboBox->setCurrentIndex(0);
		});
    connect(comPort->_saveProperies, &QPushButton::clicked, comPort->_windowProperty,
        [=](void)
        {
            comPort->setBaudRate(baudComboBox->currentText(), this->_baudRateItems);
            comPort->setDataBits(dataComboBox->currentText(), this->_dataBitsItems);
            comPort->setParity(parityComboBox->currentText(), this->_parityItems);
            comPort->setStopBits(stopComboBox->currentText(), this->_stopBitsItems);
            comPort->setFlowControl(flowComboBox->currentText(), this->_flowControlItems);
            comPort->_windowProperty->close();
        });

    comPort->_windowProperty->exec();
    delete portName;
    delete baudRate;
    delete dataBits;
    delete parity;
    delete stopBits;
    delete flowControl;
    delete baudComboBox;
    delete dataComboBox;
    delete parityComboBox;
    delete stopComboBox;
    delete flowComboBox;
    delete comPort->_windowProperty;
}

const QString   MainWindow::createFileName(const QString &portName)
{
    QDateTime         currentDateTime = QDateTime::currentDateTime();
    const QString     formattedDateTime = currentDateTime.toString("yyyy-MM-dd_hh-mm-ss");
    const QString     fileName = portName + "_" + formattedDateTime + ".csv";

    return fileName;
}

//void   MainWindow::onThreadDisplayTimerFinished(void)
//{
//    ThreadDisplayTimer *childThread = qobject_cast<ThreadDisplayTimer *>(sender());
//    if (childThread)
//        childThread->deleteLater();
//}
