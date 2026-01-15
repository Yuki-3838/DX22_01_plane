#pragma once
#include "Object.h"
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "utility.h"
#include "Material.h"
class Player :public Object
{
private:

	// 描画の為の情報（メッシュに関わる情報）
	MeshRenderer m_MeshRenderer; // 頂点バッファ・インデックスバッファ・インデックス数
	MeshRenderer m_DebugMesh; // デバッグ用のメッシュ
	std::vector<std::unique_ptr<Material>> m_DebugMaterials;
	std::vector<SUBSET> m_DebugSubsets;
	std::vector<std::unique_ptr<Texture>> m_DebugTextures;
	// 描画の為の情報（見た目に関わる部分）
	std::vector<std::unique_ptr<Material>> m_Materiales;
	std::vector<SUBSET> m_subsets;
	std::vector<std::unique_ptr<Texture>> m_Textures; // テクスチャ
	//地面に着いているかのフラグ
	bool m_IsGrounded;

	//重力・落下計算用の速度ベクトル
	DirectX::SimpleMath::Vector3 m_Velocity;
	float m_Speed; // 移動速度
	DirectX::SimpleMath::Vector3 m_DebugAttackPos;
	int m_AttackFrame;
	bool m_IsAttackActive;
public:
	Player();
	~Player() override;
	void Init();
	void Update();
	void Draw(Camera* cam);
	void Uninit();
};

