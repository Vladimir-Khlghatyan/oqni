#include "windowchart.hpp"
#include "mainwindow.h"
#include "debugger.hpp"

WindowChart::WindowChart(MainWindow *parent, const QString &pathToFiles, \
                        QCheckBox *filesList, int filesCount)
    : QDialog(parent)
    , _pathToFiles(pathToFiles) \
    , _filesList(filesList) \
    , _filesCount(filesCount)
{
    DEBUGGER();
    
    int screenWidth = QApplication::primaryScreen()->size().width();
	int screenHeight = QApplication::primaryScreen()->size().height();
    int windowWidth = screenWidth * 9 / 10;
    int windowHeight = screenHeight * 9 / 10;
    
	this->setGeometry((screenWidth - windowWidth) / 2, \
                      (screenHeight - windowHeight) / 2, windowWidth, windowHeight);
	this->setMinimumHeight(windowHeight / 2);
	this->setMinimumWidth(windowWidth / 2);

    this->_chart_OPT = nullptr;
    this->_chart_IMU = nullptr;
    this->_chartView_OPT = nullptr;
    this->_chartView_IMU = nullptr;
    this->_axisX_OPT = nullptr;
    this->_axisX_IMU = nullptr;
    this->_axisY_OPT = nullptr;
    this->_axisY_IMU = nullptr;
    this->_axisYLabel_OPT = nullptr;
    this->_axisYLabel_IMU = nullptr;
    this->_series_OPT = nullptr;
    this->_series_IMU = nullptr;
    this->_numOfSeries_OPT = 0; // initial value
    this->_numOfSeries_IMU = 0; // initial value
    this->_numOfChart_OPT = 1;
    this->_numOfChart_IMU = 3;
    this->_maxLabel_OPT = 0;
    this->_maxLabel_IMU = 0;
    this->_timeLineMin = 0;
    this->_timeLineMax_OPT = 0;
    this->_timeLineMax_IMU = 0;
    this->_zoomToHomeButton = new QPushButton;
    this->_zoomToHomeButton->setEnabled(false);
    QPixmap pixmap(":/Imgs/iconHome.png");
    this->_iconHome = new QIcon(pixmap);
    this->_zoomToHomeButton->setIcon(*_iconHome);
    this->_zoomToHomeButton->setIconSize(pixmap.size());
    this->_zoomToHomeButton->setFixedSize(pixmap.size());
    this->_zoomToHomeButton->setStyleSheet("QPushButton { border: none; }");
    this->_zoomToHomeButton->setMask(pixmap.mask());
    connect(this->_zoomToHomeButton, &QPushButton::clicked, this,
        [=]()
        {
            DEBUGGER();
            
            _axisX_OPT->setRange(_timeLineMin, _timeLineMax_OPT);
            _axisY_OPT->setRange(_valueLineMin_OPT, _valueLineMax_OPT);
            _axisYLabel_OPT->setRange(0, _maxLabel_OPT + 1);

            _chartView_OPT->_zoomed = false;
            _chartView_OPT->_currentAxisXLength = _timeLineMax_OPT - _timeLineMin;
            _chartView_OPT->_currentAxisYLength = _valueLineMax_OPT - _valueLineMin_OPT;
            for (int i = 0; i < _chartView_IMU->size().height(); ++i)
            {
                _chartView_IMU[i]._zoomed = false;
                _chartView_IMU[i]._currentAxisXLength = _timeLineMax_IMU - _timeLineMin;
                _chartView_IMU[i]._currentAxisYLength = _valueLineMax_IMU[i] - _valueLineMin_IMU[i];
            }

			_zoomToHomeButton->setEnabled(false);

            _horizontalScrollBar_OPT->setRange(_timeLineMin, _timeLineMin);
            _horizontalScrollBar_OPT->setValue(_timeLineMin);

            _verticalScrollBar_OPT->setRange(_valueLineMin_OPT, _valueLineMin_OPT);
            _verticalScrollBar_OPT->setValue(_valueLineMin_OPT);
            
            DEBUGGER();
    	});
    
    this->setModal(true);
    this->setStyleSheet("background: #e6e6e6;");
    this->raise();
	this->show();
    
    this->_checkedFilesCount_OPT = 0;
    this->_checkedFilesCount_IMU = 0;
    for	(int i = 0; i < _filesCount; ++i)
    {
        if (_filesList[i].isChecked() == true)
        {
            if (_filesList[i].text().mid(14,3) == "IMU")
                ++_checkedFilesCount_IMU;
            else if (_filesList[i].text().mid(14,3) == "OPT")
                ++_checkedFilesCount_OPT;
        }
    }
    
    this->readFromFile();
    this->execChartDialog();
    
    DEBUGGER();
}

