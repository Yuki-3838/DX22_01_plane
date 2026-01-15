#include "Enemy.h"
#include "Game.h"
#include "Player.h"   // プレイヤーを探すために必要
#include "Collision.h" // 重力や当たり判定に必要
#include "Ground.h"
#include <iostream>
#include "input.h"
using namespace DirectX::SimpleMath;
using namespace std;

bool Enemy::m_EnableDebugDraw = false;
Enemy::Enemy()
{
	m_Speed = 0.1f;
	m_Velocity = Vector3::Zero;
	m_State = EnemyState::IDLE; // 最初は待機
	m_MaxHP = 100; // 最大HP
	m_HP = m_MaxHP; // 現在のHP
}

Enemy::~Enemy()
{
}

void Enemy::Init()
{
	// =================================================
	// 1. モデル読み込み
	// =================================================
	StaticMesh staticmesh;
	std::u8string modelFile = u8"assets/model/JACK式ピピ美_v1.0/JACK式ピピ美1.0.pmx";
	std::string texDirectory = "assets/model/JACK式ピピ美_v1.0";

	std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmpStr1, texDirectory);

	m_MeshRenderer.Init(staticmesh);
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	m_subsets = staticmesh.GetSubsets();
	m_Textures = staticmesh.GetTextures();
	std::vector<MATERIAL> materials = staticmesh.GetMaterials();

	for (int i = 0; i < materials.size(); i++)
	{
		std::unique_ptr<Material> m = std::make_unique<Material>();
		m->Create(materials[i]);
		m_Materials.push_back(std::move(m));
	}
	StaticMesh debugMesh;
	std::u8string debugModelFile = u8"assets/model/golfball/golf_ball.obj";
	std::string debugTexDir = "assets/model/golfball";

	std::string tmpStrDebug(reinterpret_cast<const char*>(debugModelFile.c_str()), debugModelFile.size());
	debugMesh.Load(tmpStrDebug, debugTexDir);
	m_DebugMesh.Init(debugMesh);

	m_DebugSubsets = debugMesh.GetSubsets();
	m_DebugTextures = debugMesh.GetTextures();
	std::vector<MATERIAL> debugMats = debugMesh.GetMaterials();
	for (int i = 0; i < debugMats.size(); i++)
	{
		std::unique_ptr<Material> m = std::make_unique<Material>();
		m->Create(debugMats[i]);
		m_DebugMaterials.push_back(std::move(m));
	}
	// =================================================
	// 2. 初期配置と「地面合わせ」処理
	// =================================================

	// 敵の出現させたい場所 (X, Z) を決める
	m_Position.x = 10.0f;
	m_Position.z = 10.0f;

	m_Scale = Vector3(1.5f, 1.5f, 1.5f);
	m_Rotation.y = 3.14f; // プレイヤーの方を向く

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

