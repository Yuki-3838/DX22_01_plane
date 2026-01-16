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
	m_AttackTimer = 0;
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
	m_HpBar = new HPBar();
	m_HpBar->Init();
	m_HpBar->SetColor(Color(0.0f, 1.0f, 0.0f, 1.0f)); // 緑 (敵なら赤)
	m_HpBar->SetPosition(0.0f, -300.0f, 0.0f);      // 左上 (敵なら画面下)
	m_HpBar->SetScale(600.0f, 30.0f, 1.0f);           // サイズ
}

void Enemy::Update()
{
	// ---------------------------------------------------------
		// 0. 死亡チェック & 初期設定
		// ---------------------------------------------------------
	if (m_State == EnemyState::DEAD)
	{
		m_Position.y = -500.0f; // 簡易的な消滅処理
		return;
	}

	// 1フレーム前の位置を保存（物理判定用）
	Vector3 oldPos = m_Position;
	m_IsGrounded = false;

	// ターゲット（プレイヤー）を探す
	Player* target = nullptr;
	std::vector<Player*> players = Game::GetInstance()->GetObjects<Player>();
	if (!players.empty())
	{
		target = players[0];
	}

	// ---------------------------------------------------------
	// 1. AI (人工知能) - 行動の決定
	// ---------------------------------------------------------
	if (target)
	{
		Vector3 diff = target->GetPosition() - m_Position;
		float distance = diff.Length();

		switch (m_State)
		{
		case EnemyState::IDLE: // 【待機】
			// プレイヤーが20m以内に来たら「追跡」開始
			if (distance < 20.0f)
			{
				m_State = EnemyState::CHASE;
			}
			break;

		case EnemyState::CHASE: // 【追跡】
		{
			// 攻撃範囲（3m）に入ったら「攻撃」へ移行
			if (distance < 3.0f)
			{
				m_State = EnemyState::ATTACK;
				m_AttackTimer = 0; // タイマーリセット
			}
			else
			{
				// プレイヤーの方へ移動
				diff.y = 0.0f; // 高さは無視
				diff.Normalize();

				// 移動
				float enemySpeed = 0.05f;
				m_Position += diff * enemySpeed;

				// 向きを変える
				m_Rotation.y = atan2(diff.x, diff.z);
			}
		}
		break;

		case EnemyState::ATTACK: // 【攻撃】
		{
			m_AttackTimer++; // 時間を進める

			// --- 攻撃の流れ ---
			// 0~60フレーム: 予備動作（溜め）。棒立ちで威圧
			// 60フレーム目: 攻撃判定発生！
			// 60~120フレーム: 硬直

			// 攻撃の瞬間！
			if (m_AttackTimer == 60)
			{
				// 1. 攻撃判定ボックスを作る（敵の2m前方）
				Vector3 forward(sin(m_Rotation.y), 0.0f, cos(m_Rotation.y));
				Vector3 attackPos = m_Position + (forward * 2.0f);
				attackPos.y += 2.0f; // 高さ調整

				DirectX::BoundingBox attackBox;
				attackBox.Center = attackPos;
				attackBox.Extents = Vector3(1.5f, 1.5f, 1.5f); // 3m四方の判定

				// 2. プレイヤーと当たり判定
				// ターゲットの位置に簡易的な球体判定を作ってチェック
				DirectX::BoundingSphere playerSphere(target->GetPosition() + Vector3(0, 1, 0), 1.0f);

				if (attackBox.Intersects(playerSphere))
				{
					// プレイヤーにダメージを与える
					target->OnDamage(15);

					// ログ出力（必要なら）
					// printf("Enemy Attack Hit!\n");
				}
			}

			// 攻撃終了（2秒経ったら追跡に戻る）
			if (m_AttackTimer > 120)
			{
				m_State = EnemyState::CHASE;
				m_AttackTimer = 0;
			}
		}
		break;
		}
	}

	// ---------------------------------------------------------
	// 2. 物理挙動 (重力)
	// ---------------------------------------------------------
	const float gravity = 0.05f;
	m_Velocity.y -= gravity;
	if (m_Velocity.y < -1.0f) m_Velocity.y = -1.0f;

	m_Position.y += m_Velocity.y;

	// ---------------------------------------------------------
	// 3. 地形との当たり判定 (Physics)
	// ---------------------------------------------------------

	// Groundの頂点データを取得
	std::vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();
	std::vector<VERTEX_3D> vertices;
	for (auto& g : grounds)
	{
		std::vector<VERTEX_3D> vecs = g->GetVertices();
		for (auto& v : vecs) vertices.emplace_back(v);
	}

	float playerRadius = 1.0f;
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
		Vector3 sphereCenter = m_Position + offset;
		Vector3 oldSphereCenter = oldPos + offset;

		Collision::Segment moveSegment = { oldSphereCenter, sphereCenter };
		Collision::Sphere  playerSphere = { sphereCenter, playerRadius };

		// ① 球体としての押し戻し
		if (Collision::CheckHit(playerSphere, poly, hitPos))
		{
			Vector3 pushBackCenter = Collision::moveSphere(playerSphere, poly, hitPos);
			Vector3 normal = Collision::GetNormal(poly);

			Vector3 margin = normal * 0.03f;
			m_Position = (pushBackCenter - offset) + margin;

			if (normal.y > 0.5f)
			{
				m_Velocity.y = 0.0f;
				m_IsGrounded = true;
			}
		}
		// ② すり抜け防止
		else if (Collision::CheckHit(moveSegment, poly, hitPos))
		{
			float dist = 0;
			Vector3 correctedCenter = Collision::moveSphere(moveSegment, playerRadius, poly, hitPos, dist);
			Vector3 normal = Collision::GetNormal(poly);

			Vector3 margin = normal * 0.03f;
			m_Position = (correctedCenter - offset) + margin;

			if (normal.y > 0.5f)
			{
				m_Velocity.y = 0.0f;
				m_IsGrounded = true;
			}
		}
	}

	// ---------------------------------------------------------
	// 4. その他処理 (リスポーン、UI、デバッグ)
	// ---------------------------------------------------------

	// 奈落リスポーン
	if (m_Position.y < -100.0f)
	{
		Init();
		m_Velocity = Vector3::Zero;
	}

	// デバッグ切り替え
	if (Input::GetKeyTrigger('V'))
	{
		ToggleDebugMode();
	}

	// HPバー更新
	if (m_HpBar)
	{
		m_HpBar->SetHP((float)m_HP, (float)m_MaxHP);
		m_HpBar->Update();
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
	// デバッグ描画（オンの時だけ実行）
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
	m_HpBar->Draw(cam);
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
