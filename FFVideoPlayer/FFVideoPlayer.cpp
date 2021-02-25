/*

主窗口

*/

#include "FFVideoPlayer.h"
#include <QFileDialog>
#include "MyFFmpeg.h"
#include <QMessageBox>
#include "AudioPlay.h"
#include <string>
#include <iostream>
#include "VideoThread.h"
#include <QCursor>
#include "AboutDlg.h"
#include "NetStreamThread.h"
#include <QDesktopServices>


using namespace std;

//全局静态变量只在该cpp文件中有效
static bool ispressSlider = false;

static bool g_isPlay = true;
int g_NetStream;

FFVideoPlayer::FFVideoPlayer(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	m_Hour = 0;
	m_Min = 0;
	m_Second = 0;
	m_isMute = false;

	showMaximized(); //最大化显示   
	setMinimumSize(QSize(800, 512)); //设置窗口最小尺寸

	initUI();

#ifdef _DEBUG
	setWindowTitle("FFVideoPlayer_D");
#else
	setWindowTitle("FFVideoPlayer");
#endif

	startTimer(40);
	allConnect();
}

void FFVideoPlayer::initUI()
{
	//MainWindow背景色
	setStyleSheet("background-color: rgb(53, 53, 53);");

	//菜单颜色
	ui.menuBar->setStyleSheet("background-color:rgb(53,53,53); border:1px solid gray; color:white; padding: 1px;");

	ui.label_Playtime->clear();
	ui.label_Playtime->setText("00:00:00/00:00:00");

	//各个按钮贴图
	setButtonBackImage(ui.btnPlayVideo, "./Resources/play.png");
	setButtonBackImage(ui.btnSetVolume, "./Resources/volume.png");
	setButtonBackImage(ui.btnFullScreen, "./Resources/fullscreen.png");
}

void FFVideoPlayer::allConnect()
{
	connect(ui.btnPlayVideo, SIGNAL(clicked()), this, SLOT(slotPlay()));
	connect(ui.timeSlider, SIGNAL(sliderPressed()), this, SLOT(slotSliderPressed()));
	connect(ui.timeSlider, SIGNAL(sliderReleased()), this, SLOT(soltSliderReleased()));

	//菜单
	connect(ui.action_OpenLocalFiles, SIGNAL(triggered()), this, SLOT(OpenLocalVideo()));
	connect(ui.action_OpenNetStream, SIGNAL(triggered()), this, SLOT(OpenNetStreamDlg()));
	connect(ui.action_About, SIGNAL(triggered()), this, SLOT(PopAboutDlg()));
	connect(ui.action_GetSourceCode, SIGNAL(triggered()), this, SLOT(GetSourceCode()));

	//音量调节
	connect(ui.sliderVolume, SIGNAL(sliderReleased()), this, SLOT(volumeAdjust()));
	connect(ui.btnSetVolume, SIGNAL(clicked()), this, SLOT(setMute()));

	connect(&m_NetDlg, SIGNAL(PushStream(QString)), this, SLOT(slotPushStream(QString)));
}

void FFVideoPlayer::OpenLocalVideo()
{
	QString filename = QFileDialog::getOpenFileName(this, QString::fromLocal8Bit("选择视频文件"));
	if (filename.isEmpty())
	{
		return;
	}

	string videoPath = string((const char *)filename.toLocal8Bit()); //QString转string
	int n1 = videoPath.find_last_of('/');
	int n2 = videoPath.find_last_of('\0');
	string sname = videoPath.substr(n1 + 1, n2);
	QString qstr = QString(QString::fromLocal8Bit(sname.c_str())); //string转QString
	this->setWindowTitle(qstr);

	int totalMs = MyFFmpeg::Get()->Open(filename.toLocal8Bit());
	if (totalMs <= 0)
	{
		QMessageBox::information(this, "err", "file open failed!");
		return;
	}

	AudioPlay::Get()->sampleRate = MyFFmpeg::Get()->m_sampleRate;
	AudioPlay::Get()->channel = MyFFmpeg::Get()->m_channel;
	AudioPlay::Get()->sampleSize = 16;

	AudioPlay::Get()->Start();

	char buf[1024] = { 0 };
	int seconds = totalMs / 1000;
	m_Hour = seconds / 3600;
	m_Min = (seconds - m_Hour * 3600) / 60;
	m_Second = seconds - m_Hour * 3600 - m_Min * 60;

	g_isPlay = false;
	slotPlay();
}

/**
 * @brief 定时器事件
 * @param e
 */
void FFVideoPlayer::timerEvent(QTimerEvent *e)
{
	int playseconds = MyFFmpeg::Get()->m_pts / 1000;
	int hour = playseconds / 3600;
	int min = (playseconds - hour * 3600) / 60;
	int sec = playseconds - hour * 3600 - min * 60;
	char buf[1024] = { 0 };
	sprintf(buf, "%02d:%02d:%2d/%02d:%02d:%2d", hour, min, sec,m_Hour, m_Min, m_Second);
	this->ui.label_Playtime->setText(buf);  //当前播放时间

	if (MyFFmpeg::Get()->m_totalMs > 0)
	{
		float rate = MyFFmpeg::Get()->m_pts / (float(MyFFmpeg::Get()->m_totalMs));

		//只有按下了，才才显示进度条
		if (!ispressSlider)
		{
			this->ui.timeSlider->setValue(rate * 1000); //进度条
		}

		//        if(ui.timeSlider->geometry().contains(this->mapFromGlobal(QCursor::pos())))
		//        {
		//            ui.timeSlider->show();
		//            ui.timeSlider->setValue(rate * 1000); //进度条
		//        }
		//        else
		//        {
		//            ui.timeSlider->hide();
		//        }
	}
}

