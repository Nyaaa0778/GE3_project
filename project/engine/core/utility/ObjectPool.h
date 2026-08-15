#pragma once

#include <vector>
#include <cassert>

template <typename T>
class ObjectPool {
public:
    ObjectPool() = default;
    
    explicit ObjectPool(size_t capacity) {
        Initialize(capacity);
    }

    ~ObjectPool() = default;

    /// <summary>
    /// プールの初期化
    /// </summary>
    /// <param name="capacity">プールするオブジェクトの最大数</param>
    void Initialize(size_t capacity) {
        pool_.resize(capacity);
        freeList_.clear();
        freeList_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            freeList_.push_back(&pool_[i]);
        }
    }

    /// <summary>
    /// 未使用のオブジェクトを1つ借りる
    /// </summary>
    /// <returns>オブジェクトへのポインタ（空きがない場合はnullptr）</returns>
    T* Acquire() {
        if (freeList_.empty()) {
            return nullptr;
        }
        T* obj = freeList_.back();
        freeList_.pop_back();
        return obj;
    }

    /// <summary>
    /// 使い終わったオブジェクトをプールに戻す
    /// </summary>
    /// <param name="obj">戻すオブジェクトへのポインタ</param>
    void Release(T* obj) {
        if (obj == nullptr) return;

        // バリデーション: オブジェクトがこのプールに属しているかアドレスの範囲を確認
        assert(obj >= &pool_.front() && obj <= &pool_.back() && "Released object does not belong to this pool.");

#ifdef _DEBUG
        // 二重返却チェック (デバッグビルド時のみ)
        for (const auto& freeObj : freeList_) {
            assert(freeObj != obj && "Object double release detected.");
        }
#endif

        freeList_.push_back(obj);
    }

    /// <summary>
    /// 現在の空きオブジェクト数を取得
    /// </summary>
    size_t GetFreeCount() const { return freeList_.size(); }

    /// <summary>
    /// プールの最大容量を取得
    /// </summary>
    size_t GetCapacity() const { return pool_.size(); }

private:
    std::vector<T> pool_;          // 実体を連続したメモリに確保
    std::vector<T*> freeList_;     // 空いているオブジェクトへのポインタ配列
};
