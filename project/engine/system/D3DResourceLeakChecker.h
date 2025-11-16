#pragma once
#include<wrl.h>

class D3DResourceLeakChecker {
public:
  /// <summary>
  /// デストラクタ
  /// </summary>
  ~D3DResourceLeakChecker();

private:
  // namespace
  template <class InterfaceType>
  using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;
};
