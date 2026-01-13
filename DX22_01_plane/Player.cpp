#include "Player.h"
#include "input.h"
#include "Game.h"
#include "Ground.h"
#include "Collision.h"
#include "AssimpPerse.h"
using namespace DirectX::SimpleMath;

Player::Player()
{
	m_Speed = 0.5f;
	m_Velocity = Vector3::Zero;
	m_IsGrounded = false;
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

	m_Position.x = 0.0f;
	m_Position.z = 0.0f;

	// 地面の情報を取得
	std::vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();

	float highestY = -9999.0f; // 見つかった地面の高さ
	bool foundGround = false;

	// 全グラウンドの全ポリゴンをチェックして、足元の高さを探す
	for (auto& g : grounds)
	{
		std::vector<VERTEX_3D> vecs = g->GetVertices();
		for (int i = 0; i < vecs.size(); i += 3)
		{
			Collision::Polygon poly = {
				vecs[i + 0].position,
				vecs[i + 1].position,
				vecs[i + 2].position
			};

			// 空から真下にレイ(線)を飛ばす
			Vector3 rayStart(m_Position.x, 1000.0f, m_Position.z);
			Vector3 rayEnd(m_Position.x, -1000.0f, m_Position.z);
			Collision::Segment ray = { rayStart, rayEnd };
			Vector3 hitPos;

			if (Collision::CheckHit(ray, poly, hitPos))
			{
				if (hitPos.y > highestY)
				{
					highestY = hitPos.y;
					foundGround = true;
				}
			}
		}
	}

	// 地面が見つかったら、そこに立つ
	if (foundGround)
	{
		m_Position.y = highestY;
	}
	else
	{
		m_Position.y = 50.0f; // 地面がない場合は仮の高さ
	}
}

void Player::Update()
{
	// 1フレーム前の位置を保存 (当たり判定用)
	Vector3 oldPos = m_Position;
	// ---------------------------------------------------------
	// 1. キー入力による移動
	// ---------------------------------------------------------
	Vector3 moveDir = Vector3::Zero;

	if (Input::GetKeyPress(VK_W))
	{
		moveDir.z += 1.0f;
	}
	if (Input::GetKeyPress(VK_S))
	{
		moveDir.z -= 1.0f;
	}
	if (Input::GetKeyPress(VK_A))
	{
		moveDir.x -= 1.0f;
	}
	if (Input::GetKeyPress(VK_D))
	{
		moveDir.x += 1.0f;
	}

	// 入力がある場合
	if (moveDir.LengthSquared() > 0)
	{
		moveDir.Normalize();

		// プレイヤーは入力で直接座標を動かす (操作性を良くするため)
		m_Position.x += moveDir.x * m_Speed;
		m_Position.z += moveDir.z * m_Speed;

		// 進行方向を向く
		m_Rotation.y = atan2(-moveDir.x, -moveDir.z);
	}
	//ジャンプ
	if (Input::GetKeyTrigger(VK_SPACE) && m_IsGrounded)
	{
		m_Velocity.y = 1.5f;   // ジャンプ力 (数値を変えると高さが変わります)
		m_IsGrounded = false;  // ジャンプしたので「空中」状態にする
	}
	m_IsGrounded = false;
	// ---------------------------------------------------------
	// 2. 重力処理 (Y軸)
	// ---------------------------------------------------------
	const float gravity = 0.05f; // 重力の強さ
	m_Velocity.y -= gravity;     // 下向きに加速
	if (m_Velocity.y < -1.0f)
	{
		m_Velocity.y = -1.0f;
	}
	// 落下速度を座標に加算
	m_Position.y += m_Velocity.y;

	// ---------------------------------------------------------
	// 3. 地面との当たり判定と押し戻し
	// ---------------------------------------------------------

		// Groundの頂点データを取得
	std::vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();

	// 全グラウンドの頂点を集める
	std::vector<VERTEX_3D> vertices;
	for (auto& g : grounds)
	{
		std::vector<VERTEX_3D> vecs = g->GetVertices();
		for (auto& v : vecs) vertices.emplace_back(v);
	}

	float playerRadius = 1.0f; // プレイヤーの半径
	Vector3 offset(0, playerRadius, 0); // ★追加: 足元からボールの中心までのズレ（オフセット）

	// すべての三角形ポリゴンと判定
	for (int i = 0; i < vertices.size(); i += 3)
	{
		Collision::Polygon poly = {
			vertices[i + 0].position,
			vertices[i + 1].position,
			vertices[i + 2].position
		};

		Vector3 hitPos;

		// ★変更点: ボールを足元(m_Position)ではなく、少し上(m_Position + offset)に作る
		// これで「ボールの底」が「足元」になります
		Vector3 sphereCenter = m_Position + offset;
		Vector3 oldSphereCenter = oldPos + offset;

		Collision::Segment moveSegment = { oldSphereCenter, sphereCenter };
		Collision::Sphere  playerSphere = { sphereCenter, playerRadius };

		// 球体としての押し戻しチェック
		if (Collision::CheckHit(playerSphere, poly, hitPos))
		{
			// 押し戻された「ボールの中心座標」を受け取る
			Vector3 pushBackCenter = Collision::moveSphere(playerSphere, poly, hitPos);

			Vector3 normal = Collision::GetNormal(poly); // 法線（地面から垂直に伸びる線）

			// ★ここが修正ポイント！
			// 本来の位置から、法線方向にほんの少し(0.01f)だけ浮かせてセットする
			Vector3 margin = normal * 0.03f;
			m_Position = (pushBackCenter - offset) + margin;

			if (normal.y > 0.5f)
			{
				m_Velocity.y = 0.0f;
				m_IsGrounded = true;
			}
		}
		//すり抜け防止チェック
		else if (Collision::CheckHit(moveSegment, poly, hitPos))
		{
			float dist = 0;
			Vector3 correctedCenter = Collision::moveSphere(moveSegment, playerRadius, poly, hitPos, dist);

			Vector3 normal = Collision::GetNormal(poly); // 法線を取得

			// ★ここも同じく修正！
			// ほんの少し浮かせることで、「次のフレームで重力がかかっても地面の下に行かない」ようにする
			Vector3 margin = normal * 0.03f;
			m_Position = (correctedCenter - offset) + margin;

			if (normal.y > 0.5f)
			{
				m_Velocity.y = 0.0f;
				m_IsGrounded = true;
			}
		}
	}

	// 奈落の底に落ちたらリスポーン (安全装置)
	if (m_Position.y < -100.0f)
	{
		Init(); // 初期化処理を呼んで再スポーン
		m_Velocity = Vector3::Zero;
	}
}

void Player::Draw(Camera* cam)
{
	//カメラを選択する
	cam->SetCamera();
	//カメラを追従させる
	cam->SetTarget(m_Position);//カメラのターゲットを更新

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
