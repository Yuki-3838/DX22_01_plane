#include "Player.h"
#include "input.h"
using namespace DirectX::SimpleMath;

Player::Player()
{
	m_Speed = 0.5f;
}

Player::~Player()
{
}

void Player::Init()
{
	// メッシュ読み込み
	StaticMesh staticmesh;

	//3Dモデルデータ
	//std::u8string modelFile = u8"assets/model/cylinder/cylinder.obj";
	std::u8string modelFile = u8"assets/model/JACK式ピピ美_v1.0/JACK式ピピ美1.0.pmx";
	//テクスチャディレクトリ
	//std::string texDirectory = "assets/model/cylinder";
	std::string texDirectory = "assets/model/JACK式ピピ美_v1.0";

	//Meshを読み込む
	std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmpStr1, texDirectory);

	m_MeshRenderer.Init(staticmesh);

	// シェーダオブジェクト生成
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	// サブセット情報取得
	m_subsets = staticmesh.GetSubsets();

	// テクスチャ情報取得
	m_Textures = staticmesh.GetTextures();

	// マテリアル情報取得	
	std::vector<MATERIAL> materials = staticmesh.GetMaterials();

	// マテリアル数分ループ
	for (int i = 0; i < materials.size(); i++)
	{
		// マテリアルオブジェクト生成
		std::unique_ptr<Material> m = std::make_unique<Material>();

		// マテリアル情報をセット
		m->Create(materials[i]);

		// マテリアルオブジェクトを配列に追加
		m_Materiales.push_back(std::move(m));
	}

	//大きさを設定
	m_Scale.x = 1;
	m_Scale.y = 1;
	m_Scale.z = 1;
}

void Player::Update()
{
	// ---------------------------------------------------------
	// キー入力による移動処理
	// ---------------------------------------------------------
	Vector3 moveDir = Vector3::Zero;

	// 前後 (W / S, または矢印キー)
	if (Input::GetKeyPress(VK_W) || Input::GetKeyPress(VK_UP))
	{
		moveDir.z += 1.0f; // 奥へ
	}
	if (Input::GetKeyPress(VK_S) || Input::GetKeyPress(VK_DOWN))
	{
		moveDir.z -= 1.0f; // 手前へ
	}

	// 左右 (A / D, または矢印キー)
	if (Input::GetKeyPress(VK_A) || Input::GetKeyPress(VK_LEFT))
	{
		moveDir.x -= 1.0f; // 左へ
	}
	if (Input::GetKeyPress(VK_D) || Input::GetKeyPress(VK_RIGHT))
	{
		moveDir.x += 1.0f; // 右へ
	}

	// 移動入力がある場合のみ処理
	if (moveDir.LengthSquared() > 0)
	{
		// 斜め移動でも速度が変わらないように正規化
		moveDir.Normalize();

		// 座標を更新
		m_Position += moveDir * m_Speed;

		// 進行方向を向く (Y軸回転)
		// atan2(x, z) で移動方向の角度を計算
		m_Rotation.y = atan2(moveDir.x, moveDir.z);
	}
}

void Player::Draw(Camera* cam)
{
	//カメラを選択する
	cam->SetCamera();

	// SRT情報作成
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx;
	worldmtx = s * r * t;
	Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

	m_Shader.SetGPU();

	// インデックスバッファ・頂点バッファをセット
	m_MeshRenderer.BeforeDraw();

	//マテリアル数分ループ 
	for (int i = 0; i < m_subsets.size(); i++)
	{
		// マテリアルをセット(サブセット情報の中にあるマテリアルインデックを使用)
		m_Materiales[m_subsets[i].MaterialIdx]->SetGPU();

		if (m_Materiales[m_subsets[i].MaterialIdx]->isTextureEnable())
		{
			m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
		}

		m_MeshRenderer.DrawSubset(
			m_subsets[i].IndexNum,    // 描画するインデックス数
			m_subsets[i].IndexBase,   // 最初のインデックスバッファの位置
			m_subsets[i].VertexBase); // 頂点バッファの最初から使用
	}
}

void Player::Uninit()
{
}