/**
 * @brief 响应进度条按下的信号
 */
void FFVideoPlayer::slotSliderPressed()
{
	ispressSlider = true;
}

/**
 * @brief 响应鼠标松开进度条的信号
 */
void FFVideoPlayer::soltSliderReleased()
{
	ispressSlider = false;
	float pos = 0;
	pos = this->ui.timeSlider->value() / static_cast<float>(ui.timeSlider->maximum() + 1); //从0开始的，不能让分母为0
	MyFFmpeg::Get()->Seek(pos);
}

/**
 * @brief 播放视频
 */
void FFVideoPlayer::slotPlay()
{
	g_isPlay = !g_isPlay;
	MyFFmpeg::Get()->m_isPlay = g_isPlay;

	if (g_isPlay)
	{
		setButtonBackImage(ui.btnPlayVideo, "./Resources/stop.png");
	}
	else
	{
		setButtonBackImage(ui.btnPlayVideo, "./Resources/play.png");
	}
}

/**
 * @brief 窗口关闭时的操作
 * @param e
 */
void FFVideoPlayer::closeEvent(QCloseEvent * e)
{
	//结束线程
	VideoThread::Get()->terminate();
}

//窗口大小发生变化时，各控件大小的位置及大小的设置
void FFVideoPlayer::resizeEvent(QResizeEvent *e)
{
	if (isFullScreen())
	{
		ui.openGLWidget->move(0, 0);
		ui.openGLWidget->resize(this->width(), this->height());
	}
}

/**
 * @brief FFVideoPlayer::on_fullScreenBtn_clicked
 *        全屏按钮
 */
void FFVideoPlayer::on_btnFullScreen_clicked()
{
	if (!isFullScreen())
	{
		fullShow();
	}
}

/**
 * @brief FFVideoPlayer::mouseDoubleClickEvent
 *        鼠标双击
 */
void FFVideoPlayer::mouseDoubleClickEvent(QMouseEvent*)
{
	if (this->isFullScreen())
	{
		normalShow();
	}
	else
	{
		fullShow();
	}
}

/**
 * @brief 全屏时各个按钮隐藏
 */
void FFVideoPlayer::fullShow()
{
	this->showFullScreen();

	ui.menuBar->hide();
	ui.timeSlider->hide();
	ui.label_Playtime->hide();
	ui.btnPlayVideo->hide();
	ui.btnSetVolume->hide();
	ui.sliderVolume->hide();
	ui.btnFullScreen->hide();
}

/**
 * @brief 正常显示时各个控件均显示
 */
void FFVideoPlayer::normalShow()
{
	this->showNormal();

	ui.menuBar->show();
	ui.timeSlider->show();
	ui.label_Playtime->show();
	ui.btnPlayVideo->show();
	ui.btnSetVolume->show();
	ui.sliderVolume->show();
	ui.btnFullScreen->show();
}

//菜单--打开网络流
void FFVideoPlayer::OpenNetStreamDlg()
{
	m_NetDlg.exec();
}

void FFVideoPlayer::PopAboutDlg()
{
	AboutDlg dlg;
	dlg.exec();
}

//调节音量
void FFVideoPlayer::volumeAdjust()
{
	MyFFmpeg::Get()->m_VolumeRatio = (ui.sliderVolume->value()) * 1.00 / 100;
}

//静音设置
void FFVideoPlayer::setMute()
{
	m_isMute = !m_isMute;
	if (m_isMute)
	{
		MyFFmpeg::Get()->m_VolumeRatio = 0;
		setButtonBackImage(ui.btnSetVolume, "./Resources/mute.png");
	}
	else
	{
		MyFFmpeg::Get()->m_VolumeRatio = 0.5;
		setButtonBackImage(ui.btnSetVolume, "./Resources/volume.png");
	}
}

//设置按钮图标，按钮的默认大小是 30*30
void FFVideoPlayer::setButtonBackImage(QPushButton *button, QString image)
{  
	QPixmap pixmap(image);
	QPixmap fitpixmap = pixmap.scaled(30, 30).scaled(30, 30, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	button->setIcon(QIcon(fitpixmap));
	button->setIconSize(QSize(30, 30));
	button->setFlat(true);  //按钮透明  
	button->setStyleSheet("border: 0px"); //消除边框
}

void FFVideoPlayer::slotPushStream(QString address)
{
	g_NetStream = 1;

	setWindowTitle(address);
	NetStreamThread::getInstance()->startPlay(address);
}

void FFVideoPlayer::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::RightButton)
	{
		QMenu menu;
		QAction *ac1 = menu.addAction(QString::fromLocal8Bit("打开本地视频"));
		QAction *ac2 = menu.addAction(QString::fromLocal8Bit("查看视频信息"));
		connect(ac1, SIGNAL(triggered()), this, SLOT(OpenLocalVideo()));
		menu.exec(QCursor::pos());
	}
}