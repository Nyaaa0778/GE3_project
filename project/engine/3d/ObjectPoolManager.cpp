#include "ObjectPoolManager.h"

template<class T>
inline void ObjectPoolManager<T>::Create(int capacity) {
	pool_ = new T[capacity];
	// 同時に存在させられるオブジェクトの最大数を設定
	capacity_ = capacity;
}

template<class T>
void ObjectPoolManager<T>::Destroy() {
	delete[] pool_;
}

template<class T>
T* ObjectPoolManager<T>::Recycle() {
	for (int i = 0; i < capacity_; i++) {
		if (pool_[i].exists == false) {
			// 生存フラグを立てる
			pool_[i].exists = true;

			// 未使用なので再利用
			return &pool_[i];
		}
	}

	// 未使用なオブジェクトが見つからなかった
	return nullptr;
}

template<class T>
void ObjectPoolManager<T>::Update() {
	for (int i = 0; i < capacity_; i++) {
		if (pool_[i].exists) {
			// 生存しているオブジェクトのみを更新
			pool_[i].Update();
		}
	}
}

template<class T>
void ObjectPoolManager<T>::Draw() {
	for (int i = 0; i < capacity_; i++) {
		if (pool_[i].exists) {
			// 生存しているオブジェクトのみを描画
			pool_[i].Draw();
		}
	}
}

template<class T>
int ObjectPoolManager<T>::Count() {
	int count = 0;
	
	for (int i = 0; i < capacity_; i++) {
		if (pool_[i].exists) {
			// 生存している
			count++;
		}
	}

	return count;
}
