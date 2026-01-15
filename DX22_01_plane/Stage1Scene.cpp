#include "Stage1Scene.h"
#include "Game.h"
#include "Input.h"
#include "GolfBall.h"
#include "Ground.h"
#include "Arrow.h"
#include "Pole.h"
#include"Player.h"
#include "Texture2D.h"
#include "Enemy.h"

using namespace DirectX::SimpleMath;

// コンストラクタ
Stage1Scene::Stage1Scene()
{
	Init();
}

// デストラクタ
Stage1Scene::~Stage1Scene()
{
	Uninit();
}

// 初期化
void Stage1Scene::Init()
{
	m_Par = 4;//パー(標準打数)を設定
	m_StrokeCount = 0; //現在打数を初期化
	// オブジェクトを作成
	/*m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<GolfBall>());*/
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Ground>());
	//m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Arrow>());//矢印
	//m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Pole>());//ポール
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Player>());
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Enemy>());

	//GolfBall* ball = dynamic_cast<GolfBall*>(m_MySceneObjects[0]);//ゴルフボール
	//Arrow* arrow = dynamic_cast<Arrow*>(m_MySceneObjects[2]);//矢印
	//Pole* pole = dynamic_cast<Pole*>(m_MySceneObjects[3]);//矢印
	//ball->SetState(0);//ボールを物理挙動させる
	//arrow->SetState(0);//矢印を非表示
	//pole->SetPosition(10, 0, 10);//ポールの場所を設定

	//UI(背景)
	Texture2D* pt1 = Game::GetInstance()->AddObject<Texture2D>();
	pt1->SetTexture("assets/texture/ui_back.png");//画像を指定
	pt1->SetPosition(-475.0f, -300.0f, 0.0f);//位置を指定
	pt1->SetScale(270.0f, 75.0f, 0.0f);//大きさを指定
	m_MySceneObjects.emplace_back(pt1);

	//UI(「パー」)
	Texture2D* pt2 = Game::GetInstance()->AddObject<Texture2D>();
	pt2->SetTexture("assets/texture/ui_string.png");//画像を指定
	pt2->SetPosition(-575.0f, -245.0f, 0.0f);//位置を指定
	pt2->SetScale(60.0f, 45.0f, 0.0f);//大きさを指定
	pt2->SetUV(1, 1, 2, 1);//UVを指定
	m_MySceneObjects.emplace_back(pt2);

	//UI(「打目」)
	Texture2D* pt3 = Game::GetInstance()->AddObject<Texture2D>();
	pt3->SetTexture("assets/texture/ui_string.png");//画像を指定
	pt3->SetPosition(-400.0f, -305.0f, 0.0f);//位置を指定
	pt3->SetScale(105.0f, 63.0f, 0.0f);//大きさを指定
	pt3->SetUV(2, 1, 2, 1);//UVを指定
	m_MySceneObjects.emplace_back(pt3);

	//UI(「パーの数値」)
	Texture2D* pt4 = Game::GetInstance()->AddObject<Texture2D>();
	pt4->SetTexture("assets/texture/number.png");//画像を指定
	pt4->SetPosition(-510.0f, -245.0f, 0.0f);//位置を指定
	pt4->SetScale(60.0f, 45.0f, 0.0f);//大きさを指定
	pt4->SetUV((float)(m_Par + 1), 1, 10, 1);//UVを指定
	m_MySceneObjects.emplace_back(pt4);

	//UI(「現在打数の数値 一の位)
	Texture2D* pt5 = Game::GetInstance()->AddObject<Texture2D>();
	pt5->SetTexture("assets/texture/number.png");//画像を指定
	pt5->SetPosition(-485.0f, -300.0f, 0.0f);//位置を指定
	pt5->SetScale(95.0f, 72.0f, 0.0f);//大きさを指定
	pt5->SetUV(2, 1, 10, 1);//UVを指定
	m_MySceneObjects.emplace_back(pt5);

	//UI(「現在打数の数値 十の位)
	Texture2D* pt6 = Game::GetInstance()->AddObject<Texture2D>();
	pt6->SetTexture("assets/texture/number.png");//画像を指定
	pt6->SetPosition(-556.0f, -300.0f, 0.0f);//位置を指定
	pt6->SetScale(95.0f, 72.0f, 0.0f);//大きさを指定
	pt6->SetUV(1, 1, 10, 1);//UVを指定
	m_MySceneObjects.emplace_back(pt6);
}

//更新
void Stage1Scene::Update()
{
	//GolfBall* ball = dynamic_cast<GolfBall*>(m_MySceneObjects[0]);//ゴルフボール
	//Arrow* arrow = dynamic_cast<Arrow*>(m_MySceneObjects[2]);//矢印

	////状態ごとに処理
	//switch (m_State)
	//{
	////ボールの移動中
	//case 0:
	//	//ボールが静止したら
	//	if (ball->GetState() == 1)
	//	{
	//		m_State = 1;
	//		arrow->SetState(m_State);

	//		//打数の更新
	//		Texture2D* count[2] = {};
	//		count[0] = dynamic_cast<Texture2D*>(m_MySceneObjects[8]);//現在の数値 一の位
	//		count[1] = dynamic_cast<Texture2D*>(m_MySceneObjects[9]);//現在の数値 十の位

	//		m_StrokeCount++;//現在打数をカウントアップ

	//		//各桁を後ろから取得していく
	//		for (int i = 0; i < 2; i++)
	//		{
	//			int cnt = m_StrokeCount % (int)pow(10, i + 1) / (int)pow(10, i);//1桁取り出す

	//			count[i]->SetUV((float)(cnt + 1), 1, 10, 1);//UVを設定
	//		}
	//	}
	//	//ボールカップインしたらリザルトへ
	//	else if (ball->GetState() == 2)
	//	{
	//		Game::GetInstance()->ChangeScene(RESULT);
	//	}
	//	break;
	////方向選択中
	//case 1:
	//	//スペースキーでパワーを選択へ
	//	if (Input::GetKeyTrigger(VK_SPACE))
	//	{
	//		m_State = 2;
	//		arrow->SetState(m_State);
	//	}
	//	break;
	////パワー選択中
	//case 2:
	//	//スペースキーでショット
	//	if (Input::GetKeyTrigger(VK_SPACE))
	//	{
	//		m_State = 0;
	//		ball->SetState(m_State);
	//		arrow->SetState(m_State);

	//		Vector3 v = arrow->GetVector();
	//		ball->Shot(v);
	//	}
	//	break;
	//}
	// エンターキーを押してリザルトへ
	if (Input::GetKeyTrigger(VK_RETURN))
	{
		Game::GetInstance()->ChangeScene(RESULT);
	}
}
// 終了処理
void Stage1Scene::Uninit()
{
	// このシーンのオブジェクトを削除する
	for (auto& o : m_MySceneObjects) {
		Game::GetInstance()->DeleteObject(o);
	}
	m_MySceneObjects.clear();
}
//スコアを取得
int Stage1Scene::GetScore() const
{
	//現在打数から標準打数を引いた数値をreturn
	return (m_StrokeCount - m_Par);
}
