/*

基于FFmpeg的视频播放器

开发环境
	Qt5.12.4 64位
	FFmpeg4.2.1 Win64
	VS2017
	Win10-64
	unicode编码

*/

#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_FFVideoPlayer.h"
#include <QMediaPlayer>
#include "NetStreamDlg.h"

class FFVideoPlayer : public QMainWindow
{
	Q_OBJECT

public:
	FFVideoPlayer(QWidget *parent = Q_NULLPTR);

	void timerEvent(QTimerEvent *e);
	void closeEvent(QCloseEvent * e);
	void resizeEvent(QResizeEvent *e);  //窗口发生变化时
	void mouseDoubleClickEvent(QMouseEvent*);  //双击全屏
	void mousePressEvent(QMouseEvent *event);

private:
	void fullShow();
	void normalShow();
	void initUI();
	void allConnect();
	void setButtonBackImage(QPushButton *button, QString image);

private slots:
	void slotPlay(); //播放与暂停
	void slotSliderPressed();
	void soltSliderReleased();
	void on_btnFullScreen_clicked();
	void setMute();  //静音设置

	//菜单
	void OpenLocalVideo();
	void OpenNetStreamDlg();
	void PopAboutDlg();

	//音量调节
	void volumeAdjust();

	//网络流
	void slotPushStream(QString address);

private:
	Ui::FFVideoPlayerClass ui;
	QMediaPlayer    *player;

	//视频总时间
	int m_Hour;
	int m_Min;
	int m_Second;

	NetStreamDlg m_NetDlg;
	bool m_isMute;   //是否静音
};
