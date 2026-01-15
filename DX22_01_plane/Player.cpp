#include "Player.h"
#include "input.h"
#include "Game.h"
#include "Ground.h"
#include "Collision.h"
#include "AssimpPerse.h"
#include "Enemy.h"

using namespace DirectX::SimpleMath;

Player::Player()
{
	m_Speed = 0.5f;
	m_Velocity = Vector3::Zero;
	m_IsGrounded = false;
	m_AttackFrame = 0;
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
	StaticMesh debugMesh;
	// すでにあるゴルフボールのモデルを借用します
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
	m_HpBar = new HPBar();
	m_HpBar->Init();
	m_HpBar->SetColor(Color(0.0f, 1.0f, 0.0f, 1.0f)); // 緑
	m_HpBar->SetPosition(-450.0f, 300.0f, 0.0f);      // 左上配置
	m_HpBar->SetScale(300.0f, 20.0f, 1.0f);           // 幅300
}

void Player::Update()
{
	// 1フレーム前の位置を保存 (当たり判定用)
	Vector3 oldPos = m_Position;
	// =========================================================
	// 1. 攻撃開始の処理 
	// =========================================================

	// 攻撃していない(0)ときだけ、攻撃ボタンを受け付ける
	if (m_AttackFrame == 0)
	{
		if (Input::GetKeyTrigger(VK_LBUTTON))
		{
			m_AttackFrame = 1; // 攻撃開始！
			m_Velocity = Vector3::Zero; // 足を止める（モンハン風）
		}
	}

	// =========================================================
	// 2. 移動処理 (攻撃中は動けない)
	// =========================================================
	Vector3 moveDir = Vector3::Zero;
	if (m_AttackFrame == 0)
	{
		if (Input::GetKeyPress(VK_W)) moveDir.z += 1.0f;
		if (Input::GetKeyPress(VK_S)) moveDir.z -= 1.0f;
		if (Input::GetKeyPress(VK_A)) moveDir.x -= 1.0f;
		if (Input::GetKeyPress(VK_D)) moveDir.x += 1.0f;
	}

	if (moveDir.LengthSquared() > 0)
	{
		moveDir.Normalize();
		m_Position.x += moveDir.x * m_Speed;
		m_Position.z += moveDir.z * m_Speed;
		m_Rotation.y = atan2(-moveDir.x, -moveDir.z); // 回転
	}

	// =========================================================
	// 3. 攻撃中の処理 (当たり判定)
	// =========================================================
	// 毎回リセット
	m_IsAttackActive = false;

	if (m_AttackFrame > 0)
	{
		m_AttackFrame++;

		if (m_AttackFrame > 30)
		{
			m_AttackFrame = 0;
		}

		if (m_AttackFrame >= 10 && m_AttackFrame <= 15)
		{
			// --- 攻撃の当たり判定 (球) ---
			// 攻撃側は「球」のままでOKです
			float attackRange = 2.0f;
			Vector3 forward;
			forward.x = sin(m_Rotation.y);
			forward.z = cos(m_Rotation.y);

			// 前へ出し、高さも合わせる
			Vector3 attackPos = m_Position - (forward * attackRange);
			attackPos.y += 5.0f;

			// デバッグ表示用
			m_DebugAttackPos = attackPos;
			m_IsAttackActive = true;

			// DirectX標準の球体データを作成
			DirectX::BoundingSphere atkSphere;
			atkSphere.Center = attackPos;
			atkSphere.Radius = 1.5f; // 攻撃の大きさ

			// --- 敵全員と当たり判定 (四角) ---
			std::vector<Enemy*> enemies = Game::GetInstance()->GetObjects<Enemy>();
			for (auto& enemy : enemies)
			{
				Vector3 enemyPos = enemy->GetPosition();

				// ★ここを変更！敵を「球」から「四角」にする
				DirectX::BoundingBox enemyBox;

				// 1. ボックスの中心座標 (足元 + 高さ半分)
				// 敵の身長がだいたい 3.0m だと仮定して、中心を 1.5m 上げる
				enemyBox.Center = enemyPos + Vector3(0.0f, 4.0f, 0.0f);

				// 2. ボックスの「半径」(幅・高さ・奥行きの半分)
				// 幅1.5m(半径0.75), 高さ3.0m(半径1.5), 奥行1.5m(半径0.75) の四角
				enemyBox.Extents = Vector3(2.5f, 6.0f, 2.5f);

				// ★判定関数: 球(攻撃) vs 四角(敵) が当たっているかチェック
				if (atkSphere.Intersects(enemyBox))
				{
					if (m_AttackFrame == 10)
					{
						enemy->OnDamage(20);
					}
				}
			}
		}
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
	m_HpBar->SetHP(100.0f, 100.0f); // 実際は m_HP, m_MaxHP を渡す
	m_HpBar->Update();
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
	if (m_IsAttackActive)
	{
		// 攻撃判定の位置にセット
		Matrix r = Matrix::Identity; // 回転なし
		Matrix t = Matrix::CreateTranslation(m_DebugAttackPos);

		// サイズ調整 (判定半径1.5mに合わせて、直径3mくらいに見えるように調整)
		// ゴルフボールの元のサイズ次第ですが、適当に大きさを変えてみてください
		float debugScale = 3.0f;
		Matrix s = Matrix::CreateScale(debugScale);

		Matrix worldmtx = s * r * t;
		Renderer::SetWorldMatrix(&worldmtx);

		m_DebugMesh.BeforeDraw(); // ボールの頂点情報をGPUにセット

		// ゴルフボールを描画
		for (int i = 0; i < m_DebugSubsets.size(); i++)
		{
			m_DebugMesh.DrawSubset(
				m_DebugSubsets[i].IndexNum,
				m_DebugSubsets[i].IndexBase,
				m_DebugSubsets[i].VertexBase);
		}
	}
	m_HpBar->Draw(cam);
}

void Player::Uninit()
{
}
