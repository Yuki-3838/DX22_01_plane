#include "HPBar.h"

using namespace DirectX::SimpleMath;

HPBar::HPBar()
{
    m_BackBar = new Texture2D();
    m_FrontBar = new Texture2D();
    m_MaxHP = 100.0f;
    m_CurrentHP = 100.0f;
}

HPBar::~HPBar()
{
    delete m_BackBar;
    delete m_FrontBar;
}

void HPBar::Init()
{
    // 背景用 (グレー)
    m_BackBar->Init();
    m_BackBar->SetTexture("assets/texture/white.png"); // 白画像
    m_BackBar->SetColor(Color(0.2f, 0.2f, 0.2f, 1.0f)); // 暗い色

    // 中身用 (白 -> あとでSetColorで変更)
    m_FrontBar->Init();
    m_FrontBar->SetTexture("assets/texture/white.png");
    m_FrontBar->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
}

void HPBar::Update()
{
    // HP割合の計算
    float percent = m_CurrentHP / m_MaxHP;
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 1.0f) percent = 1.0f;

    // ■重要: 左端基準に見せる計算

    // 1. 中身の横幅を割合で縮める
    Vector3 frontScale = m_Scale;
    frontScale.x *= percent;
    m_FrontBar->SetScale(frontScale);

    // 2. 背景はそのままのサイズ
    m_BackBar->SetScale(m_Scale);

    // 3. 位置の調整 (ここがミソ！)
    // Texture2Dは中心基準なので、短くなると両側が縮んでしまう。
    // 「縮んだ分の半分」だけ左に移動させることで、左端が止まっているように見せる。
    float diff = (m_Scale.x - frontScale.x) / 2.0f;
    Vector3 frontPos = m_Position;
    frontPos.x -= diff; // 左にズラす

    m_FrontBar->SetPosition(frontPos);
    m_BackBar->SetPosition(m_Position); // 背景はそのまま

    m_BackBar->Update();
    m_FrontBar->Update();
}

void HPBar::Draw(Camera* cam)
{
    // 背景を描いてから、中身を描く
    m_BackBar->Draw(cam);
    m_FrontBar->Draw(cam);
}

void HPBar::Uninit()
{
    m_BackBar->Uninit();
    m_FrontBar->Uninit();
}


void HPBar::SetColor(Color color)
{
    m_FrontBar->SetColor(color);
}

void HPBar::SetHP(float current, float max)
{
    m_CurrentHP = current;
    m_MaxHP = max;
}