WindowChart::~WindowChart()
{
    DEBUGGER();
    
    delete _axisX_OPT;
    _axisX_OPT = nullptr;

    delete [] _axisX_IMU;
    _axisX_IMU = nullptr;

    delete _axisY_OPT;
    _axisY_OPT = nullptr;

    delete [] _axisY_IMU;
    _axisY_IMU = nullptr;

    delete _axisYLabel_OPT;
    _axisYLabel_OPT = nullptr;

    delete [] _axisYLabel_IMU;
    _axisYLabel_IMU = nullptr;

    for (int i = 0; i < _numOfSeries_OPT; ++i)
        if (_chart_OPT->series().contains(&_series_OPT[i]))
            _chart_OPT->removeSeries(&_series_OPT[i]);
    delete[] _series_OPT;
    _series_OPT = nullptr;

    for (int i = 0; i < _numOfSeries_IMU; ++i)
            _chart_IMU[i / 4].removeSeries(&_series_IMU[i]); // in each _chart_IMU[i] are 4 series (x, y, z and label)
    delete[] _series_IMU;
    _series_IMU = nullptr;

    delete _chart_OPT;
    _chart_OPT = nullptr;
    delete [] _chart_IMU;
    _chart_IMU = nullptr;
    delete _chartView_OPT;
    _chartView_OPT = nullptr;
    delete [] _chartView_IMU;
    _chartView_IMU = nullptr;
    delete _horizontalScrollBar_OPT;
    _horizontalScrollBar_OPT = nullptr;
    delete _verticalScrollBar_OPT;
    _verticalScrollBar_OPT = nullptr;
	delete[] _checkBoxChannelsValue;
    _checkBoxChannelsValue = nullptr;
	delete[] _checkBoxChannels;
    _checkBoxChannels = nullptr;
	delete _hBoxLayout;
    _hBoxLayout = nullptr;
	delete _gridLayout;
    _gridLayout = nullptr;
	delete _zoomToHomeButton;
    _zoomToHomeButton = nullptr;
    
    DEBUGGER();
}

