#include "Game.h"
#include "Renderer.h"
#include "Input.h"

Game* Game::m_Instance;//ゲームインスタンス

// コンストラクタ
Game::Game()
{
	m_Scene = nullptr;
}

// デストラクタ
Game::~Game()
{
	delete m_Scene;
	DeleteAllObject();//全てのオブジェクトを削除(※シーンを跨いだオブジェクトが無ければ不要)
}

// 初期化
void Game::Init()
{
	//インスタンス作成
	m_Instance = new Game;

	// 描画初期化処理
	Renderer::Init();

	//入力処理初期化
	Input::Create();

	// カメラ初期化
	m_Instance->m_Camera.Init();

	//最初のシーンを読み込む
	m_Instance->m_Scene = new TitleScene;
}

// 更新
void Game::Update()
{
	//入力処理更新
	Input::Update();

	//シーン更新
	m_Instance->m_Scene->Update();

	// カメラ更新
	m_Instance->m_Camera.Update();
	
	//オブジェクト更新
	for (auto& o : m_Instance->m_Objects)
	{
		o->Update();
	}
}

// 描画
void Game::Draw()
{
	// 描画前処理
	Renderer::DrawStart();

	// テストオブジェクト描画
	
	//オブジェクト描画
	for (auto& o : m_Instance->m_Objects)
	{
		o->Draw(&m_Instance->m_Camera);
	}
	// 描画後処理
	Renderer::DrawEnd();
}

// 終了処理
void Game::Uninit()
{
	// カメラ終了処理
	m_Instance->m_Camera.Uninit();

	//入力処理終了
	Input::Release();
	
	//オブジェクト終了処理
	for (auto& o : m_Instance->m_Objects)
	{
		o->Uninit();
	}

	// 描画終了処理
	Renderer::Uninit();

	//インスタンス削除
	delete m_Instance;
}

//インスタンスを取得
Game* Game::GetInstance()
{
	return m_Instance;
}

//シーンを切り替える
void Game::ChangeScene(SceneName sName)
{
	//読み込み済みのシーンがあれば削除
	int score = 0;
	if (m_Instance->m_Scene != nullptr)
	{
		//消そうとしているシーンがStage1ならスコアを保存しておく
		if (Stage1Scene* sObj = dynamic_cast<Stage1Scene*>(m_Instance->m_Scene))
		{
			score = sObj->GetScore();
		}
		delete m_Instance->m_Scene;
		m_Instance->m_Scene = nullptr;
	}

	switch (sName)
	{
	case TITLE:
		m_Instance->m_Scene = new TitleScene;//メモリ確保
		break;
	case STAGE1:
		m_Instance->m_Scene = new Stage1Scene;//メモリ確保
		break;
	case RESULT:
		m_Instance->m_Scene = new ResultScene;//メモリ確保
		dynamic_cast<ResultScene*>(m_Instance->m_Scene)->SetScore(score);//スコアを設定
		break;
	}
}

//オブジェクトを削除する
void Game::DeleteObject(Object* pt)
{
	if (pt == NULL)return;

	pt->Uninit();//終了処理

	//要素を削除
	erase_if(m_Instance->m_Objects,
		[pt](const std::unique_ptr<Object>& element)
		{
			return element.get() == pt;
		});
	m_Instance->m_Objects.shrink_to_fit();
}

//オブジェクトをすべて削除する
void Game::DeleteAllObject()
{
	//オブジェクト終了処理
	for (auto& o : m_Instance->m_Objects)
	{
		o->Uninit();
	}

	m_Instance->m_Objects.clear();//全て削除
	m_Instance->m_Objects.shrink_to_fit();//
}



