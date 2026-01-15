#pragma once
#include "Object.h"
#include "Texture2D.h"

class HPBar : public Object
{
private:
    Texture2D* m_BackBar;  // 背景（暗い色）
    Texture2D* m_FrontBar; // 中身（緑や赤）

    float m_MaxHP;
    float m_CurrentHP;

public:
    HPBar();
    ~HPBar() override;

    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;


    // 色設定
    void SetColor(DirectX::SimpleMath::Color color);

    // HP更新
    void SetHP(float current, float max);


};