void    WindowChart::readFromFile(void)
{
    DEBUGGER();
    
    QStringList splitList;
    qint64      time;
    
    QFile		*files = new QFile[_checkedFilesCount_OPT + _checkedFilesCount_IMU];
    QTextStream *ins = new QTextStream[_checkedFilesCount_OPT + _checkedFilesCount_IMU];

    // open files and count the number of channels (series) for OPT and IMU
    DEBUGGER();
    for (int i = 0, j = -1; i < _filesCount; ++i)
    {
        if (_filesList[i].isChecked())
        {
			files[++j].setFileName(_pathToFiles + _filesList[i].text());
			files[j].open(QIODevice::ReadOnly | QIODevice::Text);
            ins[j].setDevice(&(files[j]));
            if (_filesList[i].text().mid(14,3) == "IMU")
                _numOfSeries_IMU += ins[j].readLine().count("led") + 1; // counting number of IMU channels (+labels) and omitting first line in file
            else if (_filesList[i].text().mid(14,3) == "OPT")
                _numOfSeries_OPT += ins[j].readLine().count("led") + 1; // counting number of OPT channels (+labels) and omitting first line in file
        }
    }

    // creating series
    this->_series_OPT = new QLineSeries[_numOfSeries_OPT]; // labels in indexes 3 and 7
    this->_series_IMU = new QLineSeries[_numOfSeries_IMU]; // labels in indexes 3, 7 and 11

    // reading data from files to series
    DEBUGGER();
    for (int i = 0, j = 0; i < _filesCount; ++i)
    {
        if (_filesList[i].isChecked())
        {
            while (!ins[j].atEnd())
            {
                splitList = ins[j].readLine().split(',');
                time = splitList[0].toLongLong();

                if (_filesList[i].text().mid(14,3) == "IMU")
                {
                    int l = -1; // for label series tracking
                    // loop over data, except label (label is the last element in splitList)
                    for (int k = 0; k < splitList.size() - 1; ++k)
                    {
                        if (k % _numOfChart_IMU == 0)
                            ++l;
                        // we want omit series at indexes 3, 7 and 11 for labels, so we add increase 'l' and add it to 'k'
                        _series_IMU[k + l].append(time, splitList[k + 1].toUInt()); // k+1, because at index 0 is the time in millisec
                    }

                    // loop over label series at indexes 3, 7 and 11
                    for (int k = 3; k < splitList.size(); k += 4)
                        _series_IMU[k].append(time, splitList[splitList.size() - 1].toUInt()); // label is the last element in splitList
                }
                else if (_filesList[i].text().mid(14,3) == "OPT")
                    if (_checkedFilesCount_OPT)
                        for (int k = 0; k < splitList.size(); ++k)
                            _series_OPT[k + (j - _checkedFilesCount_IMU) * _numOfSeries_OPT / _checkedFilesCount_OPT].append(time, splitList[k + 1].toUInt()); // k+1, because at index 0 is the time in millisec
            }
            files[j++].close();
        }
    }

    DEBUGGER();
    if (_checkedFilesCount_OPT)
        _timeLineMax_OPT = _series_OPT[0].at(_series_OPT[0].count() - 1).x();
    if (_checkedFilesCount_IMU)
        _timeLineMax_IMU = _series_IMU[0].at(_series_IMU[0].count() - 1).x();

    delete [] files;
    delete [] ins;
    
    DEBUGGER();
}

void    WindowChart::updateValueLineAxis(void)
{
    DEBUGGER();
    
    if (this->_chartView_OPT != nullptr && this->_chartView_OPT->_zoomed == true)
        return ;
    
    bool flag = false;
    
    this->_valueLineMin_OPT = -1;
    this->_valueLineMax_OPT = 0;

    for (int i = 0; i < _numOfSeries_OPT; i++)
	{
        if (_checkBoxChannelsValue[i] == false)
            continue ;
        flag = true;
        for(int j = 0; j < _series_OPT[i].count(); j++)
		{
            if (_series_OPT[i].at(j).x() >= _timeLineMin && _series_OPT[i].at(j).x() <= _timeLineMax_OPT)
            {
                _valueLineMax_OPT = std::max((unsigned)_series_OPT[i].at(j).y(), _valueLineMax_OPT);
                _valueLineMin_OPT = std::min((unsigned)_series_OPT[i].at(j).y(), _valueLineMin_OPT);
            }
		}
	}
    if (flag == false)
    {
        this->_valueLineMin_OPT = 0;
        this->_valueLineMax_OPT = 1;
    }
    _axisY_OPT->setRange(_valueLineMin_OPT, _valueLineMax_OPT);
    
    DEBUGGER();
}

