#pragma once

#include "Object.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Camera.h"
#include "Shader.h"
#include"Texture.h"
#include "Material.h"

//-----------------------------------------------------------------------------
//TestCubeクラス
//-----------------------------------------------------------------------------
class Dice :public Object
{
	// 頂点データ
	std::vector<VERTEX_3D> m_Vertices;

	//インデックスデータ
	std::vector<unsigned int> m_Indices;

	// 描画の為の情報（メッシュに関わる情報）
	IndexBuffer	 m_IndexBuffer; // インデックスバッファ
	VertexBuffer<VERTEX_3D>	m_VertexBuffer; // 頂点バッファ

	std::unique_ptr<Material> m_Material;



	// 描画の為の情報（見た目に関わる部分）
	Texture m_Texture;//テクスチャ

public:
	void Init();
	void Update();
	void Draw(Camera* cam);
	void Uninit();
};
