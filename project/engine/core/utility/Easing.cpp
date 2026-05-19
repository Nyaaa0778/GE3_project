#include "Easing.h"

#include <cmath>
#include <numbers>

using namespace std;
using namespace std::numbers;

/// <summary>
/// ほんの少しだけゆっくり始まり、なだらかに加速する
/// </summary>
float Easing::EaseInSine(float t) {
	return 1.0f - cosf((t * pi_v<float>) / 2.0f);
}

/// <summary>
/// なだらかに減速してフワッと止まる
/// </summary>
float Easing::EaseOutSine(float t) {
	return sinf((t * pi_v<float>) / 2.0f);
}

/// <summary>
/// スムーズに動き出し、スムーズに止まる
/// </summary>
float Easing::EaseInOutSine(float t) {
	return -(cosf(pi_v<float>*t) - 1.0f) / 2.0f;
}

/// <summary>
/// じわっと始まり、自然に加速する
/// </summary>
float Easing::EaseInQuad(float t) {
	return t * t;
}

/// <summary>
/// スッと動き出し、スッと止まる
/// </summary>
float Easing::EaseOutQuad(float t) {
	float temp = 1.0f - t;
	return 1.0f - temp * temp;
}

/// <summary>
/// なめらかに加速・減速する標準的な動き
/// </summary>
float Easing::EaseInOutQuad(float t) {
	if (t < 0.5f) {
		return 2.0f * t * t;
	} else {
		float temp = -2.0f * t + 2.0f;
		return 1.0f - (temp * temp) / 2.0f;
	}
}

/// <summary>
/// やや重たげに始まり、グッと加速する
/// </summary>
float Easing::EaseInCubic(float t) {
	return t * t * t;
}

/// <summary>
/// 勢いよく飛び出し、ピタッと止まる（Quadより少しキレを出したい時に）
/// </summary>
float Easing::EaseOutCubic(float t) {
	float temp = 1.0f - t;
	return 1.0f - temp * temp * temp;
}

/// <summary>
/// はっきりとした加速と減速
/// </summary>
float Easing::EaseInOutCubic(float t) {
	if (t < 0.5f) {
		return 4.0f * t * t * t;
	} else {
		float temp = -2.0f * t + 2.0f;
		return 1.0f - (temp * temp * temp) / 2.0f;
	}
}

/// <summary>
/// 最初はほとんど動かず、後半に急加速する
/// </summary>
float Easing::EaseInQuart(float t) {
	float t2 = t * t;
	return t2 * t2; // 2乗 × 2乗 ＝ 4乗！
}

/// <summary>
/// 非常に素早く飛び出し、残りの時間をかけてゆっくり目標に近づく
/// </summary>
float Easing::EaseOutQuart(float t) {
	float temp = 1.0f - t;
	float temp2 = temp * temp;
	return 1.0f - temp2 * temp2;
}

/// <summary>
/// ピュッと動いて、ピタッと止まるキレのある動き
/// </summary>
float Easing::EaseInOutQuart(float t) {
	if (t < 0.5f) {
		float t2 = t * t;
		return 8.0f * t2 * t2;
	} else {
		float temp = -2.0f * t + 2.0f;
		float temp2 = temp * temp;
		return 1.0f - (temp2 * temp2) / 2.0f;
	}
}

/// <summary>
/// 限界まで力を溜めてから一気に加速する
/// </summary>
float Easing::EaseInQuint(float t) {
	float t2 = t * t;
	return t2 * t2 * t; // 2乗 × 2乗 × t ＝ 5乗！
}

/// <summary>
/// 一瞬で移動し、最後は微調整するようにジワジワと止まる
/// </summary>
float Easing::EaseOutQuint(float t) {
	float temp = 1.0f - t;
	float temp2 = temp * temp;
	return 1.0f - temp2 * temp2 * temp;
}

/// <summary>
/// 瞬間移動に近いほどの鋭い加速と減速
/// </summary>
float Easing::EaseInOutQuint(float t) {
	if (t < 0.5f) {
		float t2 = t * t;
		return 16.0f * t2 * t2 * t;
	} else {
		float temp = -2.0f * t + 2.0f;
		float temp2 = temp * temp;
		return 1.0f - (temp2 * temp2 * temp) / 2.0f;
	}
}

/// <summary>
/// ギリギリまで全く動かず、最後にワープするように加速する
/// </summary>
float Easing::EaseInExpo(float t) {
	if (t == 0.0f) {
		return 0.0f;
	}
	return powf(2.0f, 10.0f * t - 10.0f);
}

/// <summary>
/// 瞬時に移動し、残りの時間をかけてジリジリと進む
/// </summary>
float Easing::EaseOutExpo(float t) {
	if (t == 1.0f) {
		return 1.0f;
	}
	return 1.0f - powf(2.0f, -10.0f * t);
}

/// <summary>
/// カチッ、カチッとスイッチが切り替わるような人工的で鋭い動き
/// </summary>
float Easing::EaseInOutExpo(float t) {
	if (t == 0.0f) {
		return 0.0f;
	}
	if (t == 1.0f) {
		return 1.0f;
	}

	if (t < 0.5f) {
		return powf(2.0f, 20.0f * t - 10.0f) / 2.0f;
	} else {
		return (2.0f - powf(2.0f, -20.0f * t + 10.0f)) / 2.0f;
	}
}

