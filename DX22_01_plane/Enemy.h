#pragma once
#include "Object.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "Material.h"

// 敵の状態（AI）
enum class EnemyState
{
	IDLE,   // 待機（様子見）
	CHASE,  // 追跡（追いかける）
	ATTACK, // 攻撃
	DEAD    // 死亡
};

class Enemy : public Object
{
private:
	// 描画関連
	MeshRenderer m_MeshRenderer;
	std::vector<std::unique_ptr<Material>> m_Materials;
	std::vector<SUBSET> m_subsets;
	std::vector<std::unique_ptr<Texture>> m_Textures;
	//地面に着いているかのフラグ
	bool m_IsGrounded;

	// パラメータ
	float m_Speed;      // 移動速度
	EnemyState m_State; // 現在の状態
	int m_HP;
	int m_MaxHP;

	// 重力・物理用
	DirectX::SimpleMath::Vector3 m_Velocity;

public:
	Enemy();
	~Enemy() override;

	void Init() override;
	void Update() override;
	void Draw(Camera* cam) override;
	void Uninit() override;
	void OnDamage(int amount);
};