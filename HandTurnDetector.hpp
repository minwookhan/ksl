#include <opencv2\opencv.hpp>

class HandTurnDetector {
public:
	// angleDegTh : 방향이 이 각도 이상 바뀌면 전환으로 본다 (도 단위)
	// speedRatioTh : 다음 속도가 이전 속도의 이 비율보다 작으면 감속으로 본다 (예: 0.8 -> 20% 이상 감소)
	// minSpeed : 이 값보다 느리면 방향이 의미 없다고 보고 스킵
	HandTurnDetector(float angleDegTh = 30.0f,
		float speedRatioTh = 0.8f,
		float minSpeed = 1e-3f)
		: angleRadTh_(angleDegTh* static_cast<float>(CV_PI) / 180.0f),
		speedRatioTh_(speedRatioTh),
		minSpeed_(minSpeed)
	{
		reset(); // 생성 시 내부 상태 초기화
	}

	// 내부 상태를 완전히 초기화 (새 시퀀스 시작용)
	void reset()
	{
		hasPrevPos_ = false;
		hasPrevVel_ = false;
		frameIdx_ = 0;

		prevPos_ = cv::Point2f(0.0f, 0.0f);
		prevVel_ = cv::Point2f(0.0f, 0.0f);
	}

	// (옵션) 파라미터를 런타임에 변경하고 싶을 때 사용
	void setParams(float angleDegTh, float speedRatioTh, float minSpeed)
	{
		angleRadTh_ = angleDegTh * static_cast<float>(CV_PI) / 180.0f;
		speedRatioTh_ = speedRatioTh;
		minSpeed_ = minSpeed;
	}

	// 실시간으로 매 프레임 호출
	// pos : 이번 프레임 손 위치
	// dt  : 이전 프레임과 시간 차이(초). 고정 fps면 1.0/fps 로 넣어도 됨.
	// 반환값 : 이번 프레임에서 "방향 전환 + 감속"이 동시에 발생했으면 true
	bool update(const cv::Point2f& pos, double dt)
	{
		bool detected = false;

		if (!hasPrevPos_) {
			prevPos_ = pos;
			hasPrevPos_ = true;
			frameIdx_++;
			return false;
		}

		// 위치 -> 속도 (dt가 0이면 그냥 위치 차이만 보자)
		cv::Point2f vel;
		if (dt > 1e-6) {
			vel = (pos - prevPos_) * (1.0f / static_cast<float>(dt));
		}
		else {
			vel = (pos - prevPos_);
		}

		float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);

		if (hasPrevVel_) {
			float prevSpeed = std::sqrt(prevVel_.x * prevVel_.x + prevVel_.y * prevVel_.y);

			// 너무 느릴 때는 방향 의미 X
			// if (speed > minSpeed_ && prevSpeed > minSpeed_)
			if (prevSpeed > minSpeed_)
			{
				// 단위벡터
				cv::Point2f u1 = prevVel_ * (1.0f / prevSpeed);
				cv::Point2f u2 = vel * (1.0f / (speed + 1e-6f)); // 0 나누기 방지용 약간의 epsilon

				// 내적 -> 각도
				float dot = u1.x * u2.x + u1.y * u2.y;
				dot = std::max(-1.0f, std::min(1.0f, dot));
				float dtheta = std::acos(dot); // rad

				// 속력 비
				float ratio = speed / (prevSpeed + 1e-6f);

				// 두 조건을 동시에 만족해야 "유효한 코너"로 본다
				if (dtheta > angleRadTh_ && ratio < speedRatioTh_) {
					detected = true;
					// 여기서 원하는 후처리를 하면 된다 (로그, 이벤트 발생, 세그먼트 끊기 등)
					// 예: std::cout << "Turn+Slow at frame " << frameIdx_ << std::endl;
				}
			}
		}

		// 다음 프레임을 위해 저장
		prevPos_ = pos;
		prevVel_ = vel;
		hasPrevVel_ = true;
		frameIdx_++;

		return detected;
	}

private:
	float angleRadTh_;
	float speedRatioTh_;
	float minSpeed_;

	cv::Point2f prevPos_;
	cv::Point2f prevVel_;
	bool hasPrevPos_;
	bool hasPrevVel_;

	int frameIdx_; // 디버깅용
};