void    WindowChart::execChartDialog(void)
{
    DEBUGGER();
    
    this->_chart_OPT = new QChart();
    
    int i = 0;
    for (; i < _filesCount; ++i)
        if (_filesList[i].isChecked() == true)
            break ;
    _chart_OPT->setTitle(this->staticChartTitle(_pathToFiles + _filesList[i].text()));
    
    QFont font;
    font.setBold(true);
    font.setPointSize(14);
    _chart_OPT->setTitleFont(font);
    _chart_OPT->setBackgroundBrush(QBrush(QColor::fromRgb(235, 255, 255)));
    _chart_OPT->legend()->hide();

    for (int i = 0; i < _numOfSeries_OPT; ++i)
	{
        _chart_OPT->addSeries(&_series_OPT[i]);
        if (_checkedFilesCount_OPT) {
            switch (i % (_numOfSeries_OPT / _checkedFilesCount_OPT)) {
            case 0:
                _series_OPT[i].setColor(Qt::green);
                break;
            case 1:
                _series_OPT[i].setColor(Qt::red);
                break;
            case 2:
                _series_OPT[i].setColor(Qt::blue); // infraRed
                break;
            case 3:
                _series_OPT[i].setColor(Qt::black); // label
            }
        }
	}

    // creating axis X for OPT sensors
    this->_axisX_OPT = new QValueAxis();
    _axisX_OPT->setTitleText("Time (milliseconds)");
    _chart_OPT->addAxis(_axisX_OPT, Qt::AlignBottom);
    for (int i = 0; i < _numOfSeries_OPT; ++i)
        _series_OPT[i].attachAxis(_axisX_OPT);

    // creating axis X for IMU sensors
    // for 3 charts: accelerometer, gyroscope, magnetometer
    this->_axisX_IMU = new QValueAxis[_numOfChart_IMU];
    for (int i = 0; i < _numOfChart_IMU; ++i)
        _chart_IMU[i].addAxis(&_axisX_IMU[i], Qt::AlignBottom);
    for (int i = 0; i < _numOfSeries_IMU; ++i)
            _series_IMU[i].attachAxis(&_axisX_IMU[i / _numOfChart_IMU]);

    // creating axis Y for OPT sensors and labels
    this->_axisY_OPT = new QValueAxis();
    this->_axisYLabel_OPT = new QValueAxis();
    _axisY_OPT->setTitleText("Values");
    _axisYLabel_OPT->setTitleText("Label");
    _chart_OPT->addAxis(_axisY_OPT, Qt::AlignLeft);
    _chart_OPT->addAxis(_axisYLabel_OPT, Qt::AlignRight);
    for (int i = 0; i < _numOfSeries_OPT; ++i)
    {
        if (_checkedFilesCount_OPT) {
            switch (i % (_numOfSeries_OPT / _checkedFilesCount_OPT)) {
            case 0 ... 2:
                _series_OPT[i].attachAxis(_axisY_OPT);
                break;
            case 3: // series at indexes 3 and 7 are labels
                break;
                _series_OPT[i].attachAxis(_axisYLabel_OPT);
                for (int j = 0; j < _series_OPT[i].count(); ++j)
                    _maxLabel_OPT = std::max((int)_series_OPT[i].at(j).y(), _maxLabel_OPT);
            }
        }
    }
    _axisYLabel_OPT->setRange(0, _maxLabel_OPT + 1);

    // creating axis Y for IMU sensors and labels
    // for 3 charts: accelerometer, gyroscope, magnetometer
    this->_axisY_IMU = new QValueAxis[_numOfChart_IMU];
    this->_axisYLabel_IMU = new QValueAxis[_numOfChart_IMU];
    for (int i = 0; i < _numOfChart_IMU; ++i)
    {
        _axisY_IMU[i].setTitleText("Values");
        _axisYLabel_IMU[i].setTitleText("Label");
        _chart_IMU[i].addAxis(&_axisY_IMU[i], Qt::AlignLeft);
        _chart_IMU[i].addAxis(&_axisYLabel_IMU[i], Qt::AlignLeft);
    }
    for (int i = 0; i < _numOfSeries_IMU; ++i)
    {
        if (_checkedFilesCount_IMU)
        {
            switch (i % 4) {
            case 0 ... 2:
                _series_IMU[i].attachAxis(_axisY_IMU);
                break;
            case 3: // series at indexes 3, 7 and 11 are labels
                break;
                _series_IMU[i].attachAxis(_axisYLabel_IMU);
                for (int j = 0; j < _series_IMU[i].count(); ++j)
                    _maxLabel_IMU = std::max((int)_series_IMU[i].at(j).y(), _maxLabel_IMU);
            }
        }
    }
    _axisYLabel_IMU->setRange(0, _maxLabel_IMU + 1);








	
    this->_checkBoxChannelsValue = new bool[_numOfSeries_OPT];
    for (int i = 0; i < _numOfSeries_OPT; ++i)
        this->_checkBoxChannelsValue[i] = true;
    
	this->updateValueLineAxis();
    _axisX_OPT->setRange(_timeLineMin, _timeLineMax_OPT);
    
    this->_horizontalScrollBar_OPT = new QScrollBar(Qt::Horizontal, this);
    this->_horizontalScrollBar_OPT->setRange(0, 0);
    connect(this->_horizontalScrollBar_OPT, &QScrollBar::valueChanged, this,
        [=](int value)
    	{
            DEBUGGER();
            this->_axisX_OPT->setRange(value, value + this->_chartView_OPT->_currentAxisXLength);
            DEBUGGER();
		});
    
    this->_verticalScrollBar_OPT = new QScrollBar(Qt::Vertical, this);
    this->_verticalScrollBar_OPT->setRange(0, 0);
    connect(this->_verticalScrollBar_OPT, &QScrollBar::valueChanged, this,
        [=](int value)
    	{
            DEBUGGER();
            this->_axisY_OPT->setRange(value, value + this->_chartView_OPT->_currentAxisYLength);
            DEBUGGER();
		});

    this->_chartView_OPT = new MyChartView(_chart_OPT, _timeLineMin, _timeLineMax_OPT, _valueLineMin_OPT, _valueLineMax_OPT, \
                                       _axisX_OPT, _axisY_OPT, _axisYLabel_OPT, _maxLabel_OPT, \
                                       _zoomToHomeButton, _horizontalScrollBar_OPT, _verticalScrollBar_OPT);
    this->_chartView_OPT->setRenderHint(QPainter::Antialiasing);
    this->_chartView_OPT->setRubberBand(QChartView::RectangleRubberBand);
    
    this->_horizontalScrollBar_OPT->setParent(this->_chartView_OPT);
    this->_verticalScrollBar_OPT->setParent(this->_chartView_OPT);
    this->_verticalScrollBar_OPT->setInvertedAppearance(true); // reverse the direction
    
	this->_gridLayout = new QGridLayout;
	
	this->_hBoxLayout = new QHBoxLayout;
    this->_checkBoxChannels = new QCheckBox[_numOfSeries_OPT];

    for (int k = 0, j = -1; k < _filesCount; ++k)
    {
        if (_filesList[k].isChecked() == false || _filesList[k].text().mid(14,3) == "IMU")
            continue ;
        ++j;        
        for (int i = 0; i < _numOfSeries_OPT / _checkedFilesCount_OPT; ++i) // (_numOfSeries_OPT / _checkedFilesCount_OPT) ==> number of checkboxes in one block
        {
            int index = i + j * (_numOfSeries_OPT / _checkedFilesCount_OPT);
            switch (i){
            case 0:
                this->_checkBoxChannels[index].setText("OPT" + QString::number(k) + "green  ");
                this->_checkBoxChannels[index].setStyleSheet("color: green; font-size: 14px;");
                break;
            case 1:
                this->_checkBoxChannels[index].setText("OPT" + QString::number(k) + "red  ");
                this->_checkBoxChannels[index].setStyleSheet("color: red; font-size: 14px;");
                break;
            case 2:
                this->_checkBoxChannels[index].setText("OPT" + QString::number(k) + "infrared  ");
                this->_checkBoxChannels[index].setStyleSheet("color: blue; font-size: 14px;");
                break;
            case 3:
                this->_checkBoxChannels[index].setText("Label" + QString::number(k) + "      ");
                this->_checkBoxChannels[index].setStyleSheet("color: blue; font-size: 14px;");
                break;
            }            
            this->_checkBoxChannels[index].setChecked(true);
            this->connectStaticChatCheckBox(index);
            this->_hBoxLayout->addWidget(&_checkBoxChannels[index]);
        }
    }
    
    this->_gridLayout->addWidget(this->_chartView_OPT, 0, 0, 1, 5);
    this->_gridLayout->addWidget(this->_verticalScrollBar_OPT, 0, 0, 1, 5, Qt::AlignRight);
    this->_gridLayout->addWidget(this->_horizontalScrollBar_OPT, 0, 0, 1, 5, Qt::AlignBottom);
    this->_gridLayout->addLayout(_hBoxLayout, 1, 0, 1, 4, Qt::AlignCenter);
    this->_gridLayout->addWidget(this->_zoomToHomeButton, 1, 4, 1, 1, Qt::AlignVCenter); 
    
	this->setLayout(this->_gridLayout);
    
    DEBUGGER();
}