/// <summary>
/// 重いものを引っ張るように、最後にグンと加速する
/// </summary>
float Easing::EaseInCirc(float t) {
	return 1.0f - sqrtf(1.0f - t * t);
}

/// <summary>
/// 弾かれたように勢いよく飛び出し、最後は緩やかに止まる
/// </summary>
float Easing::EaseOutCirc(float t) {
	float temp = t - 1.0f;
	return sqrtf(1.0f - temp * temp);
}

/// <summary>
/// 伸び縮みするような、独特の粘りがある動き
/// </summary>
float Easing::EaseInOutCirc(float t) {
	if (t < 0.5f) {
		return (1.0f - sqrtf(1.0f - 4.0f * t * t)) / 2.0f;
	} else {
		float temp = -2.0f * t + 2.0f;
		return (sqrtf(1.0f - temp * temp) + 1.0f) / 2.0f;
	}
}

/// <summary>
/// 少し後ろに下がって（助走をつけて）から加速して飛んでいく
/// </summary>
float Easing::EaseInBack(float t) {
	const float c1 = 1.70158f;
	const float c3 = c1 + 1.0f;
	return c3 * t * t * t - c1 * t * t;
}

/// <summary>
/// 目標を少し通り過ぎてから、元の位置に戻って止まる
/// </summary>
float Easing::EaseOutBack(float t) {
	const float c1 = 1.70158f;
	const float c3 = c1 + 1.0f;
	float temp = t - 1.0f;
	return 1.0f + c3 * temp * temp * temp + c1 * temp * temp;
}

/// <summary>
/// 助走をつけて飛び出し、通り過ぎてから戻る
/// </summary>
float Easing::EaseInOutBack(float t) {
	const float c1 = 1.70158f;
	const float c2 = c1 * 1.525f;
	if (t < 0.5f) {
		float temp = 2.0f * t;
		return (temp * temp * ((c2 + 1.0f) * temp - c2)) / 2.0f;
	} else {
		float temp = 2.0f * t - 2.0f;
		return (temp * temp * ((c2 + 1.0f) * temp + c2) + 2.0f) / 2.0f;
	}
}

/// <summary>
/// ゴムを引っ張って放すように、震えながら後退してから一気に飛んでいく
/// </summary>
float Easing::EaseInElastic(float t) {
	if (t == 0.0f) {
		return 0.0f;
	}
	if (t == 1.0f) {
		return 1.0f;
	}

	const float c4 = (2.0f * pi_v<float>) / 3.0f;
	return -powf(2.0f, 10.0f * t - 10.0f) * sinf((t * 10.0f - 10.75f) * c4);
}

/// <summary>
/// 目標に到達した後、ゴムのようにビヨーンと数回バウンドして止まる
/// </summary>
float Easing::EaseOutElastic(float t) {
	if (t == 0.0f) {
		return 0.0f;
	}
	if (t == 1.0f) {
		return 1.0f;
	}

	const float c4 = (2.0f * pi_v<float>) / 3.0f;
	return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
}

/// <summary>
/// 震えながら飛び出し、ビヨーンと震えながら止まる
/// </summary>
float Easing::EaseInOutElastic(float t) {
	if (t == 0.0f) {
		return 0.0f;
	}
	if (t == 1.0f) {
		return 1.0f;
	}

	const float c5 = (2.0f * pi_v<float>) / 4.5f;
	if (t < 0.5f) {
		return -(powf(2.0f, 20.0f * t - 10.0f) * sinf((20.0f * t - 11.125f) * c5)) / 2.0f;
	} else {
		return (powf(2.0f, -20.0f * t + 10.0f) * sinf((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
	}
}

/// <summary>
/// 目標に落ちた後、ボールのようにポーン、ポンと跳ねて止まる
/// </summary>
float Easing::EaseOutBounce(float t) {
	const float n1 = 7.5625f;
	const float d1 = 2.75f;

	if (t < 1.0f / d1) {
		return n1 * t * t;
	} else if (t < 2.0f / d1) {
		float temp = t - 1.5f / d1;
		return n1 * temp * temp + 0.75f;
	} else if (t < 2.5f / d1) {
		float temp = t - 2.25f / d1;
		return n1 * temp * temp + 0.9375f;
	} else {
		float temp = t - 2.625f / d1;
		return n1 * temp * temp + 0.984375f;
	}
}

/// <summary>
/// 細かくバウンドしながら徐々に加速していく
/// </summary>
float Easing::EaseInBounce(float t) {
	return 1.0f - EaseOutBounce(1.0f - t);
}

/// <summary>
/// バウンドしながら始まり、バウンドしながら終わる
/// </summary>
float Easing::EaseInOutBounce(float t) {
	if (t < 0.5f) {
		return (1.0f - EaseOutBounce(1.0f - 2.0f * t)) / 2.0f;
	} else {
		return (1.0f + EaseOutBounce(2.0f * t - 1.0f)) / 2.0f;
	}
}