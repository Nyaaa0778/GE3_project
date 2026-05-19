#pragma once

namespace Easing {

	/// <summary>
	/// ほんの少しだけゆっくり始まり、なだらかに加速する
	/// </summary>
	float EaseInSine(float t);

	/// <summary>
	/// なだらかに減速してフワッと止まる
	/// </summary>
	float EaseOutSine(float t);

	/// <summary>
	/// スムーズに動き出し、スムーズに止まる
	/// </summary>
	float EaseInOutSine(float t);

	/// <summary>
	/// じわっと始まり、自然に加速する
	/// </summary>
	float EaseInQuad(float t);

	/// <summary>
	/// スッと動き出し、スッと止まる
	/// </summary>
	float EaseOutQuad(float t);

	/// <summary>
	/// なめらかに加速・減速する標準的な動き
	/// </summary>
	float EaseInOutQuad(float t);

	/// <summary>
	/// やや重たげに始まり、グッと加速する
	/// </summary>
	float EaseInCubic(float t);

	/// <summary>
	/// 勢いよく飛び出し、ピタッと止まる（Quadより少しキレを出したい時に）
	/// </summary>
	float EaseOutCubic(float t);

	/// <summary>
	/// はっきりとした加速と減速
	/// </summary>
	float EaseInOutCubic(float t);

	/// <summary>
	/// 最初はほとんど動かず、後半に急加速する
	/// </summary>
	float EaseInQuart(float t);

	/// <summary>
	/// 非常に素早く飛び出し、残りの時間をかけてゆっくり目標に近づく
	/// </summary>
	float EaseOutQuart(float t);

	/// <summary>
	/// ピュッと動いて、ピタッと止まるキレのある動き
	/// </summary>
	float EaseInOutQuart(float t);

	/// <summary>
	/// 限界まで力を溜めてから一気に加速する
	/// </summary>
	float EaseInQuint(float t);

	/// <summary>
	/// 一瞬で移動し、最後は微調整するようにジワジワと止まる
	/// </summary>
	float EaseOutQuint(float t);

	/// <summary>
	/// 瞬間移動に近いほどの鋭い加速と減速
	/// </summary>
	float EaseInOutQuint(float t);

	/// <summary>
	/// ギリギリまで全く動かず、最後にワープするように加速する
	/// </summary>
	float EaseInExpo(float t);

	/// <summary>
	/// 瞬時に移動し、残りの時間をかけてジリジリと進む
	/// </summary>
	float EaseOutExpo(float t);

	/// <summary>
	/// カチッ、カチッとスイッチが切り替わるような人工的で鋭い動き
	/// </summary>
	float EaseInOutExpo(float t);

	/// <summary>
	/// 重いものを引っ張るように、最後にグンと加速する
	/// </summary>
	float EaseInCirc(float t);

	/// <summary>
	/// 弾かれたように勢いよく飛び出し、最後は緩やかに止まる
	/// </summary>
	float EaseOutCirc(float t);

	/// <summary>
	/// 伸び縮みするような、独特の粘りがある動き
	/// </summary>
	float EaseInOutCirc(float t);

	/// <summary>
	/// 少し後ろに下がって（助走をつけて）から加速して飛んでいく
	/// </summary>
	float EaseInBack(float t);

	/// <summary>
	/// 目標を少し通り過ぎてから、元の位置に戻って止まる
	/// </summary>
	float EaseOutBack(float t);

	/// <summary>
	/// 助走をつけて飛び出し、通り過ぎてから戻る
	/// </summary>
	float EaseInOutBack(float t);

	/// <summary>
	/// ゴムを引っ張って放すように、震えながら後退してから一気に飛んでいく
	/// </summary>
	float EaseInElastic(float t);

	/// <summary>
	/// 目標に到達した後、ゴムのようにビヨーンと数回バウンドして止まる
	/// </summary>
	float EaseOutElastic(float t);

	/// <summary>
	/// 震えながら飛び出し、ビヨーンと震えながら止まる
	/// </summary>
	float EaseInOutElastic(float t);

	/// <summary>
	/// 細かくバウンドしながら徐々に加速していく
	/// </summary>
	float EaseInBounce(float t);

	/// <summary>
	/// 目標に落ちた後、ボールのようにポーン、ポンと跳ねて止まる
	/// </summary>
	float EaseOutBounce(float t);

	/// <summary>
	/// バウンドしながら始まり、バウンドしながら終わる
	/// </summary>
	float EaseInOutBounce(float t);
};