QString WindowChart::staticChartTitle(const QString &selectedFile)
{
    DEBUGGER();
    
    QString tmp = "Unknown file";
    //the Unicode non-breaking space character (\u00A0)
    QString title = "---\u00A0\u00A0\u00A0\u00A0#---\u00A0\u00A0\u00A0\u00A0";

    int lastDot = selectedFile.lastIndexOf('.');
    int lastUnderscoreLine = selectedFile.lastIndexOf('_');
    int lastSlash = selectedFile.lastIndexOf('/');
    if (lastDot == -1 || lastUnderscoreLine == -1 || lastSlash == -1)
    {
        DEBUGGER();
        return tmp;
    }

    title += selectedFile.mid(lastSlash + 1, lastUnderscoreLine - lastSlash - 1) + \
            "\u00A0\u00A0\u00A0\u00A0";
    for (int i = 0; i < _filesCount; ++i)
    {
        if (_filesList[i].isChecked() == true)
        {
			int lastDot_tmp = _filesList[i].text().lastIndexOf('.');
			int lastUnderscoreLine_tmp = _filesList[i].text().lastIndexOf('_');
			title += _filesList[i].text().mid(lastUnderscoreLine_tmp + 1, lastDot_tmp - lastUnderscoreLine_tmp - 1);
            title += "\u00A0\u00A0\u00A0";
        }
    }

    tmp = selectedFile.left(lastSlash);
    lastSlash = tmp.lastIndexOf('/');
    tmp = tmp.mid(lastSlash + 1);

    lastUnderscoreLine = tmp.lastIndexOf('_');
    if (lastUnderscoreLine != -1)
    {
        tmp = tmp.left(lastUnderscoreLine);
        lastUnderscoreLine = tmp.lastIndexOf('_');
        if (lastUnderscoreLine != -1)
            title = tmp.left(lastUnderscoreLine) + "\u00A0\u00A0\u00A0\u00A0#" + \
                    tmp.mid(lastUnderscoreLine + 1) + title.mid(11);
    }
    DEBUGGER();
    return title;
}

void WindowChart::connectStaticChatCheckBox(int i)
{
    DEBUGGER();
    
    connect(&this->_checkBoxChannels[i], &QCheckBox::clicked, this,
        [=]()
        {
            DEBUGGER();
            
            if (this->_checkBoxChannels[i].isChecked() == true)
            {
                _chart_OPT->addSeries(&_series_OPT[i]);
                _series_OPT[i].attachAxis(_axisX_OPT);
                if (i % (_numOfSeries_OPT / _checkedFilesCount_OPT) == 3) // if (i % 4 == 3), it means case of label
                    _series_OPT[i].attachAxis(_axisYLabel_OPT);
                else
                    _series_OPT[i].attachAxis(_axisY_OPT);
                this->_checkBoxChannelsValue[i] = true;
            }
            else
            {
                _chart_OPT->removeSeries(&_series_OPT[i]);
                this->_checkBoxChannelsValue[i] = false;
            }
            this->updateValueLineAxis();
            _chart_OPT->update();
        });
    
    DEBUGGER();
}