void Enemy::Update()
{
	if (m_State == EnemyState::DEAD)
	{
		// ここで死亡アニメーションなどを再生するが、
		// とりあえず今は「奈落の下に飛ばす」などで消えたことにする
		m_Position.y = -500.0f;
		return;
	}
	// 1フレーム前の位置を保存
	Vector3 oldPos = m_Position;

	// ジャンプや地面フラグのリセットなど（コピーしたままでOK）
	m_IsGrounded = false;

	// ---------------------------------------------------------
	// 1. AIによる移動決定
	// ---------------------------------------------------------

	// ゲーム内からプレイヤーを探す
	std::vector<Player*> players = Game::GetInstance()->GetObjects<Player>();

	Vector3 moveDir = Vector3::Zero;

	// プレイヤーが見つかったら追いかける
	if (players.size() > 0)
	{
		// 最初のプレイヤーをターゲットにする
		Player* target = players[0];
		Vector3 targetPos = target->GetPosition();

		// 自分からターゲットへのベクトル（矢印）を計算
		Vector3 diff = targetPos - m_Position;

		// 距離を測る
		float distance = diff.Length();

		// 「遠すぎず、近すぎない」時だけ動く (例: 20m以内、かつ2m以上離れている)
		if (distance < 100.0f && distance > 2.0f)
		{
			diff.y = 0.0f; // 高低差は無視して水平方向だけ見る
			diff.Normalize(); // 長さを1にする
			moveDir = diff;   // 移動方向にセット
		}
	}

	// ---------------------------------------------------------
	// 移動と回転の反映
	// ---------------------------------------------------------
	if (moveDir.LengthSquared() > 0)
	{
		// 敵の移動速度 (プレイヤーより少し遅くするとゲームっぽい)
		float enemySpeed = 0.05f;

		m_Position.x += moveDir.x * enemySpeed;
		m_Position.z += moveDir.z * enemySpeed;

		// 進行方向を向く
		// モデルの向きが逆なら、atan2の引数にマイナスをつけるか + XM_PI する
		m_Rotation.y = atan2(moveDir.x, moveDir.z);
	}

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
	Vector3 offset(0, playerRadius, 0);

	// すべての三角形ポリゴンと判定
	for (int i = 0; i < vertices.size(); i += 3)
	{
		Collision::Polygon poly = {
			vertices[i + 0].position,
			vertices[i + 1].position,
			vertices[i + 2].position
		};

		Vector3 hitPos;
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
	if (Input::GetKeyTrigger('V'))
	{
		ToggleDebugMode(); // 関数を呼び出してオンオフ切り替え
	}
}

void Enemy::Draw(Camera* cam)
{
	cam->SetCamera();

	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx = s * r * t;
	Renderer::SetWorldMatrix(&worldmtx);

	m_Shader.SetGPU();
	m_MeshRenderer.BeforeDraw();

	for (int i = 0; i < m_subsets.size(); i++)
	{
		m_Materials[m_subsets[i].MaterialIdx]->SetGPU();
		if (m_Materials[m_subsets[i].MaterialIdx]->isTextureEnable())
		{
			m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
		}
		m_MeshRenderer.DrawSubset(
			m_subsets[i].IndexNum,
			m_subsets[i].IndexBase,
			m_subsets[i].VertexBase);
	}
	// =================================================
	// ★追加: デバッグ描画（オンの時だけ実行）
	// =================================================
	if (m_EnableDebugDraw)
	{

		// 2. 当たり判定ボックスの中心位置を計算
		// Updateで設定した BoundingBox.Center と同じ計算式にします
		Vector3 centerPos = m_Position + Vector3(0.0f, 4.0f, 0.0f);

		Matrix r = Matrix::Identity; // 回転なし（AABBの場合）
		Matrix t = Matrix::CreateTranslation(centerPos);

		// 3. スケール（大きさ）の計算
		// ボックスのサイズ：幅1.5m, 高さ3.0m, 奥行1.5m
		// ゴルフボールの元の大きさに合わせて調整が必要です。
		// プレイヤーの時に「10.0f」で半径1.5mだったので、それを基準に比率計算します。

		// 半径1.5m = スケール10.0 の場合、
		// 幅0.75(半分) = スケール5.0
		// 高さ1.5 (半分) = スケール10.0

		// ボックス型に見せるため、XYZそれぞれの倍率を変えます
		float sx = 10.0f;
		float sy = 25.0f;
		float sz = 10.0f;

		Matrix s = Matrix::CreateScale(sx, sy, sz);

		Matrix worldmtx = s * r * t;
		Renderer::SetWorldMatrix(&worldmtx);

		m_DebugMesh.BeforeDraw();

		for (int i = 0; i < m_DebugSubsets.size(); i++)
		{
			m_DebugMaterials[m_DebugSubsets[i].MaterialIdx]->SetGPU();
			if (m_DebugMaterials[m_DebugSubsets[i].MaterialIdx]->isTextureEnable())
			{
				m_DebugTextures[m_DebugSubsets[i].MaterialIdx]->SetGPU();
			}
			m_DebugMesh.DrawSubset(
				m_DebugSubsets[i].IndexNum,
				m_DebugSubsets[i].IndexBase,
				m_DebugSubsets[i].VertexBase);
		}
	}
}

void Enemy::Uninit()
{
}

// ダメージを受けたときの処理
void Enemy::OnDamage(int amount)
{
	// すでに死んでいたら無視
	if (m_State == EnemyState::DEAD) return;

	// HPを減らす
	m_HP -= amount;

	// デバッグ出力（出力ウィンドウで確認用）
	std::cout << "ダメージを食らった" << std::endl;

	// HPが0以下になったら死亡状態へ
	if (m_HP <= 0)
	{
		m_HP = 0;
		m_State = EnemyState::DEAD;
	}
}

void Enemy::ToggleDebugMode()
{
	m_EnableDebugDraw = !m_EnableDebugDraw;
}
