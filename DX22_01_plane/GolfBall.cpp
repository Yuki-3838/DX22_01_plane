#include "GolfBall.h"
#include "Collision.h"
#include "Game.h"
#include "Ground.h"
#include "Pole.h"

using namespace std;
using namespace DirectX::SimpleMath;

//=======================================
//初期化処理
//=======================================
void GolfBall::Init()
{
	// メッシュ読み込み
	StaticMesh staticmesh;

	//3Dモデルデータ
	std::u8string modelFile = u8"assets/model/golfball/golf_ball.obj";

	//テクスチャディレクトリ
	std::string texDirectory = "assets/model/golfball";

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
		m_Materials.push_back(std::move(m));
	}

	//モデルによってスケールを調整
	m_Scale.x = 3;
	m_Scale.y = 3;
	m_Scale.z = 3;
	
	//最初に速度を与える
	m_Velocity.x = 1.0f;
	m_Velocity.y = 0.5f;
	m_Velocity.z = 1.0f;
}

//=======================================
//更新処理
//=======================================
void GolfBall::Update()
{
	if (m_State != 0) return;//静止状態ならreturn
	Vector3 oldPos = m_Position;//1フレーム前の位置を記憶しておく
	//速度が0近づいたら停止
	if (m_Velocity.LengthSquared() < 0.001f)
	{
		m_StopCount++;
	}
	else
	{
		m_StopCount = 0;
		//減速度(1フレーム当たりどれくらい減速するか)
		float decelerationPower = 0.02f;

		Vector3 deceleration = -m_Velocity;//速度の逆ベクトルを計算
		deceleration.Normalize();//ベクトルの正規化
		m_Acceleration = deceleration * decelerationPower;

		//加速度を速度に加算
		m_Velocity += m_Acceleration;
	}
	//10フレーム連続で殆ど動いていなければ静止状態へ
	if (m_StopCount > 10)
	{
		m_Velocity = Vector3(0.0f, 0.0f, 0.0f);
		m_State = 1;//静止状態
	}
	//重力
	const float gravity = 0.05f;
	m_Velocity.y -= gravity;

	//速度を座標に加算
	m_Position += m_Velocity;

	float radius = 5.0f;//ボールモデルの直径

	//Groundの頂点データを取得
	vector<Ground*>grounds = Game::GetInstance()->GetObjects <Ground>();
	std::vector<VERTEX_3D> vertices;
	for (auto& g : grounds)//Groundオブジェクトの数ループ
	{
		vector<VERTEX_3D> vecs = g->GetVertices();
		for (auto& v : vecs)//頂点の数ループ
		{
			vertices.emplace_back(v);
		}
	}

	float moveDisgtance = 9999;//移動距離
	Vector3 contactPoint;//接触点
	Vector3 normal;

	//地面との当たり判定
	for (int i = 0; i < vertices.size(); i += 3)
	{
		//三角形ポリゴン
		Collision::Polygon collisionPolygon =
		{
			vertices[i + 0].position,
			vertices[i + 1].position,
			vertices[i + 2].position
		};

		Vector3 cp;//接触点
		Collision::Segment collisionSegment = { oldPos,m_Position };
		Collision::Sphere collisionSphere = { m_Position,radius };

		//線分とポリゴンの当たり判定
		if (Collision::CheckHit(collisionSegment, collisionPolygon, cp))
		{
			float md = 0;
			Vector3 np = Collision::moveSphere(collisionSegment, radius, collisionPolygon, cp, md);
			if (moveDisgtance > md)
			{
				moveDisgtance = md;
				m_Position = np;
				contactPoint = cp;
				normal = Collision::GetNormal(collisionPolygon);
			}
		}
		//球体とポリゴンの当たり判定
		else if (Collision::CheckHit(collisionSphere, collisionPolygon, cp))
		{
			Vector3 np = Collision::moveSphere(collisionSphere, collisionPolygon, cp);
			float md = (np - oldPos).Length();
			if (moveDisgtance > md)
			{
				moveDisgtance = md;
				m_Position = np;
				contactPoint = cp;
				normal = Collision::GetNormal(collisionPolygon);
			}
		}
	}

	if (moveDisgtance != 9999)//もし当たっていたら
	{
		//ボールの速度ベクトルの法線方向成分と接線方向成分を分解
		float velcityNormal = Collision::Dot(m_Velocity, normal);
		Vector3 v1 = velcityNormal * normal;//法線方向成分
		Vector3 v2 = m_Velocity - v1;//接線方向成分

		//反射ベクトル
		const float restitution = 0.5f;
		const float friction = 1.0f;
		Vector3 reflectedVelocity = v2 * friction - v1 * restitution;

		//反射ベクトルを計算
		m_Velocity = reflectedVelocity;
	}

	//下に落ちた時はリスポーン
	if (m_Position.y < -100)
	{
		m_Position = Vector3(0.0f, 50.0f, 0.0f);//リスポーン座標
		m_Velocity = Vector3(0.0f, 0.0f, 0.0f);//速度をリセット
	}

	//Poleの位置を取得
	vector<Pole*>pole = Game::GetInstance()->GetObjects<Pole>();
	if (pole.size() > 0)
	{
		Vector3 polePos = pole[0]->GetPosition();

		Collision::Sphere balCollision = { m_Position,radius };//ゴルフボールの当たり判定
		Collision::Sphere poleCollision = { polePos,2.0f };//ポールの当たり判定

		if (Collision::CheckHit(balCollision, poleCollision))
		{
			m_State = 2;//カップイン
		}
	}
}

//=======================================
//描画処理
//=======================================
void GolfBall::Draw(Camera* cam)
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
		// マテリアルをセット(サブセット情報の中にあるマテリアルインデックスを使用)
		m_Materials[m_subsets[i].MaterialIdx]->SetGPU();

		if (m_Materials[m_subsets[i].MaterialIdx]->isTextureEnable())
		{
			m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
		}

		m_MeshRenderer.DrawSubset(
			m_subsets[i].IndexNum,		// 描画するインデックス数
			m_subsets[i].IndexBase,		// 最初のインデックスバッファの位置	
			m_subsets[i].VertexBase);	// 頂点バッファの最初から使用
	}
}

//=======================================
//終了処理
//=======================================
void GolfBall::Uninit()
{

}

