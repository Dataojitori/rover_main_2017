#pragma once
#include <time.h>
#include <list>
#include <opencv2/opencv.hpp>
#include <opencv/cvaux.h>
#include <opencv/highgui.h>
#include "task.h"
#include "utils.h"

//テスト用状態
class Testing : public TaskBase
{
protected:
	virtual bool onInit(const struct timespec& time);
	virtual bool onCommand(const std::vector<std::string> args);

	//次の状態に移行
	void nextState();

public:
	Testing();
	~Testing();
};

//筒の中に入っている状態
class Waiting : public TaskBase
{
	struct timespec mStartTime;//状態開始時刻
	unsigned int mContinuousLightCount;//光っていることが検知された回数
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onUpdate(const struct timespec& time);

	//次の状態に移行
	void nextState();

public:
	Waiting();
	~Waiting();
};

//落下している状態
class Falling : public TaskBase
{
private:
	struct timespec mStartTime;//状態開始時刻
	struct timespec mLastCheckTime;//前回のチェック時刻
	int mLastPressure;//前回の気圧
	unsigned long long mLastMotorPulseL,mLastMotorPulseR;//前回チェック時のモーター回転数
	unsigned int mContinuousPressureCount;//気圧が閾値以下の状態が続いた回数
	unsigned int mCoutinuousGyroCount;//角速度が閾値以下の状態が続いた回数
	unsigned int mContinuousMotorPulseCount;//モータ回転数が閾値以上の状態が続いた回数
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onUpdate(const struct timespec& time);

	//次の状態に移行
	void nextState();
public:
	Falling();
	~Falling();
};

//パラ分離状態(サーボを動かしてパラを切り離す)
class Separating : public TaskBase
{
private:
	struct timespec mLastUpdateTime;//前回サーボの向きを更新した時間
	bool mCurServoState;			//現在のサーボの向き(true = 1,false = 0)
	unsigned int mServoCount;		//サーボの向きを変更した回数
	enum STEP{STEP_SEPARATE = 0, STEP_PRE_PARA_JUDGE,STEP_PARA_JUDGE,STEP_PARA_DODGE,STEP_GO_FORWARD};
	enum STEP mCurStep;

protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onUpdate(const struct timespec& time);

	//次の状態に移行
	void nextState();
public:
	Separating();
	~Separating();
};

//ゴールへの移動中
class Navigating : public TaskBase
{
private:
	struct timespec mLastNaviMoveCheckTime;	 //前回のGPSによるスタック判定とナビゲーション処理のチェック時刻
	struct timespec mLastEncoderCheckTime;	 //前回のエンコーダチェック時刻
	struct timespec mEscapingRandomStartTime;//EscapingRandomの開始時刻

	//ゴール位置
	VECTOR3 mGoalPos;
	bool mIsGoalPos;

	//GPS座標から計算された過去数回分の位置
	std::list<VECTOR3> mLastPos;

	//前回のパルス数
	unsigned long long mPrevDeltaPulseL, mPrevDeltaPulseR;

protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onUpdate(const struct timespec& time);
	virtual bool onCommand(const std::vector<std::string> args);

	void navigationMove(double distance) const; //通常時の移動処理
	bool isStuckByGPS() const;//スタック判定(GPS)
	void chechStuckByEncoder(const struct timespec& time, VECTOR3 currentPos);//スタック判定チェック(エンコーダ)
	bool removeError();//異常値の除去

	//次の状態に移行
	void nextState();
public:
	void setGoal(const VECTOR3& pos);

	Navigating();
	~Navigating();
};

/* ここから　2014年6月オープンラボ前に実装 */
class ColorAccessing : public TaskBase
{
	const static double DEACCELERATE_DURATION = 0.5;
	const static int DETECTING_MAX_RETRY_COUNT = 5;			//ToDo: テストが完了したらconstants.hに移動する
	struct timespec mLastUpdateTime;//前回のチェック時刻
	struct timespec mStartTime;		//状態開始時刻
	
	enum STEP{STEP_STARTING, STEP_TURNING, STEP_STOPPING_FAST, STEP_STOPPING_LONG, STEP_CHECKING, STEP_DEACCELERATE, STEP_GO_BACK, STEP_CHANGE_OF_DIRECTION, STEP_LEAVING};
	enum STEP mCurStep;
    double mAngleOnBegin;
    bool mIsLastActionStraight;
	bool mIsGPS;					//Detectingで一度でもGPS座標を取得できている場合はtrue
	VECTOR3 mCurrentPos;				//最新の座標を保持
	bool mIsDetectingExecute;//falseならdetectingは実施せずGPSですぐにゴール判定する(2nd flight 高速度賞狙い)
    int mTryCount;
	int mDetectingRetryCount;		//一定時間経過してナビからやり直した回数
	int mMotorPower;
	int mCurrentMotorPower;
	int actCount;
	double mStraightTime;

	unsigned long long gDeltaPulseL;
	unsigned long long gDeltaPulseR;
	unsigned long long gThresholdHigh;
	unsigned long long gThresholdLow;
	unsigned long long gStraightThresholdHigh;
	unsigned long long gStraightThresholdLow;
	unsigned long long gRotationThresholdHigh;
	unsigned long long gRotationThresholdLow;
	unsigned long long gCurveThresholdHigh;
	unsigned long long gCurveThresholdLow;
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onUpdate(const struct timespec& time);
	virtual bool onCommand(const std::vector<std::string> args);

	void nextState();	//次の状態に移行
	void prevState();	//前の状態に移行

	void setMotorPower(std::string str);
	
	//ColorAccessingを開始してからの経過時間を確認
	//一定時間以上経過している場合はしばらく直進して距離を取った後Navigatingからやり直す
	//一定回数以上ナビ復帰を繰り返した場合はfalseを返す
	bool timeCheck(const struct timespec& time);

public:
	ColorAccessing();
	~ColorAccessing();
	
	//detectingを実施するかどうかを設定する(true:実施する false:実施せず)
	//基本的に呼ぶ必要はない
	//detectingをOFFにする場合はinitialize.txtに"detecting setmode OFF"を記載する
	void setIsDetectingExecute(bool flag);
	bool getIsDetectingExecute();
};
/* ここまで　2014年6月オープンラボ前に実装 */

extern Testing gTestingState;
extern Waiting gWaitingState;
extern Falling gFallingState;
extern Separating gSeparatingState;
extern Navigating gNavigatingState;
extern ColorAccessing gColorAccessing;